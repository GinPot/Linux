#ifndef __SUNH5_DW_HDMI_H__
#define __SUNH5_DW_HDMI_H__

#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/regulator/consumer.h>

#include <drm/bridge/dw_hdmi.h>
#include <drm/drm_encoder.h>

#define SUNH5_HDMI_PHY_DBG_CTRL_REG				0x0000
#define SUNH5_HDMI_PHY_DBG_CTRL_POL_MASK		GENMASK(15, 8)
#define SUNH5_HDMI_PHY_DBG_CTRL_POL_NHSYNC		BIT(8)
#define SUNH5_HDMI_PHY_DBG_CTRL_POL_NVSYNC		BIT(9)

#define SUNH5_HDMI_PHY_READ_EN_REG				0x0010
#define SUNH5_HDMI_PHY_READ_EN_MAGIC			0x54524545

#define SUNH5_HDMI_PHY_UNSCRAMBLE_REG			0x0014
#define SUNH5_HDMI_PHY_UNSCRAMBLE_MAGIC			0x42494E47

#define SUNH5_HDMI_PHY_ANA_CFG1_REG				0x0020
#define SUNH5_HDMI_PHY_ANA_CFG1_REG_CALSW		BIT(28)
#define SUNH5_HDMI_PHY_ANA_CFG1_REG_SVBH(x)		((x) << 24)
#define SUNH5_HDMI_PHY_ANA_CFG1_AMP_OPT			BIT(23)
#define SUNH5_HDMI_PHY_ANA_CFG1_EMP_OPT			BIT(22)
#define SUNH5_HDMI_PHY_ANA_CFG1_AMPCK_OPT		BIT(21)
#define SUNH5_HDMI_PHY_ANA_CFG1_EMPCK_OPT		BIT(20)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENRCAL			BIT(19)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENCALOG			BIT(18)
#define SUNH5_HDMI_PHY_ANA_CFG1_REG_SCKTMDS		BIT(17)
#define SUNH5_HDMI_PHY_ANA_CFG1_TMDSCLK_EN		BIT(16)
#define SUNH5_HDMI_PHY_ANA_CFG1_TXEN_MASK		GENMASK(15, 12)
#define SUNH5_HDMI_PHY_ANA_CFG1_TXEN_ALL		(0xf << 12)
#define SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDSCLK	BIT(11)
#define SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS2	BIT(10)
#define SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS1	BIT(9)
#define SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS0	BIT(8)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDSCLK	BIT(7)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS2		BIT(6)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS1		BIT(5)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS0		BIT(4)
#define SUNH5_HDMI_PHY_ANA_CFG1_CKEN			BIT(3)
#define SUNH5_HDMI_PHY_ANA_CFG1_LDOEN			BIT(2)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENVBS			BIT(1)
#define SUNH5_HDMI_PHY_ANA_CFG1_ENBI			BIT(0)


#define SUNH5_HDMI_PHY_ANA_CFG2_REG				0x0024
#define SUNH5_HDMI_PHY_ANA_CFG2_M_EN			BIT(31)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_DENCK		BIT(22)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_DEN			BIT(21)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_CKSS(x)		((x) << 17)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSWCK		BIT(16)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSW		BIT(15)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_CSMPS(x)	((x) << 13)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_SLV(x)		((x) << 10)
#define SUNH5_HDMI_PHY_ANA_CFG2_REG_RESDI(x)	((x) << 0)


#define SUNH5_HDMI_PHY_ANA_CFG3_REG				0x0028
#define SUNH5_HDMI_PHY_ANA_CFG3_REG_WIRE(x)		((x) << 18)
#define SUNH5_HDMI_PHY_ANA_CFG3_REG_AMPCK(x)	((x) << 14)
#define SUNH5_HDMI_PHY_ANA_CFG3_REG_AMP(x)		((x) << 7)
#define SUNH5_HDMI_PHY_ANA_CFG3_REG_EMP(x)		((x) << 4)
#define SUNH5_HDMI_PHY_ANA_CFG3_SDAEN			BIT(2)
#define SUNH5_HDMI_PHY_ANA_CFG3_SCLEN			BIT(0)


#define SUNH5_HDMI_PHY_PLL_CFG1_REG				0x002c
#define SUNH5_HDMI_PHY_PLL_CFG1_REG_OD1			BIT(31)
#define SUNH5_HDMI_PHY_PLL_CFG1_REG_OD			BIT(30)
#define SUNH5_HDMI_PHY_PLL_CFG1_LDO2_EN			BIT(29)
#define SUNH5_HDMI_PHY_PLL_CFG1_LDO1_EN			BIT(28)
#define SUNH5_HDMI_PHY_PLL_CFG1_HV_IS_33		BIT(27)
#define SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_MSK	BIT(26)
#define SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_SHIFT	26
#define SUNH5_HDMI_PHY_PLL_CFG1_PLLEN			BIT(25)
#define SUNH5_HDMI_PHY_PLL_CFG1_LDO_VSET(x)		((x) << 22)
#define SUNH5_HDMI_PHY_PLL_CFG1_UNKNOWN(x)		((x) << 20)
#define SUNH5_HDMI_PHY_PLL_CFG1_PLLDBEN			BIT(19)
#define SUNH5_HDMI_PHY_PLL_CFG1_CS				BIT(18)
#define SUNH5_HDMI_PHY_PLL_CFG1_CP_S(x)			((x) << 13)
#define SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(x)		((x) << 7)
#define SUNH5_HDMI_PHY_PLL_CFG1_BWS				BIT(6)
#define SUNH5_HDMI_PHY_PLL_CFG1_B_IN_MSK		GENMASK(5, 0)
#define SUNH5_HDMI_PHY_PLL_CFG1_B_IN_SHIFT		0

