#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/clk.h>


#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>

#include "sunh5_drv.h"
#include "sunh5_mixer.h"
#include "sunh5_vi_layer.h"
#include "sunh5_ui_layer.h"



static const struct de2_fmt_info de2_formats[] = {
	{
		.drm_fmt = DRM_FORMAT_ARGB8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_ARGB8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_ABGR8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_ABGR8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGBA8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGBA8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGRA8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGRA8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_XRGB8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_XRGB8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_XBGR8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_XBGR8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGBX8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGBX8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGRX8888,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGRX8888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGB888,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGB888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGR888,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGR888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGB565,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGB565,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGR565,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGR565,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_ARGB4444,
		.de2_fmt = SUNH5_MIXER_FBFMT_ARGB4444,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_ABGR4444,
		.de2_fmt = SUNH5_MIXER_FBFMT_ABGR4444,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGBA4444,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGBA4444,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGRA4444,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGRA4444,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_ARGB1555,
		.de2_fmt = SUNH5_MIXER_FBFMT_ARGB1555,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_ABGR1555,
		.de2_fmt = SUNH5_MIXER_FBFMT_ABGR1555,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_RGBA5551,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGBA5551,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_BGRA5551,
		.de2_fmt = SUNH5_MIXER_FBFMT_BGRA5551,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_OFF,
	},
	{
		.drm_fmt = DRM_FORMAT_UYVY,
		.de2_fmt = SUNH5_MIXER_FBFMT_UYVY,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_VYUY,
		.de2_fmt = SUNH5_MIXER_FBFMT_VYUY,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YUYV,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUYV,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YVYU,
		.de2_fmt = SUNH5_MIXER_FBFMT_YVYU,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_NV16,
		.de2_fmt = SUNH5_MIXER_FBFMT_NV16,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_NV61,
		.de2_fmt = SUNH5_MIXER_FBFMT_NV61,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_NV12,
		.de2_fmt = SUNH5_MIXER_FBFMT_NV12,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_NV21,
		.de2_fmt = SUNH5_MIXER_FBFMT_NV21,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YUV444,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGB888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YUV422,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV422,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YUV420,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV420,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YUV411,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV411,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YUV2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YVU444,
		.de2_fmt = SUNH5_MIXER_FBFMT_RGB888,
		.rgb = true,
		.csc = SUNH5_CSC_MODE_YVU2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YVU422,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV422,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YVU2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YVU420,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV420,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YVU2RGB,
	},
	{
		.drm_fmt = DRM_FORMAT_YVU411,
		.de2_fmt = SUNH5_MIXER_FBFMT_YUV411,
		.rgb = false,
		.csc = SUNH5_CSC_MODE_YVU2RGB,
	},
};

const struct de2_fmt_info *sunh5_mixer_format_info(u32 format)
{
	unsigned int i;

	for(i = 0; i < ARRAY_SIZE(de2_formats); i++){
		if(de2_formats[i].drm_fmt == format)
			return &de2_formats[i];
	}

	return NULL;
}

static void sunh5_mixer_commit(struct sunh5_engine *engine)
{
	dev_dbg(engine->dev, "%s: Enter, committing changes\n", __func__);

	regmap_write(engine->regs, SUNH5_MIXER_GLOBAL_DBUFF, SUNH5_MIXER_GLOBAL_DBUFF_ENABLE);
}

static struct drm_plane **sunh5_layers_init(struct drm_device *drm, struct sunh5_engine *engine)
{
	struct drm_plane **planes;
	struct sunh5_mixer *mixer = engine_to_sunh5_mixer(engine);
	int i;

	dev_dbg(engine->dev, "%s: Enter\n", __func__);

	planes = devm_kcalloc(drm->dev, mixer->cfg->vi_num + mixer->cfg->ui_num + 1, sizeof(*planes), GFP_KERNEL);
	if(!planes)
		return ERR_PTR(-ENOMEM);

	for(i = 0; i < mixer->cfg->vi_num; i++){
		struct sunh5_vi_layer *layer;

		layer = sunh5_vi_layer_init_one(drm, mixer, i);
		if(IS_ERR(layer)){
			dev_err(engine->dev, "%s: Couldn't initialize overlay plane\n", __func__);
			return ERR_CAST(layer);
		}

		planes[i] = &layer->plane;
	}

