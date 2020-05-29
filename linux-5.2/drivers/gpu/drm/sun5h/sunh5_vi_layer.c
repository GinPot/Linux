#include <linux/regmap.h>
#include <linux/clk.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drmP.h>

#include "sunh5_mixer.h"
#include "sunh5_vi_layer.h"
#include "sunh5_vi_scaler.h"
#include "sunh5_csc.h"






static int sunh5_vi_drm_fb_cma_prepare_fb(struct drm_plane *plane, struct drm_plane_state *state)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	return drm_gem_fb_prepare_fb(plane, state);
}

static int sunh5_vi_layer_atomic_check(struct drm_plane *plane, struct drm_plane_state *state)
{
	struct sunh5_vi_layer *layer = plane_to_sunh5_vi_layer(plane);
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	int min_scale, max_scale;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	if(!crtc)
		return 0;

	crtc_state = drm_atomic_get_existing_crtc_state(state->state, crtc);
	if(WARN_ON(!crtc_state))
		return -EINVAL;

	min_scale = DRM_PLANE_HELPER_NO_SCALING;
	max_scale = DRM_PLANE_HELPER_NO_SCALING;

	if(layer->mixer->cfg->scaler_mask & BIT(layer->channel)){
		min_scale = SUNH5_VI_SCALER_SCALE_MIN;
		max_scale = SUNH5_VI_SCALER_SCALE_MAX;
	}

	return drm_atomic_helper_check_plane_state(state, crtc_state, min_scale, max_scale, true, true);
}

static void sunh5_vi_layer_enable(struct sunh5_mixer *mixer, int channel, int overlay, bool enable,
										unsigned int zpos, unsigned int old_zops)
{
	u32 val, bld_base, ch_base;

	bld_base = sunh5_blender_base(mixer);
	ch_base = sunh5_channel_base(mixer,  channel);

	dev_dbg(mixer->engine.dev, "%s: Enter, %sabling VI channel %d overlay %d\n", __func__,  enable  ? "En" : "Dis", channel, overlay);

	if(enable)
		val = SUNH5_MIXER_CHAN_VI_LAYER_ATTR_EN;
	else
		val = 0;

	regmap_update_bits(mixer->engine.regs, 
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR(ch_base, overlay),
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR_EN, val);

	if(!enable || zpos != old_zops){
		regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_BLEND_PIPE_CTL(bld_base),
						SUNH5_MIXER_BLEND_PIPE_CTL_EN(old_zops),
						0);

		regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_BLEND_ROUTE(bld_base),
						SUNH5_MIXER_BLEND_ROUTE_PIPE_MSK(old_zops),
						0);
	}

	if(enable){
		val = SUNH5_MIXER_BLEND_PIPE_CTL_EN(zpos);

		regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_BLEND_PIPE_CTL(bld_base),
						val, val);

		val = channel << SUNH5_MIXER_BLEND_ROUTE_PIPE_SHIFT(zpos);

		regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_BLEND_ROUTE(bld_base),
						SUNH5_MIXER_BLEND_ROUTE_PIPE_MSK(zpos),
						val);
	}				
}

static void sunh5_vi_layer_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	struct sunh5_vi_layer *layer = plane_to_sunh5_vi_layer(plane);
	unsigned int old_zpos = old_state->normalized_zpos;
	struct sunh5_mixer *mixer = layer->mixer;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	sunh5_vi_layer_enable(mixer, layer->channel, layer->overlay, false, 0, old_zpos);
}

