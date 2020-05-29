#include <linux/clk-provider.h>

#include "sunh5_dw_hdmi.h"



struct sunh5_phy_clk {
	struct device			*dev;
	struct clk_hw			hw;
	struct sunh5_hdmi_phy	*phy;
};



static int sunh5_phy_clk_determine_rate(struct clk_hw *hw, struct clk_rate_request *req)
{
	unsigned long rate = req->rate;
	unsigned long best_rate = 0;
	struct clk_hw *best_parent = NULL;
	struct clk_hw *parent;
	int best_div = 1;
	int i, p;

	printk("%s:Enter\n", __func__);

	for(p = 0; p < clk_hw_get_num_parents(hw); p++){
		parent = clk_hw_get_parent_by_index(hw, p);
		if(!parent)
			continue;

		for(i = 1; i <= 16; i++){
			unsigned long ideal = rate * i;
			unsigned long rounded;

			rounded = clk_hw_round_rate(parent, ideal);

			if(rounded == ideal){
				best_rate = rounded;
				best_div = i;
				best_parent = parent;
				break;
			}

			if(!best_rate || abs(rate - rounded / i) < abs(rate - best_rate / best_div)){
				best_rate = rounded;
				best_div = i;
				best_parent = parent;
			}
		}

		if(best_rate / best_div == rate)
			break;
	}

	req->rate = best_rate / best_div;
	req->best_parent_rate = best_rate;
	req->best_parent_hw	= best_parent;

	return 0;
}

static inline struct sunh5_phy_clk *hw_to_phy_clk(struct clk_hw *hw)
{
	return container_of(hw, struct sunh5_phy_clk, hw);
}

static unsigned long sunh5_phy_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct sunh5_phy_clk *priv = hw_to_phy_clk(hw);
	u32 reg;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	regmap_read(priv->phy->regs, SUNH5_HDMI_PHY_PLL_CFG2_REG, &reg);
	reg = ((reg >> SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_SHIFT) & SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_MSK) + 1;

	return parent_rate / reg;
}

static int sunh5_phy_clk_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	struct sunh5_phy_clk *priv = hw_to_phy_clk(hw);
	unsigned long best_rate = 0;
	u8 best_m =0, m;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	for(m = 1; m <= 16; m++){
		unsigned long tmp_rate = parent_rate / m;

		if(tmp_rate > rate)
			continue;

		if(!best_rate || (rate - tmp_rate) < (rate - best_rate)){
			best_rate = tmp_rate;
			best_m = m;
		}
	}

	regmap_update_bits(priv->phy->regs, SUNH5_HDMI_PHY_PLL_CFG2_REG, SUNH5_HDMI_PHY_PLL_CFG2_PREDIV_MSK, SUNH5_HDMI_PHY_PLL_CFG2_PREDIV(best_m));

	return 0;
}

static u8 sunh5_phy_clk_get_parent(struct clk_hw *hw)
{
	struct sunh5_phy_clk *priv = hw_to_phy_clk(hw);
	u32 reg;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	regmap_read(priv->phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG, &reg);
	reg = (reg & SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_MSK) >> SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_SHIFT;

	return 0;
}

static int sunh5_phy_clk_set_parent(struct clk_hw *hw, u8 index)
{
	struct sunh5_phy_clk *priv = hw_to_phy_clk(hw);

	if(index > 1)
		return -EINVAL;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	regmap_update_bits(priv->phy->regs, SUNH5_HDMI_PHY_PLL_CFG1_REG, SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_MSK, index << SUNH5_HDMI_PHY_PLL_CFG1_CKIN_SEL_SHIFT);

	return 0;
}

static const struct clk_ops sunh5_phy_clk_ops = {
	.determine_rate	= sunh5_phy_clk_determine_rate,
	.recalc_rate	= sunh5_phy_clk_recalc_rate,
	.set_rate		= sunh5_phy_clk_set_rate,

	.get_parent		= sunh5_phy_clk_get_parent,
	.set_parent		= sunh5_phy_clk_set_parent,
};

int sunh5_phy_clk_create(struct sunh5_hdmi_phy *phy, struct device *dev, bool second_parent)
{
	struct clk_init_data init;
	struct sunh5_phy_clk *priv;
	const char *parents[2];

	dev_dbg(dev, "%s: Enter\n", __func__);

	parents[0] = __clk_get_name(phy->clk_pll0);
	if(!parents[0])
		return -ENODEV;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	init.name = "hdmi-phy_clk";
	init.ops = &sunh5_phy_clk_ops;
	init.parent_names = parents;
	init.num_parents = second_parent ? 2 : 1;
	init.flags = CLK_SET_RATE_PARENT;

	priv->dev = dev;
	priv->phy = phy;
	priv->hw.init = &init;

	phy->clk_phy = devm_clk_register(dev, &priv->hw);
	if(IS_ERR(phy->clk_phy))
		return PTR_ERR(phy->clk_phy);

	return 0;
}
