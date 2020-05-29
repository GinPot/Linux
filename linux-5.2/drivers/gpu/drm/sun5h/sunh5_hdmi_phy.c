#include <linux/delay.h>
#include <linux/of_address.h>

#include "sunh5_dw_hdmi.h"



static int sunh5_hdmi_phy_config(struct dw_hdmi *hdmi, void *data, struct drm_display_mode *mode)
{
	struct sunh5_hdmi_phy *phy = (struct sunh5_hdmi_phy *)data;
	u32 val = 0;

	dev_dbg(phy->dev, "%s: Enter\n", __func__);

	if(mode->flags & DRM_MODE_FLAG_NHSYNC)
		val |= SUNH5_HDMI_PHY_DBG_CTRL_POL_NHSYNC;

	if(mode->flags & DRM_MODE_FLAG_NVSYNC)
		val |= SUNH5_HDMI_PHY_DBG_CTRL_POL_NVSYNC;

	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_DBG_CTRL_REG, SUNH5_HDMI_PHY_DBG_CTRL_POL_MASK, val);

	if(phy->variant->has_phy_clk)
		clk_set_rate(phy->clk_phy, mode->crtc_clock * 1000);

	return phy->variant->phy_config(hdmi, phy, mode->crtc_clock * 1000);
}

static void sunh5_hdmi_phy_disable(struct dw_hdmi *hdmi, void *data)
{
	struct sunh5_hdmi_phy *phy = (struct sunh5_hdmi_phy *)data;

	dev_dbg(phy->dev, "%s: Enter\n", __func__);

	phy->variant->phy_disable(hdmi, phy);
}

static enum drm_connector_status sunh5_hdmi_phy_read_hpd(struct dw_hdmi *hdmi,
					       void *data)
{
	struct sunh5_hdmi_phy *phy = (struct sunh5_hdmi_phy *)data;
	
	dev_dbg(phy->dev, "%s: Enter\n", __func__);

	return dw_hdmi_phy_read_hpd(hdmi, data);
}

static void sunh5_hdmi_phy_update_hpd(struct dw_hdmi *hdmi, void *data,
			    bool force, bool disabled, bool rxsense)
{
	struct sunh5_hdmi_phy *phy = (struct sunh5_hdmi_phy *)data;
							   
	dev_dbg(phy->dev, "%s: Enter\n", __func__);

 	dw_hdmi_phy_update_hpd(hdmi, data, force, disabled, rxsense);
}

static void sunh5_hdmi_phy_setup_hpd(struct dw_hdmi *hdmi, void *data)
{
	struct sunh5_hdmi_phy *phy = (struct sunh5_hdmi_phy *)data;
					
	dev_dbg(phy->dev, "%s: Enter\n", __func__);

	dw_hdmi_phy_setup_hpd( hdmi,  data);
}

static const struct dw_hdmi_phy_ops sunh5_hdmi_phy_ops = {
	.init		= &sunh5_hdmi_phy_config,
	.disable	= &sunh5_hdmi_phy_disable,
	.read_hpd	= &sunh5_hdmi_phy_read_hpd,
	.update_hpd	= &sunh5_hdmi_phy_update_hpd,
	.setup_hpd	= &sunh5_hdmi_phy_setup_hpd,
};

void sunh5_hdmi_phy_set_ops(struct sunh5_hdmi_phy *phy, struct dw_hdmi_plat_data *plat_data)
{
	struct sunh5_hdmi_phy_variant *variant = phy->variant;

	if(variant->is_custom_phy){
		plat_data->phy_ops = &sunh5_hdmi_phy_ops;
		plat_data->phy_name = "sunh5_dw_hdmi_phy";
		plat_data->phy_data = phy;
	} else {
		plat_data->mpll_cfg = variant->mpl1_cfg;
		plat_data->cur_ctr = variant->cur_ctr;
		plat_data->phy_config = variant->phy_cfg;
	}
}

void sunh5_hdmi_phy_init(struct sunh5_hdmi_phy *phy)
{
	phy->variant->phy_init(phy);
}

static void sunh5_hdmi_phy_unlock(struct sunh5_hdmi_phy *phy)
{
	/* enable read access to HDMI controller */
	regmap_write(phy->regs, SUNH5_HDMI_PHY_READ_EN_REG, SUNH5_HDMI_PHY_READ_EN_MAGIC);

	/* unscramble register offsets */
	regmap_write(phy->regs, SUNH5_HDMI_PHY_UNSCRAMBLE_REG, SUNH5_HDMI_PHY_UNSCRAMBLE_MAGIC);
}