#define SUNH5_HDMI_PHY_PLL_CFG2_REG				0x0030
#define SUNH5_HDMI_PHY_PLL_CFG2_SV_H			BIT(31)
#define SUNH5_HDMI_PHY_PLL_CFG2_VCOGAIN_EN		BIT(19)
#define SUNH5_HDMI_PHY_PLL_CFG2_VCO_S(x)		((x) << 12)
#define SUNH5_HDMI_PHY_PLL_CFG2_SDIV2			BIT(9)
#define SUNH5_HDMI_PHY_PLL_CFG2_S(x)			((x) << 6)
#define SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_MSK		GENMASK(3, 0)
#define SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_SHIFT	0
#define SUNH5_HDMI_PHY_PLL_CFG2_PREDIV(x)		(((x) - 1) << 0)

#define SUNH5_HDMI_PHY_PLL_CFG3_REG				0x0034
#define SUNH5_HDMI_PHY_PLL_CFG3_SOUT_DIV2		BIT(0)

#define SUNH5_HDMI_PHY_ANA_STS_REG				0x0038
#define SUNH5_HDMI_PHY_ANA_STS_B_OUT_MSK		GENMASK(16, 11)
#define SUNH5_HDMI_PHY_ANA_STS_B_OUT_SHIFT		11
#define SUNH5_HDMI_PHY_ANA_STS_RCALEND2D		BIT(7)
#define SUNH5_HDMI_PHY_ANA_STS_RCAL_MASK		GENMASK(5, 0)


#define SUNH5_HDMI_PHY_CEC_REG					0x003c


struct sunh5_hdmi_phy;


struct sunh5_dw_hdmi_quirks{
	enum drm_mode_status (*mode_valid)(struct drm_connector *connector, const struct drm_display_mode *mode);
	unsigned int set_rate : 1;
};

struct sunh5_hdmi_phy_variant {
	bool has_phy_clk;
	bool has_second_pll;
	unsigned int is_custom_phy : 1;
	
	const struct dw_hdmi_curr_ctrl		*cur_ctr;
	const struct dw_hdmi_mpll_config	*mpl1_cfg;
	const struct dw_hdmi_phy_config		*phy_cfg;

	void (*phy_init)(struct sunh5_hdmi_phy *phy);
	void (*phy_disable)(struct dw_hdmi *hdmi, struct sunh5_hdmi_phy *phy);
	int (*phy_config)(struct dw_hdmi *hdmi, struct sunh5_hdmi_phy *phy, unsigned int clk_rate);
};

struct sunh5_hdmi_phy{
	struct device						*dev;
	struct clk							*clk_bus;
	struct clk							*clk_mod;
	struct clk							*clk_phy;
	struct clk							*clk_pll0;
	struct clk							*clk_pll1;
	unsigned int						rcal;
	struct regmap						*regs;
	struct reset_control				*rst_phy;
	struct sunh5_hdmi_phy_variant		*variant;
};

struct sunh5_dw_hdmi {
	struct clk							*clk_tmds;
	struct device						*dev;
	struct dw_hdmi						*hdmi;
	struct drm_encoder					encoder;
	struct sunh5_hdmi_phy				*phy;
	struct dw_hdmi_plat_data 			plat_data;
	struct regulator					*regulator;
	const struct sunh5_dw_hdmi_quirks	*quirks;
	struct reset_control				*rst_ctrl;
};





static inline struct sunh5_dw_hdmi *encoder_to_sunh5_dw_hdmi(struct drm_encoder *encoder)
{
	return container_of(encoder, struct sunh5_dw_hdmi, encoder);
}

int sunh5_hdmi_phy_probe(struct sunh5_dw_hdmi *hdmi, struct device_node *node);
void sunh5_hdmi_phy_remove(struct sunh5_dw_hdmi *hdmi);
void sunh5_hdmi_phy_init(struct sunh5_hdmi_phy *phy);
void sunh5_hdmi_phy_set_ops(struct sunh5_hdmi_phy *phy, struct dw_hdmi_plat_data *plat_data);

int sunh5_phy_clk_create(struct sunh5_hdmi_phy *phy, struct device *dev, bool second_parent);

#endif	/* __SUNH5_DW_HDMI_H__ */
