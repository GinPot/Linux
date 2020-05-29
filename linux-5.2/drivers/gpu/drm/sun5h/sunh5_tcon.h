#ifndef __SUNH5_TCON_H__
#define __SUNH5_TCON_H__



#define SUNH5_TCON_GCTL_REG								0x0
#define SUNH5_TCON_GCTL_TCON_ENABLE						BIT(31)
#define SUNH5_TCON_GCTL_IOMAP_MASK						BIT(0)
#define SUNH5_TCON_GCTL_IOMAP_TCON1						(1 << 0)



#define SUNH5_TCON_GINT0_REG							0x4
#define SUNH5_TCON_GINT0_VBLANK_ENABLE(pipe)			BIT(31 - (pipe))
#define SUNH5_TCON_GINT0_TCON0_TRI_FINISH_ENABLE		BIT(27)
#define SUNH5_TCON_GINT0_VBLANK_INT(pipe)				BIT(15 - (pipe))
#define SUNH5_TCON_GINT0_TCON0_TRI_FINISH_INT			BIT(11)



#define SUNH5_TCON_GINT1_REG							0x8





#define SUNH5_TCON0_IO_TRI_REG							0x8c
#define SUNH5_TCON1_IO_TRI_REG							0xf4



#define SUNH5_TCON1_CTL_REG								0x90
#define SUNH5_TCON1_CTL_TCON_ENABLE						BIT(31)
#define SUNH5_TCON1_CTL_INTERLACE_ENABLE				BIT(20)
#define SUNH5_TCON1_CTL_CLK_DELAY_MASK					GENMASK(8, 4)
#define SUNH5_TCON1_CTL_CLK_DELAY(delay)				((delay << 4) & SUNH5_TCON1_CTL_CLK_DELAY_MASK)

#define SUNH5_TCON1_BASIC0_REG							0x94
#define SUNH5_TCON1_BASIC0_X(width)						((((width) - 1) & 0xfff) << 16)
#define SUNH5_TCON1_BASIC0_Y(height)					(((height) - 1) & 0xfff)

#define SUNH5_TCON1_BASIC1_REG							0x98
#define SUNH5_TCON1_BASIC1_X(width)						((((width) - 1) & 0xfff) << 16)
#define SUNH5_TCON1_BASIC1_Y(height)					(((height) - 1) & 0xfff)

#define SUNH5_TCON1_BASIC2_REG							0x9c
#define SUNH5_TCON1_BASIC2_X(width)						((((width) - 1) & 0xfff) << 16)
#define SUNH5_TCON1_BASIC2_Y(height)					(((height) - 1) & 0xfff)

#define SUNH5_TCON1_BASIC3_REG							0xa0
#define SUNH5_TCON1_BASIC3_H_TOTAL(total)				((((total) - 1) & 0x1fff) << 16)
#define SUNH5_TCON1_BASIC3_H_BACKPORCH(bp)				(((bp) - 1) & 0xfff)

#define SUNH5_TCON1_BASIC4_REG							0xa4
#define SUNH5_TCON1_BASIC4_V_TOTAL(total)				(((total) & 0x1fff) << 16)
#define SUNH5_TCON1_BASIC4_V_BACKPORCH(bp)				(((bp) - 1) & 0xfff)

#define SUNH5_TCON1_BASIC5_REG							0xa8
#define SUNH5_TCON1_BASIC5_H_SYNC(width)				((((width) - 1) & 0x3ff) << 16)
#define SUNH5_TCON1_BASIC5_V_SYNC(height)				(((height) - 1) & 0x3ff)





struct sunh5_tcon_quirks {
	bool has_channel_1;
};

struct sunh5_tcon {
	struct device					*dev;
	struct drm_device				*drm;
	struct regmap					*regs;

	/* Main bus clock */
	struct clk						*clk;

	/* Clock fir the TCON channels */
	struct clk						*sclk1;

	/* Reset control */
	struct reset_control			*lcd_rst;

	/* Platform adjustments */
	const struct sunh5_tcon_quirks	*quirks;

	int 							id;

	/* Associated crtc */
	struct sunh5_crtc				*crtc;

	/* TCON list management */
	struct list_head				list;
};




void sunh5_tcon_enable_vblank(struct sunh5_tcon *tcon, bool enable);
void sunh5_tcon_mode_set(struct sunh5_tcon *tcon, const struct drm_encoder *encoder, const struct drm_display_mode *mode);
void sunh5_tcon_set_status(struct sunh5_tcon *crtc, const struct drm_encoder *encoder, bool enable);



#endif	/* __SUNH5_TCON_H__ */
