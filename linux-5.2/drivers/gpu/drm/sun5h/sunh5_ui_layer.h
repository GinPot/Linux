#ifndef __SUNH5_UI_LAYER_H__
#define __SUNH5_UI_LAYER_H__

#include <drm/drm_plane.h>

struct sunh5_mixer;



#define SUNH5_MIXER_CHAN_UI_LAYER_ATTR(base, layer)			((base) + 0x20 * (layer) + 0x0)
#define SUNH5_MIXER_CHAN_UI_LAYER_SIZE(base, layer)			((base) + 0x20 * (layer) + 0x4)
#define SUNH5_MIXER_CHAN_UI_LAYER_PITCH(base, layer)			((base) + 0x20 * (layer) + 0xc)
#define SUNH5_MIXER_CHAN_UI_LAYER_TOP_LADDR(base, layer)		((base) + 0x20 * (layer) + 0x10)
#define SUNH5_MIXER_CHAN_UI_OVL_SIZE(base)					((base) + 0x88)

#define SUNH5_MIXER_CHAN_UI_LAYER_ATTR_EN					BIT(0)

#define SUNH5_MIXER_CHAN_UI_LAYER_ATTR_FBFMT_MASK			GENMASK(12, 8)
#define SUNH5_MIXER_CHAN_UI_LAYER_ATTR_FBFMT_OFFSET			8

struct sunh5_ui_layer {
	struct drm_plane	plane;
	struct sunh5_mixer	*mixer;
	int 				channel;
	int 				overlay;
};



static inline struct sunh5_ui_layer *plane_to_sunh5_ui_layer(struct drm_plane *plane)
{
	return container_of(plane, struct sunh5_ui_layer, plane);
}

struct sunh5_ui_layer *sunh5_ui_layer_init_one(struct drm_device *drm, struct sunh5_mixer *mixer, int index);


#endif	/* __SUNH5_UI_LAYER_H__ */
