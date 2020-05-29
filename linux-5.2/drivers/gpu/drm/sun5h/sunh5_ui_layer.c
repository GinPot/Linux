#include <linux/regmap.h>

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
#include "sunh5_ui_layer.h"
#include "sunh5_ui_scaler.h"



static int sunh5_ui_drm_fb_cma_prepare_fb(struct drm_plane *plane, struct drm_plane_state *state)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	return drm_gem_fb_prepare_fb(plane, state);
}

static int sunh5_ui_layer_atomic_check(struct drm_plane *plane, struct drm_plane_state *state){
	struct sunh5_ui_layer *layer = plane_to_sunh5_ui_layer(plane);
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	int min_scale, max_scale;

	if(!crtc)
		return 0;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	crtc_state = drm_atomic_get_existing_crtc_state(state->state, crtc);
	if(WARN_ON(!crtc_state))
		return -EINVAL;

	min_scale = DRM_PLANE_HELPER_NO_SCALING;
	max_scale = DRM_PLANE_HELPER_NO_SCALING;

	if(layer->mixer->cfg->scaler_mask & BIT(layer->channel)){
		min_scale = SUNH5_UI_SCALER_SCALE_MIN;
		max_scale = SUNH5_UI_SCALER_SCALE_MAX;
	}

	return drm_atomic_helper_check_plane_state(state, crtc_state, min_scale, max_scale, true, true);
}

static void sunh5_ui_layer_enable(struct sunh5_mixer *mixer, int channel, int overlay, bool enable, unsigned int zpos, unsigned int old_zpos)
{
	u32 val, bld_base, ch_base;

	bld_base = sunh5_blender_base(mixer);
	ch_base = sunh5_channel_base(mixer, channel);

	dev_dbg(mixer->engine.dev, "%s: Enter, %sabling UI channel %d overlay %d\n", __func__,  enable  ? "En" : "Dis", channel, overlay);

	if(enable)
		val = SUNH5_MIXER_CHAN_UI_LAYER_ATTR_EN;
	else
		val = 0;

	regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_CHAN_UI_LAYER_ATTR(ch_base, overlay),
						SUNH5_MIXER_CHAN_UI_LAYER_ATTR_EN, val);

	if(!enable || zpos != old_zpos){
		regmap_update_bits(mixer->engine.regs,
							SUNH5_MIXER_BLEND_PIPE_CTL(bld_base),
							SUNH5_MIXER_BLEND_PIPE_CTL_EN(old_zpos),
							0);
		regmap_update_bits(mixer->engine.regs,
							SUNH5_MIXER_BLEND_ROUTE(bld_base),
							SUNH5_MIXER_BLEND_ROUTE_PIPE_MSK(old_zpos),
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

static void sunh5_ui_layer_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	struct sunh5_ui_layer *layer = plane_to_sunh5_ui_layer(plane);
	unsigned int old_zpos = old_state->normalized_zpos;
	struct sunh5_mixer *mixer = layer->mixer;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	sunh5_ui_layer_enable(mixer, layer->channel, layer->overlay, false, 0, old_zpos);
}

static int sunh5_ui_layer_update_coord(struct sunh5_mixer *mixer, int channel, int overlay,
												struct drm_plane *plane, unsigned int zpos)
{
	struct drm_plane_state *state = plane->state;
	u32 src_w, src_h, dst_w, dst_h;
	u32 bld_base, ch_base;
	u32 outsize, insize;
	u32 hphase, vphase;

	dev_dbg(plane->dev->dev, "%s: Enter, Updating UI channel %d overlay %d\n", __func__, channel, overlay);

	bld_base = sunh5_blender_base(mixer);
	ch_base = sunh5_channel_base(mixer, channel);

	src_w = drm_rect_width(&state->src) >> 16;
	src_h = drm_rect_height(&state->src) >> 16;
	dst_w = drm_rect_width(&state->dst);
	dst_h = drm_rect_height(&state->dst);

	hphase = state->src.x1 & 0xffff;
	vphase = state->src.y1 & 0xffff;

	insize = SUNH5_MIXER_SIZE(src_w, src_h);
	outsize = SUNH5_MIXER_SIZE(dst_w, dst_h);

	if(plane->type == DRM_PLANE_TYPE_PRIMARY){
		bool interlaced = false;
		u32 val;

		dev_dbg(plane->dev->dev, "%s: Primary layer, updating global size W: %u  H: %u\n", __func__, dst_w, dst_h);
		regmap_write(mixer->engine.regs, SUNH5_MIXER_GLOBAL_SIZE, outsize);
		regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_OUTSIZE(bld_base), outsize);

		if(state->crtc)
			interlaced = state->crtc->state->adjusted_mode.flags & DRM_MODE_FLAG_INTERLACE;

		if(interlaced)
			val = SUNH5_MIXER_BLEND_OUTCTL_INTERLACED;
		else
			val = 0;

		regmap_update_bits(mixer->engine.regs, SUNH5_MIXER_BLEND_OUTCTL(bld_base), SUNH5_MIXER_BLEND_OUTCTL_INTERLACED, val);

		dev_dbg(plane->dev->dev, "%s: Switching display mixer interlaced mode %s\n", __func__, interlaced ? "on" : "off");
	}

	/* Set height and width */
	dev_dbg(plane->dev->dev, "%s: Layer sourcr offset X: %d  Y: %d\n", __func__, state->src.x1 >> 16, state->src.y1 >> 16);
	dev_dbg(plane->dev->dev, "%s: Layer sourcr size W: %d  H: %d\n", __func__, src_w, src_h);

	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_UI_LAYER_SIZE(ch_base, overlay), insize);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_UI_OVL_SIZE(ch_base), insize);

	if(insize != outsize || hphase || vphase){
		u32 hscale, vscale;

		dev_dbg(plane->dev->dev, "%s: HW scaling is enabled\n", __func__);

		hscale = state->src_w / state->crtc_w;
		vscale = state->src_h / state->crtc_h;

		sunh5_ui_scaler_setup(mixer, channel, src_w, src_h, dst_w, dst_h, hscale, vscale, hphase, vphase);
		sunh5_ui_scaler_enable(mixer, channel, true);
	}else{
		dev_dbg(plane->dev->dev, "%s: HW scaling is not needed\n", __func__);
		sunh5_ui_scaler_enable(mixer, channel, false);
	}

	/* Set base coordinat */
	dev_dbg(plane->dev->dev, "%s: Layer destination coordinates X: %d  Y: %d\n", __func__,
													state->dst.x1, state->dst.y1);
	dev_dbg(plane->dev->dev, "%s: Layer destination size W: %d  H: %d\n", __func__,
													dst_w, dst_h);

	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_ATTR_COORD(bld_base, zpos),
									SUNH5_MIXER_COORD(state->dst.x1, state->dst.y1));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_ATTR_INSIZE(bld_base, zpos), outsize);

	return 0;
}