static int sunh5_vi_layer_update_coord(struct sunh5_mixer *mixer, int channel,
										int overlay, struct drm_plane *plane,
										unsigned int zpos)
{
	struct drm_plane_state *state = plane->state;
	const struct drm_format_info *format = state->fb->format;
	u32 src_w, src_h, dst_w, dst_h;
	u32 bld_base, ch_base;
	u32 outsize, insize;
	u32 hphase, vphase;
	u32 hn = 0, hm = 0;
	u32 vn = 0, vm = 0;
	bool subsampled;
	

	dev_dbg(plane->dev->dev, "%s: Enter, Updating VI channel %d overlay %d\n", __func__, channel, overlay);

	bld_base = sunh5_blender_base(mixer);
	ch_base = sunh5_channel_base(mixer, channel);

	src_w = drm_rect_width(&state->src) >> 16;
	src_h = drm_rect_height(&state->src) >> 16;
	dst_w = drm_rect_width(&state->dst);
	dst_h = drm_rect_height(&state->dst);

	hphase = state->src.x1 & 0xffff;
	vphase = state->src.y1 & 0xffff;

	/* make coordinates dividable by subsampling factor */
	if(format->hsub > 1){
		int mask, remainder;

		mask = format->hsub - 1;
		remainder = (state->src.x1 >> 16) & mask;
		src_w = (src_w + remainder) & ~mask;
		hphase += remainder << 16;
	}

	if(format->vsub > 1){
		int mask, remainder;

		mask = format->vsub - 1;
		remainder = (state->src.y1 >>  16) & mask;
		src_h = (src_h + remainder) & ~mask;
		vphase += remainder << 16;
	}

	insize = SUNH5_MIXER_SIZE(src_w, src_h);
	outsize = SUNH5_MIXER_SIZE(dst_w, dst_h);

	/* Set height and width */
	dev_dbg(plane->dev->dev, "%s: Layer source offset X: %d  Y: %d\n", __func__,
																		(state->src.x1 >> 16) & ~(format->hsub - 1),
																		(state->src.y1 >> 16) & ~(format->vsub - 1));
	dev_dbg(plane->dev->dev, "%s: Layer source size W: %d  H: %d\n", __func__, src_w, src_h);
	regmap_write(mixer->engine.regs,
					SUNH5_MIXER_CHAN_VI_LAYER_SIZE(ch_base, overlay),
					insize);
	regmap_write(mixer->engine.regs,
					SUNH5_MIXER_CHAN_VI_OVL_SIZE(ch_base),
					insize);

	/*
	 * Scaler must be enabled for subsampled formats, so it scales
	 * chroma to same size sa luma.
	 */
	subsampled = format->hsub > 1 || format->vsub > 1;

	if(insize != outsize || subsampled || hphase || vphase){
		unsigned int scanline, required;
		struct drm_display_mode *mode;
		u32 hscale, vscale, fps;
		u64 ability;

		dev_dbg(plane->dev->dev, "%s: HW scaling is enabled\n", __func__);

		mode = &plane->state->crtc->state->mode;
		fps = (mode->clock * 1000) / (mode->vtotal * mode->htotal);
		ability = clk_get_rate(mixer->mode_clk);

		/* BSP algorithm assumes 80% efficiency of VI scaler unit */
		ability *= 80;
		do_div(ability, mode->vdisplay * fps * max(src_w, dst_w));

		required = src_h * 100 / dst_h;

		if(ability < required){
			dev_dbg(plane->dev->dev, "%s: Using vertical coarse scaling\n", __func__);
			vm = src_h;
			vn = (u32)ability * dst_h / 100;
			src_h = vn;
		}

		/* it seems that every RGB scaler has buffer for 2048 pixels */
		scanline = subsampled ? mixer->cfg->scanline_yuv : 2048;

		if(src_w > scanline){
			dev_dbg(plane->dev->dev, "%s: Using horizontal coarse scaling\n", __func__);
			hm = src_w;
			hn = scanline;
			src_w = hn;
		}

		hscale = (src_w << 16) / dst_w;
		vscale = (src_h << 16) / dst_h;

		sunh5_vi_scaler_setup(mixer, channel, src_w, src_h, dst_w, dst_h,
								hscale, vscale, hphase, vphase, format);
		sunh5_vi_scaler_enable(mixer, channel, true);
	}
	else{
		dev_dbg(plane->dev->dev, "%s: HW scaling is not needed\n", __func__);
		sunh5_vi_scaler_enable(mixer, channel, false);
	}

	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_HDS_Y(ch_base), SUNH5_MIXER_CHAN_VI_DS_N(hn) | SUNH5_MIXER_CHAN_VI_DS_M(hm));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_HDS_UV(ch_base), SUNH5_MIXER_CHAN_VI_DS_N(hn) | SUNH5_MIXER_CHAN_VI_DS_M(hm));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_VDS_Y(ch_base), SUNH5_MIXER_CHAN_VI_DS_N(hn) | SUNH5_MIXER_CHAN_VI_DS_M(hm));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_VDS_UV(ch_base), SUNH5_MIXER_CHAN_VI_DS_N(hn) | SUNH5_MIXER_CHAN_VI_DS_M(hm));
	
	/* Set base coordinates */
	dev_dbg(plane->dev->dev, "%s: Layer destination coordinates X: %d  Y: %d\n", __func__, state->dst.x1, state->dst.y1);
	dev_dbg(plane->dev->dev, "%s: Layer destination size W: %d  H: %d\n", __func__, dst_w, dst_h);

	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_ATTR_COORD(bld_base, zpos), SUNH5_MIXER_COORD(state->dst.x1, state->dst.y1));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_ATTR_INSIZE(bld_base, zpos), outsize);

	return 0;
}

