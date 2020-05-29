#include <linux/component.h>
#include <linux/ioport.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/clk.h>

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_encoder.h>
#include <drm/drm_modes.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

#include "sunh5_drv.h"
#include "sunh5_tcon.h"
#include "sunh5_engine.h"
#include "sunh5_crtc.h"





static int sunh5_tcon_get_clk_delay(struct device *dev, const struct drm_display_mode *mode, int channel)
{
	int delay = mode->vtotal - mode->vdisplay;

	if(mode->flags & DRM_MODE_FLAG_INTERLACE)
		delay /= 2;

	if(channel == 1)
		delay -= 2;

	delay = min(delay, 30);

	dev_dbg(dev, "%s: TCON %d clock delay %u\n", __func__, channel, delay);

	return delay;
}

static void sunh5_tcon1_mode_set(struct sunh5_tcon *tcon, const struct drm_display_mode *mode)
{
	unsigned int bp, hsync, vsync, vtotal;
	u8 clk_delay;
	u32 val;

	WARN_ON(!tcon->quirks->has_channel_1);

	/* Configure the dot clock */
	clk_set_rate(tcon->sclk1, mode->crtc_clock * 1000);

	/* Adjust clock delay */
	clk_delay = sunh5_tcon_get_clk_delay(tcon->dev, mode, 1);
	regmap_update_bits(tcon->regs, SUNH5_TCON1_CTL_REG, SUNH5_TCON1_CTL_CLK_DELAY_MASK, SUNH5_TCON1_CTL_CLK_DELAY(clk_delay));

	/* Set interlaced mode */
	if(mode->flags & DRM_MODE_FLAG_INTERLACE)
		val = SUNH5_TCON1_CTL_INTERLACE_ENABLE;
	else
		val = 0;
	regmap_update_bits(tcon->regs, SUNH5_TCON1_CTL_REG, SUNH5_TCON1_CTL_INTERLACE_ENABLE, val);

	/* Set the input resolution */
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC0_REG, SUNH5_TCON1_BASIC0_X(mode->crtc_hdisplay) | SUNH5_TCON1_BASIC0_Y(mode->crtc_vdisplay));

	/* Set the upscaling resolution */
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC1_REG, SUNH5_TCON1_BASIC1_X(mode->crtc_hdisplay) | SUNH5_TCON1_BASIC1_Y(mode->crtc_vdisplay));

	/* Set the output resolution */
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC2_REG, SUNH5_TCON1_BASIC2_X(mode->crtc_hdisplay) | SUNH5_TCON1_BASIC2_Y(mode->crtc_vdisplay));

	/* Set horizontal display timings */
	bp = mode->crtc_htotal - mode->crtc_hsync_start;
	dev_dbg(tcon->dev, "%s: Setting horizontal total %d, backporch %d\n", __func__, mode->htotal, bp);
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC3_REG, SUNH5_TCON1_BASIC3_H_TOTAL(mode->crtc_htotal) | SUNH5_TCON1_BASIC3_H_BACKPORCH(bp));

	bp = mode->crtc_vtotal - mode->crtc_vsync_start;
	dev_dbg(tcon->dev, "%s: Setting vertical total %d, backporch %d\n", __func__, mode->vtotal, bp);

	/*
	 * The vertical resolution needs to be doubled in all
	 * cases. We could use crtc_vtotal and always multiply by two,
	 * but that leads to a rounding error in interlace when vtotal
	 * is odd.
	 *
	 * This happens with TV's PAL for example, where vtotal will
	 * be 625, crtc_vtotal 312, and thus crtc_vtotal * 2 will be
	 * 624, which apparently confuses the hardware.
	 *
	 * To work around this, we will always use vtotal, and
	 * multiply by two only if we're not in interlace.
	 */
	vtotal = mode->vtotal;
	if(!(mode->flags & DRM_MODE_FLAG_INTERLACE))
		vtotal = vtotal * 2;

	/* Set vertical display timings  */
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC4_REG, SUNH5_TCON1_BASIC4_V_TOTAL(vtotal) | SUNH5_TCON1_BASIC4_V_BACKPORCH(bp));


	/* Set Hsync and Vsync length */
	hsync = mode->crtc_hsync_end - mode->crtc_hsync_start;
	vsync = mode->crtc_vblank_end - mode->crtc_vsync_start;
	dev_dbg(tcon->dev, "%s: Setting HSYNC %d, VSYNC %d\n", __func__, hsync, vsync);
	regmap_write(tcon->regs, SUNH5_TCON1_BASIC5_REG, SUNH5_TCON1_BASIC5_V_SYNC(vtotal) | SUNH5_TCON1_BASIC5_H_SYNC(bp));

	/* Map output pins to channel 1 */
	regmap_update_bits(tcon->regs, SUNH5_TCON_GCTL_REG, SUNH5_TCON_GCTL_IOMAP_MASK, SUNH5_TCON_GCTL_IOMAP_TCON1);

}