static void sunh5_hdmi_phy_init_h3(struct sunh5_hdmi_phy *phy)
{
	unsigned int val;

	dev_dbg(phy->dev, "%s: Enter\n", __func__);

	sunh5_hdmi_phy_unlock(phy);

	regmap_write(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 0);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENBI,
									SUNH5_HDMI_PHY_ANA_CFG1_ENBI);
	udelay(5);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_TMDSCLK_EN,
									SUNH5_HDMI_PHY_ANA_CFG1_TMDSCLK_EN);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENVBS,
									SUNH5_HDMI_PHY_ANA_CFG1_ENVBS);
	usleep_range(10, 20);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_LDOEN,
									SUNH5_HDMI_PHY_ANA_CFG1_LDOEN);
	udelay(5);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_CKEN,
									SUNH5_HDMI_PHY_ANA_CFG1_CKEN);
	usleep_range(40, 100);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENRCAL,
									SUNH5_HDMI_PHY_ANA_CFG1_ENRCAL);
	usleep_range(100, 200);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENCALOG,
									SUNH5_HDMI_PHY_ANA_CFG1_ENCALOG);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS0 |
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS1 |
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS2,
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS0 |
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS1 |
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS2);

	/* wait for calibration to finish */										
	regmap_read_poll_timeout(phy->regs, SUNH5_HDMI_PHY_ANA_STS_REG, val, (val & SUNH5_HDMI_PHY_ANA_STS_RCALEND2D), 100, 2000);

	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDSCLK,
									SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDSCLK);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, 
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS0 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS1 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS2 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDSCLK,
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS0 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS1 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS2 |
									SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDSCLK);

	/* enable DDC communication */
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG3_REG, 
									SUNH5_HDMI_PHY_ANA_CFG3_SCLEN |
									SUNH5_HDMI_PHY_ANA_CFG3_SDAEN,
									SUNH5_HDMI_PHY_ANA_CFG3_SCLEN |
									SUNH5_HDMI_PHY_ANA_CFG3_SDAEN);

	/* reset PHY PLL clock parent */
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG,
									SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_MSK, 0);

	/* set HW control of CEC pins */
	regmap_write(phy->regs, SUNH5_HDMI_PHY_CEC_REG, 0);

	/* read calibration data */
	regmap_read(phy->regs, SUNH5_HDMI_PHY_ANA_STS_REG, &val);
	phy->rcal = (val & SUNH5_HDMI_PHY_ANA_STS_RCAL_MASK) >> 2;
}

static void sunh5_hdmi_phy_disable_h3(struct dw_hdmi *hdmi, struct sunh5_hdmi_phy *phy)
{
	regmap_write(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG,
							SUNH5_HDMI_PHY_ANA_CFG1_LDOEN |
							SUNH5_HDMI_PHY_ANA_CFG1_ENVBS |
							SUNH5_HDMI_PHY_ANA_CFG1_ENBI);
	regmap_write(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG, 0);
}