	for(i = 0; i < mixer->cfg->ui_num; i++){
		struct sunh5_ui_layer *layer;
		layer = sunh5_ui_layer_init_one(drm, mixer, i);
		if(IS_ERR(layer)){
			dev_err(drm->dev, "Couldn't initialize %s plane\n", i ? "overlay" : "primary");
			return ERR_CAST(layer);
		}

		planes[mixer->cfg->vi_num + i] = &layer->plane;
	}

	return planes;
}

static const struct sunh5_engine_ops sunh5_engine_ops = {
	.commit			= sunh5_mixer_commit,
	.layers_init	= sunh5_layers_init,
};

static int sunh5_mixer_of_get_id(struct device *dev, struct device_node *node)
{
	struct device_node *ep, *remote;
	struct of_endpoint of_ep;

	/* Output prot is 1, and we want the first endpoint */
	ep = of_graph_get_endpoint_by_regs(node, 1, -1);
	if(!ep)
		return -EINVAL;

	remote = of_graph_get_remote_endpoint(ep);
	dev_dbg(dev, "%s: ep->name=%s	remote->name=%s	", __func__, ep->name, remote->name);
	of_node_put(ep);
	if(!remote)
		return -EINVAL;

	of_graph_parse_endpoint(remote, &of_ep);
	of_node_put(remote);

	dev_dbg(dev, "%s: of_ep.id=%d of_ep.port=%d\n", __func__, of_ep.id,of_ep.port);

	return of_ep.id;
		
}

static struct regmap_config sunh5_mixer_regmap_config = {
	.reg_bits		= 32,
	.val_bits		= 32,
	.reg_stride		= 4,
	.max_register	= 0xbfffc,	/* guessed */
};

static int sunh5_mixer_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = data;
	struct sunh5_drv *drv = drm->dev_private;
	struct sunh5_mixer *mixer;
	struct resource *res;
	void __iomem *regs;
	unsigned int base;
	int plane_cnt;
	int i, ret;

	dev_dbg(dev, "%s: Enter\n", __func__);

	/*
	 * The mixer uses single 32-bit register to store memory
	 * addresses, so that it cannot deal with 64-bit memory
	 * addresses.
	 * Restrict the DMA mask so that the mixer won't be
	 * allocated some memory that is too high.
	 */
	ret = dma_set_mask(dev, DMA_BIT_MASK(32));
	if(ret){
		dev_err(dev, "%s: Cannot do 32-bit DMA.\n", __func__);
		return ret;
	}

	mixer = devm_kzalloc(dev, sizeof(*mixer), GFP_KERNEL);
	if(!mixer)
		return -ENOMEM;
	dev_set_drvdata(dev, mixer);
	mixer->engine.dev = dev;
	mixer->engine.ops = &sunh5_engine_ops;
	mixer->engine.node = dev->of_node;
	
	/*
	 * While this function can fail, we shouldn't do anything
	 * if this happens. Some early DE2 DT entries don't provide
	 * mixer id but work nevertheless because matching between
	 * TCON and mixer is done by comparing node pointers (old
	 * way) instead comparing ids. If this function fails and
	 * id is needed, it will fail during id matching anyway.
	 */
	mixer->engine.id = sunh5_mixer_of_get_id(dev, dev->of_node);

	mixer->cfg = of_device_get_match_data(dev);
	if(!mixer->cfg)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(dev, res);
	if(IS_ERR(regs))
		return PTR_ERR(regs);

	mixer->engine.regs = devm_regmap_init_mmio(dev, regs, &sunh5_mixer_regmap_config);
	if(IS_ERR(mixer->engine.regs)){
		dev_err(dev, "Couldn't create the mixer regmap\n");
		return PTR_ERR(mixer->engine.regs);
	}

	mixer->reset = devm_reset_control_get(dev, NULL);
	if(IS_ERR(mixer->reset)){
		dev_err(dev, "Couldn't get our reset line\n");
		return PTR_ERR(mixer->reset);
	}

	ret = reset_control_deassert(mixer->reset);				/* 相关位提供低信号,使得IP可以正常工作 */
	if(ret){
		dev_err(dev, "Couldn't deassert our reset line");
		return ret;
	}

	mixer ->bus_clk = devm_clk_get(dev, "bus");
	if(IS_ERR(mixer->bus_clk)){
		dev_err(dev, "Couldn't get the mixer bus clock\n");
		ret = PTR_ERR(mixer->bus_clk);
		goto err_assert_reset;
	}
	clk_prepare_enable(mixer->bus_clk);

	mixer->mode_clk = devm_clk_get(dev, "mod");
	if(IS_ERR(mixer->mode_clk)){
		dev_err(dev, "Couldn't get the mixer module clock\n");
		ret = PTR_ERR(mixer->mode_clk);
		goto err_disable_bus_clk;
	}

	/*
	 * It seems that we need to enforce that rate for whatever
	 * reason for the mixer to be functional. Make sure it's the
	 * case
	 */
	if(mixer->cfg->mode_rate)
		clk_set_rate(mixer->mode_clk, mixer->cfg->mode_rate);
	clk_prepare_enable(mixer->mode_clk);

	list_add_tail(&mixer->engine.list, &drv->engine_list);

	base = sunh5_blender_base(mixer);

	/* Reset registers and disable unused sub-engines */
	for(i = 0; i < DE2_MIXER_UNIT_SIZE; i += 4)
		regmap_write(mixer->engine.regs, i, 0);

	regmap_write(mixer->engine.regs, SUNH5_MIXER_FCE_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BWS_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_LTI_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_PEAK_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_ASE_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_FCC_EN, 0);
	regmap_write(mixer->engine.regs, SUNH5_MIXER_DCSC_EN, 0);

	/* Enable the mixer */
	regmap_write(mixer->engine.regs, SUNH5_MIXER_GLOBAL_CTL, SUNH5_MIXER_GLOBAL_CTL_RT_EN);

	/* Set background color to black */
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_BKCOLOR(base), SUNH5_MIXER_BLEND_COLOR_BLACK);

	/*
	 * Set fill color of bottom plane to black. Generally not needed
	 * except when VI plane is at bottom (zpos = 0) and enabled.
	 */
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_PIPE_CTL(base), SUNH5_MIXER_BLEND_PIPE_CTL_EN(0));
	regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_ATTR_FCOLOR(base, 0), SUNH5_MIXER_BLEND_COLOR_BLACK);

	plane_cnt = mixer->cfg->vi_num + mixer->cfg->ui_num;
	for(i = 0; i < plane_cnt; i++)
		regmap_write(mixer->engine.regs, SUNH5_MIXER_BLEND_MODE(base, i), SUNH5_MIXER_BLEND_MODE_DEF);

	regmap_update_bits(mixer->engine.regs, SUNH5_MIXER_BLEND_PIPE_CTL(base), SUNH5_MIXER_BLEND_PIPE_CTL_EN_MSK, 0);

	return 0;
	