void sunh5_tcon_mode_set(struct sunh5_tcon *tcon, const struct drm_encoder *encoder, const struct drm_display_mode *mode)
{
	switch(encoder->encoder_type){
		case DRM_MODE_ENCODER_TVDAC:
		case DRM_MODE_ENCODER_TMDS:
			sunh5_tcon1_mode_set(tcon, mode);
		break;

		default:
			dev_err(tcon->dev, "%s: Unknown type, doing nothing...\n", __func__);
	}
}

static void sunh5_tcon_channel_set_status(struct sunh5_tcon *tcon, int channel, bool enabled)
{
	struct clk *clk;

	switch(channel){
		case 1:
			WARN_ON(!tcon->quirks->has_channel_1);
			regmap_update_bits(tcon->regs, SUNH5_TCON1_CTL_REG, SUNH5_TCON1_CTL_TCON_ENABLE, enabled ? SUNH5_TCON1_CTL_TCON_ENABLE : 0);
			clk = tcon->sclk1;
		break;
		default:
			dev_err(tcon->dev, "%s: Unknown channel, doing nothing...\n", __func__);
			return;
	}

	if(enabled){
		clk_prepare_enable(clk);
		clk_rate_exclusive_get(clk);
	}else{
		clk_rate_exclusive_put(clk);
		clk_disable_unprepare(clk);
	}
}

void sunh5_tcon_set_status(struct sunh5_tcon *tcon, const struct drm_encoder *encoder, bool enabled)
{
	int channel;

	switch(encoder->encoder_type){
		case DRM_MODE_ENCODER_DSI:
		case DRM_MODE_ENCODER_NONE:
			channel = 0;
		break;
		case DRM_MODE_ENCODER_TMDS:
		case DRM_MODE_ENCODER_TVDAC:
			channel = 1;
		break;
		default:
			dev_err(tcon->dev, "%s: Unknown encoder type, doing nothing...\n", __func__);
			return;
	}

	regmap_update_bits(tcon->regs, SUNH5_TCON_GCTL_REG, SUNH5_TCON_GCTL_TCON_ENABLE, enabled ? SUNH5_TCON_GCTL_TCON_ENABLE : 0);

	sunh5_tcon_channel_set_status(tcon, channel, enabled);
}

void sunh5_tcon_enable_vblank(struct sunh5_tcon *tcon, bool enable)
{
	u32 mask, val = 0;

	dev_dbg(tcon->dev, "%s: %sabling VBLANK interrupt\n", __func__, enable ? "En" : "Dis");

	mask = SUNH5_TCON_GINT0_VBLANK_ENABLE(0) | SUNH5_TCON_GINT0_VBLANK_ENABLE(1) | SUNH5_TCON_GINT0_TCON0_TRI_FINISH_ENABLE;

	if(enable)
		val = mask;

	regmap_update_bits(tcon->regs, SUNH5_TCON_GINT0_REG, mask, val);	
}

static int sunh5_tcon_init_clocks(struct device *dev, struct sunh5_tcon *tcon)
{
	tcon->clk = devm_clk_get(dev, "ahb");
	if(IS_ERR(tcon->clk)){
		dev_err(dev, "%s: Couldn't get the TCON bus clock\n", __func__);
		return PTR_ERR(tcon->clk);
	}
	clk_prepare_enable(tcon->clk);

	if(tcon->quirks->has_channel_1){
		tcon->sclk1 = devm_clk_get(dev, "tcon-ch1");
		if(IS_ERR(tcon->sclk1)){
			dev_err(dev, "%s: Couldn't get the TCON channel 1 clock\n", __func__);
			return PTR_ERR(tcon->sclk1);
		}
	}

	return 0;
}

static struct regmap_config sunh5_tcon_regmap_config = {
	.reg_bits		= 32,
	.val_bits		= 32,
	.reg_stride		= 4,
	.max_register	= 0x800,
};