static int sunh5_hdmi_phy_config_h3(struct dw_hdmi *hdmi, struct sunh5_hdmi_phy *phy, unsigned int clk_rate)
{
	u32 pll_cfg1_init;
	u32 pll_cfg2_init;
	u32 ana_cfg1_end;
	u32 ana_cfg2_init;
	u32 ana_cfg3_init;
	u32 b_offset = 0;
	u32 val;

	/* bandwidth / frequency independent settings */

	pll_cfg1_init = SUNH5_HDMI_PHY_PLL_CFG1_LDO2_EN |
			SUNH5_HDMI_PHY_PLL_CFG1_LDO1_EN |
			SUNH5_HDMI_PHY_PLL_CFG1_LDO_VSET(7) |
			SUNH5_HDMI_PHY_PLL_CFG1_UNKNOWN(1) |
			SUNH5_HDMI_PHY_PLL_CFG1_PLLDBEN |
			SUNH5_HDMI_PHY_PLL_CFG1_CS |
			SUNH5_HDMI_PHY_PLL_CFG1_CP_S(2) |
			SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(63) |
			SUNH5_HDMI_PHY_PLL_CFG1_BWS;

	pll_cfg2_init = SUNH5_HDMI_PHY_PLL_CFG2_SV_H |
			SUNH5_HDMI_PHY_PLL_CFG2_VCOGAIN_EN |
			SUNH5_HDMI_PHY_PLL_CFG2_SDIV2;

	ana_cfg1_end = SUNH5_HDMI_PHY_ANA_CFG1_REG_SVBH(1) |
		       SUNH5_HDMI_PHY_ANA_CFG1_AMP_OPT |
		       SUNH5_HDMI_PHY_ANA_CFG1_EMP_OPT |
		       SUNH5_HDMI_PHY_ANA_CFG1_AMPCK_OPT |
		       SUNH5_HDMI_PHY_ANA_CFG1_EMPCK_OPT |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENRCAL |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENCALOG |
		       SUNH5_HDMI_PHY_ANA_CFG1_REG_SCKTMDS |
		       SUNH5_HDMI_PHY_ANA_CFG1_TMDSCLK_EN |
		       SUNH5_HDMI_PHY_ANA_CFG1_TXEN_MASK |
		       SUNH5_HDMI_PHY_ANA_CFG1_TXEN_ALL |
		       SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDSCLK |
		       SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS2 |
		       SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS1 |
		       SUNH5_HDMI_PHY_ANA_CFG1_BIASEN_TMDS0 |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS2 |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS1 |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENP2S_TMDS0 |
		       SUNH5_HDMI_PHY_ANA_CFG1_CKEN |
		       SUNH5_HDMI_PHY_ANA_CFG1_LDOEN |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENVBS |
		       SUNH5_HDMI_PHY_ANA_CFG1_ENBI;

	ana_cfg2_init = SUNH5_HDMI_PHY_ANA_CFG2_M_EN |
			SUNH5_HDMI_PHY_ANA_CFG2_REG_DENCK |
			SUNH5_HDMI_PHY_ANA_CFG2_REG_DEN |
			SUNH5_HDMI_PHY_ANA_CFG2_REG_CKSS(1) |
			SUNH5_HDMI_PHY_ANA_CFG2_REG_CSMPS(1);

	ana_cfg3_init = SUNH5_HDMI_PHY_ANA_CFG3_REG_WIRE(0x3e0) |
			SUNH5_HDMI_PHY_ANA_CFG3_SDAEN |
			SUNH5_HDMI_PHY_ANA_CFG3_SCLEN;

	/* bandwidth / frequency dependent settings */
	if (clk_rate <= 27000000) {
		pll_cfg1_init |= SUNH5_HDMI_PHY_PLL_CFG1_HV_IS_33 |
				 SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(32);
		pll_cfg2_init |= SUNH5_HDMI_PHY_PLL_CFG2_VCO_S(4) |
				 SUNH5_HDMI_PHY_PLL_CFG2_S(4);
		ana_cfg1_end |= SUNH5_HDMI_PHY_ANA_CFG1_REG_CALSW;
		ana_cfg2_init |= SUNH5_HDMI_PHY_ANA_CFG2_REG_SLV(4) |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_RESDI(phy->rcal);
		ana_cfg3_init |= SUNH5_HDMI_PHY_ANA_CFG3_REG_AMPCK(3) |
				 SUNH5_HDMI_PHY_ANA_CFG3_REG_AMP(5);
	} else if (clk_rate <= 74250000) {
		pll_cfg1_init |= SUNH5_HDMI_PHY_PLL_CFG1_HV_IS_33 |
				 SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(32);
		pll_cfg2_init |= SUNH5_HDMI_PHY_PLL_CFG2_VCO_S(4) |
				 SUNH5_HDMI_PHY_PLL_CFG2_S(5);
		ana_cfg1_end |= SUNH5_HDMI_PHY_ANA_CFG1_REG_CALSW;
		ana_cfg2_init |= SUNH5_HDMI_PHY_ANA_CFG2_REG_SLV(4) |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_RESDI(phy->rcal);
		ana_cfg3_init |= SUNH5_HDMI_PHY_ANA_CFG3_REG_AMPCK(5) |
				 SUNH5_HDMI_PHY_ANA_CFG3_REG_AMP(7);
	} else if (clk_rate <= 148500000) {
		pll_cfg1_init |= SUNH5_HDMI_PHY_PLL_CFG1_HV_IS_33 |
				 SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(32);
		pll_cfg2_init |= SUNH5_HDMI_PHY_PLL_CFG2_VCO_S(4) |
				 SUNH5_HDMI_PHY_PLL_CFG2_S(6);
		ana_cfg2_init |= SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSWCK |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSW |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_SLV(2);
		ana_cfg3_init |= SUNH5_HDMI_PHY_ANA_CFG3_REG_AMPCK(7) |
				 SUNH5_HDMI_PHY_ANA_CFG3_REG_AMP(9);
	} else {
		b_offset = 2;
		pll_cfg1_init |= SUNH5_HDMI_PHY_PLL_CFG1_CNT_INT(63);
		pll_cfg2_init |= SUNH5_HDMI_PHY_PLL_CFG2_VCO_S(6) |
				 SUNH5_HDMI_PHY_PLL_CFG2_S(7);
		ana_cfg2_init |= SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSWCK |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_BIGSW |
				 SUNH5_HDMI_PHY_ANA_CFG2_REG_SLV(4);
		ana_cfg3_init |= SUNH5_HDMI_PHY_ANA_CFG3_REG_AMPCK(9) |
				 SUNH5_HDMI_PHY_ANA_CFG3_REG_AMP(13) |
				 SUNH5_HDMI_PHY_ANA_CFG3_REG_EMP(3);
	}

	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG,
			   SUNH5_HDMI_PHY_ANA_CFG1_TXEN_MASK, 0);

	/*
	 * NOTE: We have to be careful not to overwrite PHY parent
	 * clock selection bit and clock divider.
	 */
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG,
			   (u32)~SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_MSK,
			   pll_cfg1_init);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG2_REG,
			   (u32)~SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_MSK,
			   pll_cfg2_init);
	usleep_range(10000, 15000);
	regmap_write(phy->regs, SUNH5_HDMI_PHY_PLL_CFG3_REG,
		     SUNH5_HDMI_PHY_PLL_CFG3_SOUT_DIV2);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG,
			   SUNH5_HDMI_PHY_PLL_CFG1_PLLEN,
			   SUNH5_HDMI_PHY_PLL_CFG1_PLLEN);
	msleep(100);

	/* get B value */
	regmap_read(phy->regs, SUNH5_HDMI_PHY_ANA_STS_REG, &val);
	val = (val & SUNH5_HDMI_PHY_ANA_STS_B_OUT_MSK) >>
		SUNH5_HDMI_PHY_ANA_STS_B_OUT_SHIFT;
	val = min(val + b_offset, (u32)0x3f);

	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG,
			   SUNH5_HDMI_PHY_PLL_CFG1_REG_OD1 |
			   SUNH5_HDMI_PHY_PLL_CFG1_REG_OD,
			   SUNH5_HDMI_PHY_PLL_CFG1_REG_OD1 |
			   SUNH5_HDMI_PHY_PLL_CFG1_REG_OD);
	regmap_update_bits(phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG,
			   SUNH5_HDMI_PHY_PLL_CFG1_B_IN_MSK,
			   val << SUNH5_HDMI_PHY_PLL_CFG1_B_IN_SHIFT);
	msleep(100);
	regmap_write(phy->regs, SUNH5_HDMI_PHY_ANA_CFG1_REG, ana_cfg1_end);
	regmap_write(phy->regs, SUNH5_HDMI_PHY_ANA_CFG2_REG, ana_cfg2_init);
	regmap_write(phy->regs, SUNH5_HDMI_PHY_ANA_CFG3_REG, ana_cfg3_init);

	return 0;

}