static int sunh5_ui_layer_update_formats(struct sunh5_mixer *mixer, int channel,
												int overlay, struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	const struct de2_fmt_info *fmt_info;
	u32 val, ch_base;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	ch_base = sunh5_channel_base(mixer, channel);

	fmt_info = sunh5_mixer_format_info(state->fb->format->format);
	if(!fmt_info || !fmt_info->rgb){
		dev_dbg(plane->dev->dev, "%s: Invalid format\n", __func__);
		return -EINVAL;
	}

	val = fmt_info->de2_fmt << SUNH5_MIXER_CHAN_UI_LAYER_ATTR_FBFMT_OFFSET;
	regmap_update_bits(mixer->engine.regs,
						SUNH5_MIXER_CHAN_UI_LAYER_ATTR(ch_base, overlay),
						SUNH5_MIXER_CHAN_UI_LAYER_ATTR_FBFMT_MASK, val);

	return 0;
}

static int sunh5_ui_layer_update_buffer(struct sunh5_mixer *mixer, int channel, 
												int overlay, struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	struct drm_framebuffer *fb = state->fb;
	struct drm_gem_cma_object *gem;
	dma_addr_t paddr;
	u32 ch_base;
	int bpp;

	ch_base = sunh5_channel_base(mixer, channel);

	/* Get the physical address of the buffer in memory */
	gem = drm_fb_cma_get_gem_obj(fb, 0);

	dev_dbg(plane->dev->dev, "%s: Using GEM @ %pad\n", __func__, &gem->paddr);

	/* Compute the start of the displayed memory */
	bpp = fb->format->cpp[0];
	paddr = gem->paddr + fb->offsets[0];

	/* Fixup framebuffer address for src coordinates */
	paddr += (state->src.x1 >> 16) * bpp;
	paddr += (state->src.y1 >> 16) * fb->pitches[0];

	/* Set the line width */
	dev_dbg(plane->dev->dev, "%s: Layer line width: %d bytes\n", __func__, fb->pitches[0]);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_UI_LAYER_PITCH(ch_base, overlay), fb->pitches[0]);

	dev_dbg(plane->dev->dev, "%s: Setting buffer address to %pad\n", __func__, &paddr);

	regmap_write(mixer->engine.regs, SUNH5_MIXER_CHAN_UI_LAYER_TOP_LADDR(ch_base, overlay), lower_32_bits(paddr));
	
	return 0;
}