err_disable_bus_clk:
	clk_disable_unprepare(mixer->bus_clk);
err_assert_reset:
	reset_control_assert(mixer->reset);			/* 相关位提供高信息,使的IP一直处于复位生效状态 */
	return ret;
}

static void sunh5_mixer_unbind(struct device *dev, struct device *master, void *data){
	struct sunh5_mixer *mixer = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: Enter\n", __func__);

	clk_disable_unprepare(mixer->mode_clk);
	clk_disable_unprepare(mixer->bus_clk);
	reset_control_assert(mixer->reset);
}

static const struct component_ops sunh5_mixer_ops = {
	.bind	= sunh5_mixer_bind,
	.unbind	= sunh5_mixer_unbind,
};

static int sunh5_mixer_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &sunh5_mixer_ops);
}

static int sunh5_mixer_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sunh5_mixer_ops);

	return 0;
}

static const struct sunh5_mixer_cfg sunh5_mixer0_cfg = {
	.ccsc			= 0,
	.mode_rate		= 432000000,
	.scaler_mask	= 0xf,
	.scanline_yuv	= 2048,
	.ui_num			= 3,
	.vi_num			= 1,
};

static const struct of_device_id sunh5_mixer_of_table[] = {
	{
		.compatible	= "allwinner,sunh5-de2-mixer-0",
		.data		= &sunh5_mixer0_cfg,
	},
	{  }
};

MODULE_DEVICE_TABLE(of, sunh5_mixer_of_table);

static struct platform_driver sunh5_mixer_platform_driver = {
	.probe				= sunh5_mixer_probe,
	.remove				= sunh5_mixer_remove,
	.driver				= {
		.name			= "sunh5-mixer",
		.of_match_table	= sunh5_mixer_of_table,
	},
};

module_platform_driver(sunh5_mixer_platform_driver);

MODULE_DESCRIPTION("Allwinner DE2 Mixer Driver");
MODULE_AUTHOR("GinPot <GinPot@xx.com>");
MODULE_LICENSE("GLP");