static int sunh5_vi_layer_update_format(struct sunh5_mixer *mixer, int channel, int overlay, struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	const struct de2_fmt_info *fmt_info;
	u32 val, ch_base;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	ch_base = sunh5_channel_base(mixer, channel);

	fmt_info = sunh5_mixer_format_info(state->fb->format->format);
	if(!fmt_info){
		dev_dbg(plane->dev->dev, "%s: Invalid Format\n", __func__);
		return -EINVAL;
	}

	val = fmt_info->de2_fmt << SUNH5_MIXER_CHAN_VI_LAYER_ATTR_FBFMT_OFFSET;
	regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR(ch_base, overlay),
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR_FBFMT_MASK, val);

	if(fmt_info->csc != SUNH5_CSC_MODE_OFF){
		sunh5_csc_set_ccsc_coefficients(mixer, channel, fmt_info->csc);
		sunh5_csc_enable_ccsc(mixer, channel, true);
	}else{
		sunh5_csc_enable_ccsc(mixer, channel, false);
	}

	if(fmt_info->rgb)
		val = SUNH5_MIXER_CHAN_VI_LAYER_ATTR_RGB_MODE;
	else
		val = 0;

	regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR(ch_base, overlay),
						SUNH5_MIXER_CHAN_VI_LAYER_ATTR_RGB_MODE, val);

	return 0;
}

static int sunh5_vi_layer_update_buffer(struct sunh5_mixer *mixer, int channel, int overlay, struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	struct drm_framebuffer *fb = state->fb;
	const struct drm_format_info *format = fb->format;
	struct drm_gem_cma_object *gem;
	u32 dx, dy, src_x, src_y;
	dma_addr_t paddr;
	u32 ch_base;
	int i;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	ch_base = sunh5_channel_base(mixer, channel);

	/* Adjust x and y to be dividable by subsampling factor */
	src_x = (state->src.x1 >> 16) & ~(format->hsub - 1);
	src_y = (state->src.y1 >> 16) & ~(format->vsub - 1);

	for(i = 0; i < format->num_planes; i++){
		/* Get the physical address of the buffer in memory */
		gem = drm_fb_cma_get_gem_obj(fb, i);

		dev_dbg(plane->dev->dev, "%s: Using GEM %pad\n", __func__, &gem->paddr);

		/* Compute the start of the displayed memory */
		paddr = gem->paddr + fb->offsets[i];

		dx = src_x;
		dy = src_y;

		if(i > 0){
			dx /= format->hsub;
			dy /= format->vsub;
		}

		/* Fixup framebuffer address for src coordinates */
		paddr += dx * format->cpp[i];
		paddr += dy * fb->pitches[i];

		/* Set the line width */
		dev_dbg(plane->dev->dev, "%s: Layer %d. line width: %d bytes\n", __func__, i + 1, fb->pitches[i]);
		regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_LAYER_PITCH(ch_base, overlay, i), fb->pitches[i]);

		dev_dbg(plane->dev->dev, "%s: Setting %d. buffer address to %pad\n", __func__, i + 1, &paddr);

		regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_VI_LAYER_TOP_LADDR(ch_base, overlay, i), lower_32_bits(paddr));
	}

	return 0;
}

static void sunh5_vi_layer_atomic_update(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	struct sunh5_vi_layer *layer = plane_to_sunh5_vi_layer(plane);
	unsigned int zpos = plane->state->normalized_zpos;
	unsigned int old_zpos = old_state->normalized_zpos;
	struct sunh5_mixer *mixer = layer->mixer;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	if(!plane->state->visible){
		sunh5_vi_layer_enable(mixer, layer->channel, layer->overlay, false, 0, old_zpos);
		return;
	}

	sunh5_vi_layer_update_coord(mixer, layer->channel, layer->overlay, plane, zpos);
	sunh5_vi_layer_update_format(mixer, layer->channel, layer->overlay, plane);
	sunh5_vi_layer_update_buffer(mixer, layer->channel, layer->overlay, plane);
	sunh5_vi_layer_enable(mixer, layer->channel, layer->overlay, true, zpos, old_zpos);
}

