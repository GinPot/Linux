#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <drm/drm_of.h>
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include "sunh5_dw_hdmi.h"



static void sunh5_dw_hdmi_encoder_mode_set(struct drm_encoder *encoder, struct drm_display_mode *mode, struct drm_display_mode *adj_mode)
{
	struct sunh5_dw_hdmi *hdmi = encoder_to_sunh5_dw_hdmi(encoder);

	dev_dbg(hdmi->dev, "%s: Enter\n", __func__);

	if(hdmi->quirks->set_rate)
		clk_set_rate(hdmi->clk_tmds, mode->crtc_clock * 1000);
}

static const struct drm_encoder_helper_funcs sunh5_dw_hdmi_encoder_helper_funcs = {
	.mode_set = sunh5_dw_hdmi_encoder_mode_set,
};

static void sunh5_drm_encoder_cleanup(struct drm_encoder *encoder)
{
	dev_dbg(encoder->dev->dev, "%s: Enter\n", __func__);
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs sunh5_dw_hdmi_encoder_funcs = {
	.destroy = sunh5_drm_encoder_cleanup,
};

static u32 sunh5_dw_hdmi_find_possible_crtcs(struct drm_device *drm, struct device_node *node)
{
	struct device_node *remote;
	u32 crtcs = 0;

	remote = of_graph_get_remote_node(node, 0, -1);
	if(!remote)
		return 0;

	crtcs = drm_of_find_possible_crtcs(drm, node);
	
	of_node_put(remote);

	return crtcs;
}

static int sunh5_dw_hdmi_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_hdmi_plat_data *plat_data;
	struct drm_device *drm = data;
	struct device_node *phy_node;
	struct drm_encoder *encoder;
	struct sunh5_dw_hdmi *hdmi;
	int ret;

	if(!pdev->dev.of_node)
		return -ENODEV;

	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	hdmi = devm_kzalloc(&pdev->dev, sizeof(*hdmi), GFP_KERNEL);
	if(!hdmi)
		return -ENOMEM;

	plat_data = &hdmi->plat_data;
	hdmi->dev = &pdev->dev;
	encoder = &hdmi->encoder;

	hdmi->quirks = of_device_get_match_data(dev);
	encoder->possible_crtcs = sunh5_dw_hdmi_find_possible_crtcs(drm, dev->of_node);
	/*
	 * If we failed to find the CRTC(s) which this encoder is
	 * supposed to be connected to, it's because the CRTC has
	 * not been registered yet.  Defer probing, and hope that
	 * the required CRTC is added later.
	 */
	if(encoder->possible_crtcs == 0)
		return -EPROBE_DEFER;

	hdmi->rst_ctrl = devm_reset_control_get(dev, "ctrl");
	if(IS_ERR(hdmi->rst_ctrl)){
		dev_err(dev, "%s: Could not get ctrl rest control\n", __func__);
		return PTR_ERR(hdmi->rst_ctrl);
	}

	hdmi->clk_tmds = devm_clk_get(dev, "tmds");
	if(IS_ERR(hdmi->clk_tmds)){
		dev_err(dev, "%s: Could not get tmds clock\n", __func__);
		return PTR_ERR(hdmi->clk_tmds);
	}

	hdmi->regulator = devm_regulator_get(dev, "hvcc");
	if(IS_ERR(hdmi->regulator)){
		dev_err(dev, "%s: Could not get regulator\n", __func__);
		return PTR_ERR(hdmi->regulator);
	}

	ret = regulator_enable(hdmi->regulator);
	if(ret){
		dev_err(dev, "%s: Failed to enable regulator\n", __func__);
		return ret;
	}

	ret = reset_control_deassert(hdmi->rst_ctrl);
	if(ret){
		dev_err(dev, "%s: Could not deassert ctrl reset control\n", __func__);
		goto err_disable_regulator;
	}

	ret = clk_prepare_enable(hdmi->clk_tmds);
	if(ret){
		dev_err(dev, "%s: Could not enable tmds clock\n", __func__);
		goto err_assert_ctrl_reset;
	}

	phy_node = of_parse_phandle(dev->of_node, "phys", 0);
	if(!phy_node){
		dev_err(dev, "%s: Can't found PHY phandle\n", __func__);
		goto err_disable_clk_tmds;
	}

	ret = sunh5_hdmi_phy_probe(hdmi, phy_node);
	of_node_put(phy_node);
	if(ret){
		dev_err(dev, "%s: Couldn't get the HDMI PHY\n", __func__);
		goto err_disable_clk_tmds;
	}

	drm_encoder_helper_add(encoder, &sunh5_dw_hdmi_encoder_helper_funcs);
	drm_encoder_init(drm, encoder, &sunh5_dw_hdmi_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);

	sunh5_hdmi_phy_init(hdmi->phy);

	plat_data->mode_valid = hdmi->quirks->mode_valid;
	sunh5_hdmi_phy_set_ops(hdmi->phy, plat_data);

	platform_set_drvdata(pdev, hdmi);

	hdmi->hdmi = dw_hdmi_bind(pdev, encoder, plat_data);
	/*
	 * If dw_hdmi_bind() fails we'll never call dw_hdmi_unbind(),
	 * which would have called the encoder cleanup.  Do it manually.
	 */
	if(IS_ERR(hdmi->hdmi)){
		ret = PTR_ERR(hdmi->hdmi);
		goto cleanup_encoder;
	}

	return 0;


cleanup_encoder:
	drm_encoder_cleanup(encoder);
	sunh5_hdmi_phy_remove(hdmi);
err_disable_clk_tmds:
	clk_disable_unprepare(hdmi->clk_tmds);
err_assert_ctrl_reset:
	reset_control_assert(hdmi->rst_ctrl);
err_disable_regulator:
	regulator_disable(hdmi->regulator);

	return ret;
}

