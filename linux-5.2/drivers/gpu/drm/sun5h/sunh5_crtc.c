#include <linux/clk-provider.h>
#include <linux/ioport.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_modes.h>
#include <drm/drm_probe_helper.h>

#include "sunh5_engine.h"
#include "sunh5_tcon.h"
#include "sunh5_drv.h"
#include "sunh5_crtc.h"


/*
 * While this isn't really working in the DRM theory, in practice we
 * can only ever have one encoder per TCON since we have a mux in our
 * TCON.
 */
static struct drm_encoder *sunh5_crtc_get_encoder(struct drm_crtc *crtc)
{
	struct drm_encoder *encoder;

	drm_for_each_encoder(encoder, crtc->dev)
		if(encoder->crtc == crtc)
			return encoder;

	return NULL;
}

static int sunh5_crtc_atomic_check(struct drm_crtc *crtc, struct drm_crtc_state *state)
{
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);
	struct sunh5_engine *engine = scrtc->engine;
	int ret = 0;

	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);

	if(engine && engine->ops && engine->ops->atomic_check)
		ret = engine->ops->atomic_check(engine, state);

	return ret;
}

static void sunh5_crtc_atomic_begin(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);
	struct drm_device *dev = crtc->dev;
//	struct sunh5_engine *engine = scrtc->engine;
	unsigned long flags;

	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);

	if(crtc->state->event){
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);		/* WARN_ON():调用dump_stack,打印堆栈信息,不会oops */

		spin_lock_irqsave(&dev->event_lock, flags);
		scrtc->event = crtc->state->event;
		spin_unlock_irqrestore(&dev->event_lock, flags);
		crtc->state->event = NULL;
	}
}

static void sunh5_crtc_atomic_flush(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);
	struct drm_pending_vblank_event *event = crtc->state->event;

	dev_dbg(crtc->dev->dev, "%s: Committing plane changes\n", __func__);

	sunh5_engine_commit(scrtc->engine);

	if(event){
		crtc->state->event = NULL;

		spin_lock_irq(&crtc->dev->event_lock);
		if(drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, event);
		else
			drm_crtc_send_vblank_event(crtc, event);
		spin_unlock_irq(&crtc->dev->event_lock);
	}
}

static void sunh5_crtc_atomic_enable(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
	struct drm_encoder *encoder = sunh5_crtc_get_encoder(crtc);
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);

	dev_dbg(crtc->dev->dev, "%s: Enabling the CRTC\n", __func__);

	sunh5_tcon_set_status(scrtc->tcon, encoder, true);

	drm_crtc_vblank_on(crtc);
}

static void sunh5_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct drm_display_mode *mode = &crtc->state->adjusted_mode;
	struct drm_encoder *encoder = sunh5_crtc_get_encoder(crtc);
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);

	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);

	sunh5_tcon_mode_set(scrtc->tcon, encoder, mode);
}

static const struct drm_crtc_helper_funcs sunh5_crtc_helper_funcs = {
	.atomic_check	= sunh5_crtc_atomic_check,
	.atomic_begin	= sunh5_crtc_atomic_begin,
	.atomic_flush	= sunh5_crtc_atomic_flush,
	.atomic_enable	= sunh5_crtc_atomic_enable,
	.mode_set_nofb	= sunh5_crtc_mode_set_nofb,
};

static int sunh5_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);
	
	dev_dbg(crtc->dev->dev, "%s: Enter, enabling VBLANK on crtc %p\n", __func__, crtc);

	sunh5_tcon_enable_vblank(scrtc->tcon, true);

	return 0;
}

static void sunh5_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct sunh5_crtc *scrtc = drm_crtc_to_sunh5_crtc(crtc);
	
	dev_dbg(crtc->dev->dev, "%s: Enter, Disabling VBLANK on crtc %p\n", __func__, crtc);

	sunh5_tcon_enable_vblank(scrtc->tcon, false);
}

static void sunh5_crtc_atomic_helper_crtc_reset(struct drm_crtc *crtc)
{
	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);
	drm_atomic_helper_crtc_reset(crtc);
}

