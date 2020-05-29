#ifndef	__SUNH5_VI_LAYER_H__
#define	__SUNH5_VI_LAYER_H__

#include <drm/drm_plane.h>
#include "sunh5_mixer.h"



#define SUNH5_MIXER_CHAN_VI_LAYER_ATTR(base, layer)				((base) + 0x30 * (layer) + 0x0)
#define SUNH5_MIXER_CHAN_VI_LAYER_SIZE(base, layer)				((base) + 0x30 * (layer) + 0x4)

#define SUNH5_MIXER_CHAN_VI_LAYER_PITCH(base, layer, plane)		((base) + 0x30 * (layer) + 0xc + 4 * (plane))
#define SUNH5_MIXER_CHAN_VI_LAYER_TOP_LADDR(base, layer, plane)	((base) + 0x30 * (layer) + 0x18 + 4 * (plane))

#define SUNH5_MIXER_CHAN_VI_OVL_SIZE(base)						((base) + 0xe8)
#define SUNH5_MIXER_CHAN_VI_HDS_Y(base)							((base) + 0xf0)
#define SUNH5_MIXER_CHAN_VI_HDS_UV(base)						((base) + 0xf4)
#define SUNH5_MIXER_CHAN_VI_VDS_Y(base)							((base) + 0xf8)
#define SUNH5_MIXER_CHAN_VI_VDS_UV(base)						((base) + 0xfc)




#define SUNH5_MIXER_CHAN_VI_LAYER_ATTR_EN						BIT(0)

/* RGB mode should be set for RGB formats and cleared for YCbCr */
#define SUNH5_MIXER_CHAN_VI_LAYER_ATTR_RGB_MODE					BIT(15)
#define SUNH5_MIXER_CHAN_VI_LAYER_ATTR_FBFMT_OFFSET				8
#define SUNH5_MIXER_CHAN_VI_LAYER_ATTR_FBFMT_MASK				GENMASK(12, 8)


#define SUNH5_MIXER_CHAN_VI_DS_N(x)								((x) << 16)
#define SUNH5_MIXER_CHAN_VI_DS_M(x)								((x) << 0)




struct sunh5_vi_layer {
	struct drm_plane	plane;
	struct sunh5_mixer	*mixer;
	int 				channel;
	int 				overlay;
};


static inline struct sunh5_vi_layer *plane_to_sunh5_vi_layer(struct drm_plane *plane)
{
	return container_of(plane, struct sunh5_vi_layer, plane);
}

struct sunh5_vi_layer *sunh5_vi_layer_init_one(struct drm_device *drm, struct sunh5_mixer *mixer, int index);



#endif	/* __SUNH5_VI_LAYER_H__ */