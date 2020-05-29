#ifndef __SUNH5_CRTC_H__
#define __SUNH5_CRTC_H__

#include <drm/drm_crtc.h>

struct sunh5_crtc {
	struct drm_crtc						crtc;
	struct drm_pending_vblank_event		*event;

	struct sunh5_engine					*engine;
	struct sunh5_tcon					*tcon;
};



static inline struct sunh5_crtc *drm_crtc_to_sunh5_crtc(struct drm_crtc *crtc)
{
	return container_of(crtc, struct sunh5_crtc, crtc);
}

struct sunh5_crtc *sunh5_crtc_init(struct drm_device *drm, struct sunh5_engine *engine, struct sunh5_tcon *tcon);



#endif	/* __SUNH5_CRTC_H__ */