static void sunh5_crtc_cleanup(struct drm_crtc *crtc)
{
	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);
	drm_crtc_cleanup(crtc);
}

static int sunh5_crtc_atomic_helper_set_config(struct drm_mode_set *set,
				 struct drm_modeset_acquire_ctx *ctx)

{
	dev_dbg(set->crtc->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_set_config(set,ctx)
;
}

static int sunh5_crtc_atomic_helper_page_flip(struct drm_crtc *crtc,
				struct drm_framebuffer *fb,
				struct drm_pending_vblank_event *event,
				uint32_t flags,
				struct drm_modeset_acquire_ctx *ctx)
{
	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_page_flip(crtc,
				fb,
				event,
				flags,
				ctx);
}

static struct drm_crtc_state *sunh5_crtc_atomic_helper_crtc_duplicate_state(struct drm_crtc *crtc)
{
	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);
	return drm_atomic_helper_crtc_duplicate_state(crtc);
}

static void sunh5_crtc_atomic_helper_crtc_destroy_state(struct drm_crtc *crtc,
					  struct drm_crtc_state *state)
{
	dev_dbg(crtc->dev->dev, "%s: Enter\n", __func__);

	drm_atomic_helper_crtc_destroy_state(crtc,state);
}
				

static const struct drm_crtc_funcs sunh5_crtc_funcs = {
	.reset					= sunh5_crtc_atomic_helper_crtc_reset,
	.destroy				= sunh5_crtc_cleanup,
	.set_config				= sunh5_crtc_atomic_helper_set_config,
	.page_flip				= sunh5_crtc_atomic_helper_page_flip,
	.atomic_duplicate_state	= sunh5_crtc_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= sunh5_crtc_atomic_helper_crtc_destroy_state,
	.enable_vblank			= sunh5_crtc_enable_vblank,
	.disable_vblank			= sunh5_crtc_disable_vblank,
};


struct sunh5_crtc *sunh5_crtc_init(struct drm_device *drm, struct sunh5_engine *engine, struct sunh5_tcon *tcon)
{
	struct sunh5_crtc *scrtc;
	struct drm_plane **planes;
	struct drm_plane *primary = NULL, *cursor = NULL;
	int ret, i;

	dev_dbg(drm->dev, "%s: Enter\n", __func__);

	scrtc = devm_kzalloc(drm->dev, sizeof(*scrtc), GFP_KERNEL);
	if(!scrtc)
		return ERR_PTR(-ENOMEM);

	scrtc->engine = engine;
	scrtc->tcon = tcon;

	/* Create our layers */
	planes = sunh5_engine_layers_init(drm, engine);
	if(IS_ERR(planes)){
		dev_err(drm->dev, "%s: Couldn't create the planes\n", __func__);
		return NULL;
	}

	/* find primary and cursor planes for drm_crtc_init_with_planes */
	for(i = 0; planes[i]; i++){
		struct drm_plane *plane = planes[i];

		switch(plane->type){
			case DRM_PLANE_TYPE_PRIMARY:
				primary = plane;
			break;
			case DRM_PLANE_TYPE_CURSOR:
				cursor = plane;
			break;
			default:
			break;
		}
	}

	ret = drm_crtc_init_with_planes(drm, &scrtc->crtc, primary, cursor, &sunh5_crtc_funcs, NULL);
	if(ret){
		dev_err(drm->dev, "%s: Couldn't init DRM CRTC\n", __func__);
		return ERR_PTR(ret);
	}

	drm_crtc_helper_add(&scrtc->crtc, &sunh5_crtc_helper_funcs);

	/* Set crtc.port to output port node of the tcon */
	scrtc->crtc.port = of_graph_get_port_by_id(scrtc->tcon->dev->of_node, 1);

	/* Set possible_crtcs to this crtc for overlay planes */
	for(i = 0; planes[i]; i++){
		uint32_t possible_crtcs = drm_crtc_mask(&scrtc->crtc);
		struct drm_plane *plane = planes[i];

		if(plane->type == DRM_PLANE_TYPE_OVERLAY)
			plane->possible_crtcs = possible_crtcs;
	}
	
	return scrtc;
}