static const struct sunh5_hdmi_phy_variant sunh5_h3_hdmi_phy = {
	.has_phy_clk	= true,
	.is_custom_phy	= true,
	.phy_init		= &sunh5_hdmi_phy_init_h3,
	.phy_disable	= &sunh5_hdmi_phy_disable_h3,
	.phy_config		= &sunh5_hdmi_phy_config_h3,
};

static const struct of_device_id sunh5_hdmi_phy_of_table[] = {
	{
		.compatible = "allwinner,sunh5_hdmi_phy",
		.data = &sunh5_h3_hdmi_phy,
	},
	{  }
};

static struct regmap_config sunh5_hdmi_phy_regmap_config = {
	.reg_bits		= 32,
	.val_bits		= 32,
	.reg_stride		= 4,
	.max_register	= SUNH5_HDMI_PHY_CEC_REG,
	.name			= "phy"
};

int sunh5_hdmi_phy_probe(struct sunh5_dw_hdmi *hdmi, struct device_node *node)
{
	const struct of_device_id *match;
	struct device *dev = hdmi->dev;
	struct sunh5_hdmi_phy *phy;
	struct resource res;
	void __iomem *regs;
	int ret;

	match = of_match_node(sunh5_hdmi_phy_of_table, node);
	if(!match){
		dev_err(dev, "%s: Incompatible HDMI PHY\n", __func__);
		return -EINVAL;
	}

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if(!phy)
		return -ENOMEM;

	phy->dev = hdmi->dev;
	phy->variant = (struct sunh5_hdmi_phy_variant *)match->data;

	ret = of_address_to_resource(node, 0, &res);
	if(ret){
		dev_err(dev, "%s: phy: Couldn't get our resources\n", __func__);
		return ret;
	}

	regs = devm_ioremap_resource(dev, &res);
	if(IS_ERR(regs)){
		dev_err(dev, "%s: Couldn't map the HDMI PHY registers\n", __func__);
		return PTR_ERR(regs);
	}

	phy->regs = devm_regmap_init_mmio(dev, regs, &sunh5_hdmi_phy_regmap_config);
	if(IS_ERR(phy->regs)){
		dev_err(dev, "%s: Couldn't create the HDMI PHY regmap\n", __func__);
		return PTR_ERR(phy->regs);
	}

	phy->clk_bus = of_clk_get_by_name(node, "bus");
	if(IS_ERR(phy->clk_bus)){
		dev_err(dev, "%s: Couldn't get bus clock\n", __func__);
		return PTR_ERR(phy->clk_bus);
	}

	phy->clk_mod = of_clk_get_by_name(node, "mod");
	if(IS_ERR(phy->clk_bus)){
		dev_err(dev, "%s: Couldn't get mod clock\n", __func__);
		ret = PTR_ERR(phy->clk_bus);
		goto err_put_clk_bus;
	}

	if(phy->variant->has_phy_clk){
		phy->clk_pll0 = of_clk_get_by_name(node, "pll-0");
		if(IS_ERR(phy->clk_pll0)){
			dev_err(dev, "%s: Couldn't get pll-0 clock\n", __func__);
			ret = PTR_ERR(phy->clk_pll0);
			goto err_put_clk_mod;
		}
	}

	phy->rst_phy = of_reset_control_get_shared(node, "phy");
	if(IS_ERR(phy->rst_phy)){
		dev_err(dev, "%s: Couldn't get phy reset control\n", __func__);
		ret = PTR_ERR(phy->rst_phy);
		goto err_put_clk_pll0;
	}

	ret = reset_control_deassert(phy->rst_phy);
	if(ret){
		dev_err(dev, "%s: Cannot deassert phy reset control: %d\n", __func__, ret);
		goto err_put_rst_phy;
	}

	ret = clk_prepare_enable(phy->clk_bus);
	if(ret){
		dev_err(dev, "%s: Cannot enable bus clock: %d\n", __func__, ret);
		goto err_deassert_rst_phy;
	}

	ret = clk_prepare_enable(phy->clk_mod);
	if(ret){
		dev_err(dev, "%s: Cannot enable mod clock: %d\n", __func__, ret);
		goto err_disable_clk_bus;
	}

	if(phy->variant->has_phy_clk){
		ret = sunh5_phy_clk_create(phy, dev, phy->variant->has_second_pll);
		if(ret){
			dev_err(dev, "%s: Couldn't create the PHY clock\n", __func__);
			goto err_disable_clk_mod;
		}

		clk_prepare_enable(phy->clk_phy);
	}

	hdmi->phy = phy;

	return 0;
	
err_disable_clk_mod:
	clk_disable_unprepare(phy->clk_mod);
err_disable_clk_bus:
	clk_disable_unprepare(phy->clk_bus);
err_deassert_rst_phy:
	reset_control_assert(phy->rst_phy);
err_put_rst_phy:
	reset_control_put(phy->rst_phy);
err_put_clk_pll0:
	clk_put(phy->clk_pll0);
err_put_clk_mod:
	clk_put(phy->clk_mod);
err_put_clk_bus:
	clk_put(phy->clk_bus);

	return ret;
}

void sunh5_hdmi_phy_remove(struct sunh5_dw_hdmi *hdmi)
{
	struct sunh5_hdmi_phy *phy = hdmi->phy;

	clk_disable_unprepare(phy->clk_mod);
	clk_disable_unprepare(phy->clk_bus);
	clk_disable_unprepare(phy->clk_phy);

	reset_control_assert(phy->rst_phy);

	reset_control_put(phy->rst_phy);

	clk_put(phy->clk_pll0);
	clk_put(phy->clk_pll1);
	clk_put(phy->clk_mod);
	clk_put(phy->clk_bus);
}