static void sunh5_ui_layer_atomic_update(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	struct sunh5_ui_layer *layer = plane_to_sunh5_ui_layer(plane);
	unsigned int zpos = plane->state->normalized_zpos;
	unsigned int old_zpos = old_state->normalized_zpos;
	struct sunh5_mixer *mixer = layer->mixer;

	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

	if(!plane->state->visible){
		sunh5_ui_layer_enable(mixer, layer->channel, layer->overlay, false, 0, old_zpos);
		return;
	}

	sunh5_ui_layer_update_coord(mixer, layer->channel, layer->overlay, plane, zpos);
	sunh5_ui_layer_update_formats(mixer, layer->channel, layer->overlay, plane);
	sunh5_ui_layer_update_buffer(mixer, layer->channel, layer->overlay, plane);
	sunh5_ui_layer_enable(mixer, layer->channel, layer->overlay, true, zpos, old_zpos);
}

static struct drm_plane_helper_funcs sunh5_ui_layer_helper_funcs = {
	.prepare_fb		= sunh5_ui_drm_fb_cma_prepare_fb,
	.atomic_check	= sunh5_ui_layer_atomic_check,
	.atomic_disable	= sunh5_ui_layer_atomic_disable,
	.atomic_update	= sunh5_ui_layer_atomic_update,
};

static int sunh5_ui_drm_atomic_helper_disable_plane(struct drm_plane *plane,
				    struct drm_modeset_acquire_ctx *ctx)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);

 	return drm_atomic_helper_disable_plane(plane, ctx);
}

static void sunh5_ui_drm_plane_cleanup(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	drm_plane_cleanup(plane);
}

static void sunh5_ui_drm_atomic_helper_plane_reset(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	drm_atomic_helper_plane_reset(plane);
}

static struct drm_plane_state *sunh5_ui_drm_atomic_helper_plane_duplicate_state(struct drm_plane *plane)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_plane_duplicate_state(plane);
}

static void sunh5_ui_drm_atomic_helper_plane_destroy_state(struct drm_plane *plane,
					   struct drm_plane_state *state)
{
	dev_dbg(plane->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_plane_destroy_state(plane, state);
}

static int sunh5_ui_drm_atomic_helper_update_plane(struct drm_plane *plane,
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

static const struct drm_plane_funcs sunh5_ui_layer_funcs = {
	.atomic_destroy_state	= sunh5_ui_drm_atomic_helper_plane_destroy_state,
	.atomic_duplicate_state	= sunh5_ui_drm_atomic_helper_plane_duplicate_state,
	.destroy				= sunh5_ui_drm_plane_cleanup,
	.disable_plane			= sunh5_ui_drm_atomic_helper_disable_plane,
	.reset					= sunh5_ui_drm_atomic_helper_plane_reset,
	.update_plane			= sunh5_ui_drm_atomic_helper_update_plane,
};

static const u32 sunh5_ui_layer_formats[] = {
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_BGRA5551,
	DRM_FORMAT_BGRA4444,
	DRM_FORMAT_BGRA8888,
	DRM_FORMAT_BGRX8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_XRGB8888,
};

struct sunh5_ui_layer *sunh5_ui_layer_init_one(struct drm_device *drm, struct sunh5_mixer *mixer, int index)
{
	enum drm_plane_type type = DRM_PLANE_TYPE_OVERLAY;
	int channel = mixer->cfg->vi_num + index;
	struct sunh5_ui_layer *layer;
	unsigned int plane_cnt;
	int ret;

	dev_dbg(drm->dev, "%s: Enter\n", __func__);

	layer = devm_kzalloc(drm->dev, sizeof(*layer), GFP_KERNEL);
	if(!layer)
		return ERR_PTR(-ENOMEM);
	
	if(index == 0)
		type = DRM_PLANE_TYPE_PRIMARY;

	/* possible crtcs are set later */
	ret = drm_universal_plane_init(drm, &layer->plane, 0,
									&sunh5_ui_layer_funcs,
									sunh5_ui_layer_formats,
									ARRAY_SIZE(sunh5_ui_layer_formats),
									NULL, type, NULL);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't initialize layer\n", __func__);
		return ERR_PTR(ret);
	}

	plane_cnt = mixer->cfg->ui_num + mixer->cfg->vi_num;

	ret = drm_plane_create_zpos_property(&layer->plane, channel, 0, plane_cnt - 1);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't add zpos property\n", __func__);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(&layer->plane, &sunh5_ui_layer_helper_funcs);
	layer->mixer = mixer;
	layer->channel = channel;
	layer->overlay = 0;

	return layer;
}