static int sunh5_tcon_init_regmap(struct device *dev, struct sunh5_tcon *tcon)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;
	void __iomem *regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(dev, res);
	if(IS_ERR(regs))
		return PTR_ERR(regs);

	tcon->regs = devm_regmap_init_mmio(dev, regs, &sunh5_tcon_regmap_config);
	if(IS_ERR(tcon->regs)){
		dev_err(dev, "%s: Couldn't create the TCON regmap\n", __func__);
		return PTR_ERR(tcon->regs);
	}

	/* Make sure the TCON is disabled and all IRQs are off */
	regmap_write(tcon->regs, SUNH5_TCON_GCTL_REG, 0);
	regmap_write(tcon->regs, SUNH5_TCON_GINT0_REG, 0);
	regmap_write(tcon->regs, SUNH5_TCON_GINT1_REG, 0);

	/* Disable IO lines and set them to tristate */
	regmap_write(tcon->regs, SUNH5_TCON0_IO_TRI_REG, ~0);			/* ?????????????? */
	regmap_write(tcon->regs, SUNH5_TCON1_IO_TRI_REG, ~0);

	return 0;
}

static void sunh5_tcon_free_clocks(struct sunh5_tcon *tcon)
{
	clk_disable_unprepare(tcon->sclk1);
	clk_disable_unprepare(tcon->clk);
}

static void sunh5_tcon_finish_page_flip(struct drm_device *dev, struct sunh5_crtc *scrtc)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);
	if(scrtc->event){
		drm_crtc_send_vblank_event(&scrtc->crtc, scrtc->event);
		drm_crtc_vblank_put(&scrtc->crtc);
		scrtc->event = NULL;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

static irqreturn_t sunh5_tcon_handler(int irq, void *private)
{
	struct sunh5_tcon *tcon = private;
	struct drm_device *drm = tcon->drm;
	struct sunh5_crtc *scrtc = tcon->crtc;
//	struct sunh5_engine *engine = scrtc->engine;
	unsigned int status;

	regmap_read(tcon->regs, SUNH5_TCON_GINT0_REG, &status);

	if(!(status & (SUNH5_TCON_GINT0_VBLANK_INT(0) | SUNH5_TCON_GINT0_VBLANK_INT(1) | SUNH5_TCON_GINT0_TCON0_TRI_FINISH_INT)))
		return IRQ_NONE;

	drm_crtc_handle_vblank(&scrtc->crtc);
	sunh5_tcon_finish_page_flip(drm, scrtc);

	/* Acknowledge the interrupt */
	regmap_update_bits(tcon->regs, SUNH5_TCON_GINT0_REG,
						SUNH5_TCON_GINT0_VBLANK_INT(0) | SUNH5_TCON_GINT0_VBLANK_INT(1) | SUNH5_TCON_GINT0_TCON0_TRI_FINISH_INT, 0);

	return IRQ_HANDLED;
}

static int sunh5_tcon_init_irq(struct device *dev, struct sunh5_tcon *tcon)
{
	struct platform_device *pdev = to_platform_device(dev);
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if(irq < 0){
		dev_err(dev, "%s: Couldn't retrieve the TCON interrupt\n", __func__);
		return irq;
	}

	ret = devm_request_irq(dev, irq, sunh5_tcon_handler, 0, dev_name(dev), tcon);
	if(ret){
		dev_err(dev, "%s: Couldn't request the IRQ\n", __func__);
		return ret;
	}

	return 0;
}

static struct sunh5_engine *sunh5_tcon_find_engine(struct sunh5_drv *drv, struct device_node *node)
{
	struct device_node *port, *ep, *remote;
	struct sunh5_engine *engine = ERR_PTR(-EINVAL);
//	u32 reg = 0;

	port = of_graph_get_port_by_id(node, 0);
	if(!port)
		return ERR_PTR(-EINVAL);
	
	if(of_get_available_child_count(port) != 1)
		goto out_put_port;

	/* Get the first connection without specifying an ID */
	ep = of_get_next_available_child(port, NULL);	
	if(!ep)
		goto out_put_port;

	remote = of_graph_get_remote_port_parent(ep);
	if(!remote)
		goto out_put_ep;

//	printk("of_get_available_child_count(port)=%d	remote->name=%s\n",of_get_available_child_count(port), remote->name);

	/* does this node match any registered engines? */
	list_for_each_entry(engine, &drv->engine_list, list){
//		printk("engine->node->name=%s\n",engine->node->name);
		if(remote == engine->node)
			goto out_put_remote;
	}
	

out_put_remote:
	of_node_put(remote);
out_put_ep:
	of_node_put(ep);
out_put_port:
	of_node_put(port);

	return engine;
}

static int sunh5_tcon_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct sunh5_drv *drv = drm->dev_private;
	struct sunh5_engine *engine;
//	struct device_node *remote;
	struct sunh5_tcon *tcon;
//	struct reset_control *edp_rstc;
//	bool has_lvds_rst, has_lvds_alt;
	int ret;

	dev_dbg(dev, "%s: Enter\n", __func__);

	engine = sunh5_tcon_find_engine(drv, dev->of_node);
	if(IS_ERR(engine)){
		dev_err(dev, "%s: Couldn't find matching engine\n", __func__);
		return -EPROBE_DEFER;
	}

	tcon = devm_kzalloc(dev, sizeof(*tcon), GFP_KERNEL);
	if(!tcon)
		return -ENOMEM;
	dev_set_drvdata(dev, tcon);
	tcon->drm = drm;
	tcon->dev = dev;
	tcon->id = engine->id;
	tcon->quirks = of_device_get_match_data(dev);

	tcon->lcd_rst = devm_reset_control_get(dev, "lcd");
	if(IS_ERR(tcon->lcd_rst)){
		dev_err(dev, "%s: Couldn't get our reset line\n", __func__);
		return PTR_ERR(tcon->lcd_rst);
	}

	/* Make sure our TCON is reset */
	ret = reset_control_reset(tcon->lcd_rst);				/* 提供从高到低的信号来充值IP */
	if(ret){
		dev_err(dev, "%s: Couldn't deassert our reset line\n", __func__);
		return ret;
	}

	ret = sunh5_tcon_init_clocks(dev, tcon);
	if(ret){
		dev_err(dev, "%s: Couldn't init our TCON clocks\n", __func__);
		goto err_assert_reset;
	}

	ret = sunh5_tcon_init_regmap(dev, tcon);
	if(ret){
		dev_err(dev, "%s: Couldn't init our TCON regmap\n", __func__);
		goto err_free_clocks;
	}

	ret = sunh5_tcon_init_irq(dev, tcon);
	if(ret){
		dev_err(dev, "%s: Couldn't init our TCON interrupts\n", __func__);
		goto err_free_clocks;
	}

	tcon->crtc = sunh5_crtc_init(drm, engine, tcon);
	if(IS_ERR(tcon->crtc)){
		dev_err(dev, "%s: Couldn't create our CRTC\n", __func__);
		ret = PTR_ERR(tcon->crtc);
		goto err_free_clocks;
	}

	list_add_tail(&tcon->list, &drv->tcon_list);

	return 0;

err_free_clocks:
	sunh5_tcon_free_clocks(tcon);
err_assert_reset:
	reset_control_assert(tcon->lcd_rst);

	return ret;
}