static void sunh5_dw_hdmi_unbind(struct device *dev, struct device *master, void *data)
{
	struct sunh5_dw_hdmi *hdmi = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: Enter\n", __func__);

	dw_hdmi_unbind(hdmi->hdmi);
	sunh5_hdmi_phy_remove(hdmi);
	clk_disable_unprepare(hdmi->clk_tmds);
	reset_control_assert(hdmi->rst_ctrl);
	regulator_disable(hdmi->regulator);
}

static const struct component_ops sunh5_dw_hdmi_ops = {
	.bind	= sunh5_dw_hdmi_bind,
	.unbind = sunh5_dw_hdmi_unbind,
};

static int sunh5_dw_hdmi_probe(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	return component_add(&pdev->dev, &sunh5_dw_hdmi_ops);

}

static int sunh5_dw_hdmi_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	component_del(&pdev->dev, &sunh5_dw_hdmi_ops);

	return 0;
}

static enum drm_mode_status sunh5_dw_hdmi_mode_valid(struct drm_connector *connector, const struct drm_display_mode *mode)
{
	if(mode->clock > 297000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static const struct sunh5_dw_hdmi_quirks sunh5_quirks = {
	.mode_valid	= sunh5_dw_hdmi_mode_valid,
	.set_rate = true,
};

static const struct of_device_id sunh5_dw_hdmi_dt_ids[] = {
	{
		.compatible	= "allwinner,sunh5-dw-hdmi",
		.data		= &sunh5_quirks,
	},
	{  }
};
MODULE_DEVICE_TABLE(of, sunh5_dw_hdmi_dt_ids);

static struct platform_driver sunh5_dw_hdmi_platform_driver = {
	.probe				= sunh5_dw_hdmi_probe,
	.remove				= sunh5_dw_hdmi_remove,
	.driver				= {
		.name			= "sunh5-dw-hdmi",
		.of_match_table	=  sunh5_dw_hdmi_dt_ids,
	},
};

module_platform_driver(sunh5_dw_hdmi_platform_driver);

MODULE_DESCRIPTION("Allwinner DW HDMI bridge");
MODULE_AUTHOR("GinPot <GinPot@xx.com>");
MODULE_LICENSE("GPL");
