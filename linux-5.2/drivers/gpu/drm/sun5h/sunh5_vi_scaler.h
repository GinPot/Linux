#ifndef	__SUNH5_VI_SCALER_H__
#define	__SUNH5_VI_SCALER_H__

#include <drm/drm_fourcc.h>
#include "sunh5_mixer.h"


#define DE2_VI_SCALER_UNIT_BASE					0x20000
#define DE2_VI_SCALER_UNIT_SIZE					0x20000




/* this two macros assumes 16 fractional bits which is standard in DRM */
#define SUNH5_VI_SCALER_SCALE_MIN				1
#define SUNH5_VI_SCALER_SCALE_MAX				((1UL << 20) - 1)

#define SUNH5_VI_SCALER_SCALE_FRAC				20
#define SUNH5_VI_SCALER_PHASE_FRAC				20
#define SUNH5_VI_SCALER_COEFF_COUNT				32
#define SUNH5_VI_SCALER_SIZE(w, h)				(((h) - 1) << 16 | ((w) -1 ))



#define SUNH5_SCALER_VSU_CRTL(base)				((base) + 0x0)
#define SUNH5_SCALER_VSU_CRTL_EN				BIT(0)
#define SUNH5_SCALER_VSU_CTRL_COEFF_RDY			BIT(4)


#define SUNH5_SCALER_VSU_OUTSIZE(base)			((base) + 0x40)
#define SUNH5_SCALER_VSU_YINSIZE(base)			((base) + 0x80)
#define SUNH5_SCALER_VSU_YHSTEP(base)			((base) + 0x88)
#define SUNH5_SCALER_VSU_YVSTEP(base)			((base) + 0x8c)
#define SUNH5_SCALER_VSU_YHPHASE(base)			((base) + 0x90)
#define SUNH5_SCALER_VSU_YVPHASE(base)			((base) + 0x98)
#define SUNH5_SCALER_VSU_CINSIZE(base)			((base) + 0xc0)
#define SUNH5_SCALER_VSU_CHSTEP(base)			((base) + 0xc8)
#define SUNH5_SCALER_VSU_CVSTEP(base)			((base) + 0xcc)
#define SUNH5_SCALER_VSU_CHPHASE(base)			((base) + 0xd0)
#define SUNH5_SCALER_VSU_CVPHASE(base)			((base) + 0xd8)
#define SUNH5_SCALER_VSU_YHCOEFF0(base, i)		((base) + 0x200 + 0x4 * (i))
#define SUNH5_SCALER_VSU_YHCOEFF1(base, i)		((base) + 0x300 + 0x4 * (i))
#define SUNH5_SCALER_VSU_YVCOEFF(base, i)		((base) + 0x400 + 0x4 * (i))
#define SUNH5_SCALER_VSU_CHCOEFF0(base, i)		((base) + 0x600 + 0x4 * (i))
#define SUNH5_SCALER_VSU_CHCOEFF1(base, i)		((base) + 0x700 + 0x4 * (i))
#define SUNH5_SCALER_VSU_CVCOEFF(base, i)		((base) + 0x800 + 0x4 * (i))



void sunh5_vi_scaler_enable(struct sunh5_mixer *mixer, int layer, bool enable);
void sunh5_vi_scaler_setup(struct sunh5_mixer *mixer, int layer,
								u32 scr_w, u32 src_h, u32 dst_w, u32 dst_h,
								u32 hscale, u32 vscale, u32 hphase, u32 vphase,
								const struct drm_format_info *format);


#endif	/* __SUNH5_VI_SCALER_H__ */
