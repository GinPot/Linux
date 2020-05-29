#ifndef __SUNH5_MIXER_H__
#define __SUNH5_MIXER_H__

#include <linux/regmap.h>

#include "sunh5_engine.h"
#include "sunh5_csc.h"


#define SUNH5_MIXER_SIZE(w, h)						(((h) - 1) << 16 | ((w) - 1))
#define SUNH5_MIXER_COORD(x, y)						((y) << 16 | (x))


#define SUNH5_MIXER_GLOBAL_CTL						0x0
#define SUNH5_MIXER_GLOBAL_CTL_RT_EN				BIT(0)

#define SUNH5_MIXER_GLOBAL_DBUFF					0x8
#define SUNH5_MIXER_GLOBAL_DBUFF_ENABLE				BIT(0)

#define SUNH5_MIXER_GLOBAL_SIZE						0xc

#define DE2_MIXER_UNIT_SIZE							0x6000

#define DE2_BLD_BASE								0x1000
#define DE2_CH_BASE									0x2000
#define DE2_CH_SIZE									0x1000

#define DE3_BLD_BASE								0x0800
#define DE3_CH_BASE									0x1000
#define DE3_CH_SIZE									0X0800


#define SUNH5_MIXER_BLEND_PIPE_CTL(base)			((base) + 0)
#define SUNH5_MIXER_BLEND_PIPE_CTL_EN_MSK			GENMASK(12, 8)
#define SUNH5_MIXER_BLEND_PIPE_CTL_EN(pipe)			BIT(8 + pipe)
#define SUNH5_MIXER_BLEND_PIPE_CTL_FC_EN(pipe)		BIT(pipe)

#define SUNH5_MIXER_BLEND_ATTR_FCOLOR(base, x)		((base) + 0x4 + 0x10 * (x))
#define SUNH5_MIXER_BLEND_ATTR_INSIZE(base, x)		((base) + 0x8 + 0x10 * (x))
#define SUNH5_MIXER_BLEND_ATTR_COORD(base, x)		((base) + 0xc + 0x10 * (x))

#define SUNH5_MIXER_BLEND_ROUTE(base) 				((base) + 0x80)
#define SUNH5_MIXER_BLEND_ROUTE_PIPE_MSK(n)			(0xf << ((n) << 2))	
#define SUNH5_MIXER_BLEND_ROUTE_PIPE_SHIFT(n)		((n) << 2)

#define SUNH5_MIXER_BLEND_BKCOLOR(base)				((base) + 0x88)
#define SUNH5_MIXER_BLEND_COLOR_BLACK				0xff000000			/* colors are always in AARRGGBB format */

#define SUNH5_MIXER_BLEND_OUTSIZE(base)				((base) + 0x8c)

#define SUNH5_MIXER_BLEND_MODE(base, x)				((base) + 0x90 + 0x04 * (x))
#define SUNH5_MIXER_BLEND_MODE_DEF					0x03010301			/* The numbers are some still unknown magic numbers */

#define SUNH5_MIXER_BLEND_OUTCTL(base)				((base) + 0xfc)
#define SUNH5_MIXER_BLEND_OUTCTL_INTERLACED			BIT(1)


#define SUNH5_MIXER_FBFMT_ARGB8888					0
#define SUNH5_MIXER_FBFMT_ABGR8888					1
#define SUNH5_MIXER_FBFMT_RGBA8888					2
#define SUNH5_MIXER_FBFMT_BGRA8888					3
#define SUNH5_MIXER_FBFMT_XRGB8888					4
#define SUNH5_MIXER_FBFMT_XBGR8888					5
#define SUNH5_MIXER_FBFMT_RGBX8888					6
#define SUNH5_MIXER_FBFMT_BGRX8888					7
#define SUNH5_MIXER_FBFMT_RGB888					8
#define SUNH5_MIXER_FBFMT_BGR888					9
#define SUNH5_MIXER_FBFMT_RGB565					10
#define SUNH5_MIXER_FBFMT_BGR565					11
#define SUNH5_MIXER_FBFMT_ARGB4444					12
#define SUNH5_MIXER_FBFMT_ABGR4444					13
#define SUNH5_MIXER_FBFMT_RGBA4444					14
#define SUNH5_MIXER_FBFMT_BGRA4444					15
#define SUNH5_MIXER_FBFMT_ARGB1555					16
#define SUNH5_MIXER_FBFMT_ABGR1555					17
#define SUNH5_MIXER_FBFMT_RGBA5551					18
#define SUNH5_MIXER_FBFMT_BGRA5551					19