static void sunh5_tcon_unbind(struct device *dev, struct device *master, void *data)
{
	struct sunh5_tcon *tcon = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: Enter\n", __func__);

	list_del(&tcon->list);
	sunh5_tcon_free_clocks(tcon);
}

static const struct component_ops sunh5_tcon_ops = {
	.bind	= sunh5_tcon_bind,
	.unbind	= sunh5_tcon_unbind,
};

static int sunh5_tcon_probe(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	return component_add(&pdev->dev, &sunh5_tcon_ops);
}

static int sunh5_tcon_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "%s: Enter\n", __func__);

	component_del(&pdev->dev, &sunh5_tcon_ops);

	return 0;
}

static const struct sunh5_tcon_quirks sunh5_tv_quirks = {
	.has_channel_1	= true,
};

const struct of_device_id sunh5_tcon_of_table[] = {
	{ .compatible = "allwinner,sunh5-tcon-tv", .data = &sunh5_tv_quirks },
	{  }
};
MODULE_DEVICE_TABLE(of, sunh5_tcon_of_table);

static struct platform_driver sunh5_tcon_platform_driver = {
	.probe				= sunh5_tcon_probe,
	.remove				= sunh5_tcon_remove,
	.driver				= {
		.name			= "sunh5-tcon",
		.of_match_table	= sunh5_tcon_of_table,
	},
};
module_platform_driver(sunh5_tcon_platform_driver);

MODULE_DESCRIPTION("Allwinner H5 Timing Controller Driver");
MODULE_AUTHOR("GinPot <GinPot@xx.com>");
MODULE_LICENSE("GPL");