static const struct drm_plane_helper_funcs sunh5_vi_layer_helper_funcs = {
	.prepare_fb		= sunh5_vi_drm_fb_cma_prepare_fb,
	.atomic_check	= sunh5_vi_layer_atomic_check,
	.atomic_disable = sunh5_vi_layer_atomic_disable,
	.atomic_update	= sunh5_vi_layer_atomic_update,
};

static int sunh5_vi_drm_atomic_helper_disable_plane(struct drm_plane *plane,
				    struct drm_modeset_acquire_ctx *ctx)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

 	return drm_atomic_helper_disable_plane(plane, ctx);
}

static void sunh5_vi_drm_plane_cleanup(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	drm_plane_cleanup(plane);
}

static void sunh5_vi_drm_atomic_helper_plane_reset(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	drm_atomic_helper_plane_reset(plane);
}

static struct drm_plane_state *sunh5_vi_drm_atomic_helper_plane_duplicate_state(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_plane_duplicate_state(plane);
}

static void sunh5_vi_drm_atomic_helper_plane_destroy_state(struct drm_plane *plane,
					   struct drm_plane_state *state)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_plane_destroy_state(plane, state);
}

static int sunh5_vi_drm_atomic_helper_update_plane(struct drm_plane *plane,
				   struct drm_crtc *crtc,
				   struct drm_framebuffer *fb,
				   int crtc_x, int crtc_y,
				   unsigned int crtc_w, unsigned int crtc_h,
				   uint32_t src_x, uint32_t src_y,
				   uint32_t src_w, uint32_t src_h,
				   struct drm_modeset_acquire_ctx *ctx)
{

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	
	return drm_atomic_helper_update_plane(plane,
				   crtc,
				   fb,
				   crtc_x, crtc_y,
				   crtc_w, crtc_h,
				   src_x, src_y,
				   src_w, src_h,
				   ctx);
}

static const struct drm_plane_funcs sunh5_vi_layer_funcs = {
	.atomic_destroy_state	= sunh5_vi_drm_atomic_helper_plane_destroy_state,
	.atomic_duplicate_state	= sunh5_vi_drm_atomic_helper_plane_duplicate_state,
	.destroy				= sunh5_vi_drm_plane_cleanup,
	.disable_plane			= sunh5_vi_drm_atomic_helper_disable_plane,
	.reset					= sunh5_vi_drm_atomic_helper_plane_reset,
	.update_plane			= sunh5_vi_drm_atomic_helper_update_plane,
};

/*
 * While all RGB formats are supported, VI planes don't support
 * alpha blending, so there is no point having formats with alpha
 * channel if their opaque analog exist.
 */
static const u32 sunh5_vi_layer_formats[] = {
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_BGRA5551,
	DRM_FORMAT_BGRA4444,
	DRM_FORMAT_BGRX8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_XRGB8888,

	DRM_FORMAT_NV16,
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV61,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_YUV411,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YVU411,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YVU422,
	DRM_FORMAT_YVU444,
};

struct sunh5_vi_layer *sunh5_vi_layer_init_one(struct drm_device *drm,
												struct sunh5_mixer *mixer,
												int index)
{
	struct sunh5_vi_layer *layer;
	unsigned int plane_cnt;
	int ret;

	dev_dbg(drm->dev, "%s: Enter\n", __func__);

	layer = devm_kzalloc(drm->dev, sizeof(*layer), GFP_KERNEL);
	if(!layer)
		return ERR_PTR(-ENOMEM);

	/* possible crtcs are set later */
	ret = drm_universal_plane_init(drm, &layer->plane, 0,
									&sunh5_vi_layer_funcs,
									sunh5_vi_layer_formats,
									ARRAY_SIZE(sunh5_vi_layer_formats),
									NULL, DRM_PLANE_TYPE_OVERLAY, NULL);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't initialize layer\n", __func__);
		return ERR_PTR(ret);
	}

	plane_cnt = mixer->cfg->ui_num + mixer->cfg->vi_num;

	ret = drm_plane_create_zpos_property(&layer->plane, index, 0 , plane_cnt - 1);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't add zpos property\n", __func__);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(&layer->plane, &sunh5_vi_layer_helper_funcs);
	layer->mixer = mixer;
	layer->channel = index;
	layer->overlay = 0;

	return layer;
}