#define SUNH5_MIXER_FBFMT_YUYV						0
#define SUNH5_MIXER_FBFMT_UYVY						1
#define SUNH5_MIXER_FBFMT_YVYU						2
#define SUNH5_MIXER_FBFMT_VYUY						3
#define SUNH5_MIXER_FBFMT_NV16						4
#define SUNH5_MIXER_FBFMT_NV61						5
#define SUNH5_MIXER_FBFMT_YUV422					6
/* format 7 doesn't exist */
#define SUNH5_MIXER_FBFMT_NV12						8
#define SUNH5_MIXER_FBFMT_NV21						9
#define SUNH5_MIXER_FBFMT_YUV420					10
/* format 11 doesn't exist */
/* format 12 is semi-planar YUV411 UVUV */
/* format 13 is semi-planar YUV411 VUVU */
#define SUNH5_MIXER_FBFMT_YUV411					14

/*
 * Sub-engine listed bellow are unused for now. The EN registers are here only
 * to be used to disable these sub-engines
 */
#define SUNH5_MIXER_FCE_EN							0xa0000
#define SUNH5_MIXER_BWS_EN							0xa2000
#define SUNH5_MIXER_LTI_EN							0xa4000
#define SUNH5_MIXER_PEAK_EN							0xa6000
#define SUNH5_MIXER_ASE_EN							0xa8000
#define SUNH5_MIXER_FCC_EN							0xaa000
#define SUNH5_MIXER_DCSC_EN							0xb0000


struct de2_fmt_info {
	u32						drm_fmt;
	u32						de2_fmt;
	bool					rgb;
	enum sunh5_csc_mode		csc;
};

/**
 * struct sun8i_mixer_cfg - mixer HW configuration
 * @vi_num: number of VI channels
 * @ui_num: number of UI channels
 * @scaler_mask: bitmask which tells which channel supports scaling
 *	First, scaler supports for VI channels is defined and after that, scaler
 *	support for UI channels. For example, if mixer has 2 VI channels without
 *	scaler and 2 UI channels with scaler, bitmask would be 0xC.
 * @ccsc: select set of CCSC base addresses
 *	Set value to 0 if this is first mixer or second mixer with VEP support.
 *	Set value to 1 if this is second mixer without VEP support. Other values
 *	are invalid.
 * @mod_rate: module clock rate that needs to be set in order to have
 *	a functional block.
 * @is_de3: true, if this is next gen display engine 3.0, false otherwise.
 * @scaline_yuv: size of a scanline for VI scaler for YUV formats.
 */
 struct sunh5_mixer_cfg {
	int				vi_num;
	int				ui_num;
	int				scaler_mask;
	int				ccsc;
	unsigned long	mode_rate;
	unsigned int 	is_de3 : 1;
	unsigned int	scanline_yuv;
 };

struct sunh5_mixer {
	struct sunh5_engine				engine;

	const struct sunh5_mixer_cfg	*cfg;

	struct reset_control			*reset;

	struct clk						*bus_clk;
	struct clk						*mode_clk;
};



static inline struct sunh5_mixer *engine_to_sunh5_mixer(struct sunh5_engine *engine)
{
	return container_of(engine, struct sunh5_mixer, engine);
}

static inline u32 sunh5_blender_base(struct sunh5_mixer *mixer)
{
	return mixer->cfg->is_de3 ? DE3_BLD_BASE : DE2_BLD_BASE;
}

static inline u32 sunh5_channel_base(struct sunh5_mixer *mixer, int channel)
{
	if(mixer->cfg->is_de3)
		return DE3_CH_BASE + channel * DE3_CH_SIZE;
	else
		return DE2_CH_BASE + channel * DE2_CH_SIZE;
}


const struct de2_fmt_info *sunh5_mixer_format_info(u32 format);


#endif	/* __SUNH5_MIXER_H__ */
