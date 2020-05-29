#include <linux/version.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/simple_card_utils.h>


#define PREFIX	"ac108-voice-card,"
#define CELL	"#sound-dai-cells"
#define DAI		"sound-dai"

#define ac108_card_to_dev(ac108_card)		(ac108_card->snd_card.dev)
#define ac108_card_to_link(ac108_card, i)	(ac108_card->snd_card.dai_link + i)
#define ac108_card_to_props(ac108_card, i)	(ac108_card->dai_props + i)

struct ac108_card_priv {
	struct snd_soc_card snd_card;
	struct snd_soc_dai_link *dai_link;
	struct ac108_dai_props {
		struct asoc_simple_dai cpu_dai;
		struct asoc_simple_dai codec_dai;
		unsigned int mclk_fs;
	} *dai_props;

	unsigned int mclk_fs;
	unsigned channels_playback_default;
	unsigned channels_playback_override;
	unsigned channels_capture_default;
	unsigned channels_capture_override;

	struct work_struct work_codec_clk;
	spinlock_t lock;
	
};



static int asoc_simple_parse_dai(struct device_node *node,
							struct snd_soc_dai_link_component *dlc,
							struct device_node **dai_of_node,
							const char **dai_name,
							int *is_single_link)
{
	struct of_phandle_args args;
	int ret;

	if(!node)
		return 0;

	/* 
	 * Use snd_soc_dai_link_component instead of legacy style.
	 * It is only for codec, but cpu will be supported in the futurn.
	 * see 
	 *  soc-core.c: snd_soc_init_multicodec()
	 */
	if(dlc)
	{
		dai_name	= &dlc->dai_name;
		dai_of_node	= &dlc->of_node;
	}

	/*
	 * Get node via "sound-dai = <&phandle port>"
	 * it will be used as xxx_of_node on soc_bind_dai_link()
	 */
	ret = of_parse_phandle_with_args(node, DAI, CELL, 0, &args);

	/* Get dai->name */
	if(dai_name)
	{
		ret = snd_soc_of_get_dai_name(node, dai_name);
		if(ret < 0){
			return ret;
		}
	}

	*dai_of_node = args.np;

	if(is_single_link)
		*is_single_link = !args.args_count;


	return 0;
}

static int ac108_card_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct ac108_card_priv *ac108_card = snd_soc_card_get_drvdata(rtd->card);
	struct ac108_dai_props *dai_props = ac108_card_to_props(ac108_card, rtd->num);
	int ret;

	dev_dbg(ac108_card->snd_card.dev, "%s - entern\n", __func__);

	ret = clk_prepare_enable(dai_props->cpu_dai.clk);
	if(ret)
	{
		dev_dbg(ac108_card->snd_card.dev, "%s - enable cpu clk fail\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(dai_props->codec_dai.clk);
	if(ret)
	{
		dev_dbg(ac108_card->snd_card.dev, "%s - enable codec clk fail\n", __func__);
		clk_disable_unprepare(dai_props->cpu_dai.clk);
		return ret;
	}

	if(rtd->cpu_dai->driver->playback.channels_min)
		ac108_card->channels_playback_default = rtd->cpu_dai->driver->playback.channels_min;
	if(rtd->cpu_dai->driver->capture.channels_min)
		ac108_card->channels_capture_default = rtd->cpu_dai->driver->capture.channels_min;

	rtd->cpu_dai->driver->playback.channels_min = ac108_card->channels_playback_override;
	rtd->cpu_dai->driver->playback.channels_max = ac108_card->channels_playback_override;
	rtd->cpu_dai->driver->capture.channels_min = ac108_card->channels_capture_override;
	rtd->cpu_dai->driver->capture.channels_max = ac108_card->channels_capture_override;	

	return ret;
}

static void ac108_card_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct ac108_card_priv *ac108_card = snd_soc_card_get_drvdata(rtd->card);
	struct ac108_dai_props *dai_props = ac108_card_to_props(ac108_card, rtd->num);

	dev_dbg(ac108_card->snd_card.dev, "%s - entern\n", __func__);

	rtd->cpu_dai->driver->playback.channels_min = ac108_card->channels_playback_default;
	rtd->cpu_dai->driver->playback.channels_max = ac108_card->channels_playback_default;
	rtd->cpu_dai->driver->capture.channels_min = ac108_card->channels_capture_default;
	rtd->cpu_dai->driver->capture.channels_max = ac108_card->channels_capture_default;

	clk_disable_unprepare(dai_props->cpu_dai.clk);
	clk_disable_unprepare(dai_props->codec_dai.clk);
}

static int ac108_card_hw_params(struct snd_pcm_substream *substream,
										struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct ac108_card_priv *ac108_card = snd_soc_card_get_drvdata(rtd->card);
	struct ac108_dai_props *dai_props = ac108_card_to_props(ac108_card, rtd->num);
	unsigned int mclk, mclk_fs = 0;
	int ret = 0;

	dev_dbg(ac108_card->snd_card.dev, "%s - entern  ac108_card->mclk_fs=%d	dai_props->mclk_fs=%d\n", __func__, ac108_card->mclk_fs, dai_props->mclk_fs);

	if(ac108_card->mclk_fs)
		mclk_fs = ac108_card->mclk_fs;
	else
		if(dai_props->mclk_fs)
			mclk_fs = dai_props->mclk_fs;

	if(mclk_fs)
	{
		mclk = params_rate(params) * mclk_fs;
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
		if(ret && ret != -ENOTSUPP)
			goto err;

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
		if(ret && ret != -ENOTSUPP)
			goto err;
	}
	return 0;

err:
	return ret;
}

/*
#define _SET_CLOCK_CNT	2
static int (* _set_colck[_SET_CLOCK_CNT])(int y_start_n_stop);
*/

static int ac108_card_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->codec_dai;
//	struct ac108_card_priv *ac108_card = snd_soc_card_get_drvdata(rtd->card);
	int ret = 0;

	dev_dbg(rtd->card->dev, "%s() stream=%s cmd=%d play:%d, capt:%d\n",
								__func__, snd_pcm_stream_str(substream), cmd,
								cpu_dai->playback_active, cpu_dai->capture_active);

	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			//if(cancel_work_sync(&ac108_card->work_codec_clk) != 0) {}
			//spin_lock_irqsave(_set_clock);
			break;

		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:

			break;

		default:
			ret = -EINVAL;
	}

	return ret;
}

static const struct snd_soc_ops ac108_card_ops = {
	.startup 	= ac108_card_startup,
	.shutdown 	= ac108_card_shutdown,
	.hw_params	= ac108_card_hw_params,
	.trigger	= ac108_card_trigger,
};

static int ac108_card_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct ac108_card_priv *ac108_card = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct ac108_dai_props *dai_props = ac108_card_to_props(ac108_card, rtd->num);
	int ret;

	dev_dbg(ac108_card->snd_card.dev, "%s - entern\n", __func__);

	ret = asoc_simple_init_dai(codec_dai, &dai_props->codec_dai);
	if(ret < 0)
		return ret;

	ret = asoc_simple_init_dai(cpu_dai, &dai_props->cpu_dai);
	if(ret < 0)
		return ret;

	dev_dbg(ac108_card->snd_card.dev, "%s - quit\n", __func__);
	
	return 0;
}

static int ac108_card_dai_link_of(struct device_node *node,
										struct ac108_card_priv *ac108_card,
										int idx, bool is_top_level_node)
{
	struct device *dev = ac108_card_to_dev(ac108_card);
	struct snd_soc_dai_link *dai_link= ac108_card_to_link(ac108_card, idx);
	struct ac108_dai_props *dai_props = ac108_card_to_props(ac108_card, idx);
	struct asoc_simple_dai *cpu_dai = &dai_props->cpu_dai;
	struct asoc_simple_dai *codec_dai = &dai_props->codec_dai;
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char prop[128];
	char *prefix = "";
	int ret = 0, single_cpu;

	dev_dbg(ac108_card->snd_card.dev, "%s - entern\n", __func__);

	if(is_top_level_node)
		prefix = PREFIX;

	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(node, prop);


	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(node, prop);
	if(!cpu)
	{
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		return -EINVAL;
	}

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(node, prop);
	if(!codec)
	{
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_get_cpu_err;
	}

	/*
	 * get dai_fmt
	 * 0~3bit:   i2s/right_j/left_j/dsp_a/dsp_b		xxx,format
	 * 4~7bit:   continuous or gated clock			xxx,continuous-clock
	 * 8~11bit:  BCLK/frame polarity				xxx,bitclock-inversion; xxx,frame-inversion
	 * 12~15bit: slave/master						xxx,bitclock-master; xxx,frame-master
	 */
	ret = asoc_simple_parse_daifmt(dev, node, codec, prefix, &dai_link->dai_fmt);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get dai fmt fail\n", __func__);
		goto dai_link_of_err;
	}

	of_property_read_u32(node, "mclk-fs", &dai_props->mclk_fs);

	ret = asoc_simple_parse_cpu(cpu, dai_link, &single_cpu);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get cpu dai fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_codec(codec, dai_link);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get codec dai fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_platform(plat, dai_link);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get platform fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = snd_soc_of_parse_tdm_slot(cpu, &cpu_dai->tx_slot_mask,
									&cpu_dai->rx_slot_mask,
									&cpu_dai->slots,
									&cpu_dai->slot_width);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get cpu tdm slot fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = snd_soc_of_parse_tdm_slot(codec, &codec_dai->tx_slot_mask,
									&codec_dai->rx_slot_mask,
									&codec_dai->slots,
									&codec_dai->slot_width);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get codec tdm slot fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_clk_cpu(dev, cpu, dai_link, cpu_dai);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Get cpu clk fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_clk_codec(dev, codec, dai_link, codec_dai);
	if(ret < 0)
	{
		dev_dbg(dev, "%s Get codec clk fail\n", __func__);
		goto dai_link_of_err;
	}

	ret = asoc_simple_set_dailink_name(dev, dai_link, "%s-%s", dai_link->cpu_dai_name, dai_link->codec_dai_name);
	if(ret < 0)
	{
		dev_dbg(dev, "%s: Set dailink name fail\n", __func__);
		goto dai_link_of_err;
	}

	/* Assumes platform == cpu */
//	asoc_simple_canonicalize_platform(dai_link);
	asoc_simple_canonicalize_cpu(dai_link, single_cpu);

	dai_link->ops = &ac108_card_ops;
	dai_link->init = ac108_card_dai_init;

	dev_dbg(dev, "\tname : %s\n", dai_link->platform_name);
	dev_dbg(dev, "\tname : %s\n", dai_link->stream_name);
	dev_dbg(dev, "\tformat : %04x\n", dai_link->dai_fmt);
	dev_dbg(dev, "\tcpu : %s / %d\n", dai_link->cpu_dai_name, dai_props->cpu_dai.sysclk);
	dev_dbg(dev, "\tcodec : %s / %d\n", dai_link->codec_dai_name, dai_props->codec_dai.sysclk);

dai_link_of_err:
	of_node_put(codec);

dai_link_get_cpu_err:
	of_node_put(cpu);
	
	return ret;
}

static int ac108_card_parse_aux_devs(struct device_node *node,
											struct ac108_card_priv *ac108_card)
{
//	struct device *dev = ac108_card_to_dev(ac108_card);
//	struct device_node *aux_node;
	int len;
	
	if(!of_find_property(node, PREFIX "aux-devs", &len))
		return 0;	/* Ok to have no aux-devs */

	return 0;
}

static int ac108_card_parse_of(struct device_node *node, struct ac108_card_priv *ac108_card)
{
	struct device_node *dai_link;
	int ret = 0;

	dev_dbg(ac108_card->snd_card.dev, "%s - entern\n", __func__);

	dai_link = of_get_child_by_name(node, PREFIX "dai-link");

	/* The off-codec widgets */
	if(of_property_read_bool(node, PREFIX "widgets"))
	{
		ret = snd_soc_of_parse_audio_simple_widgets(&ac108_card->snd_card, PREFIX "widgets");
		if(ret)
			goto card_parse_end;
	}

	/* DAPM routes */
	if(of_property_read_bool(node, PREFIX "routing"))
	{
		ret = snd_soc_of_parse_audio_routing(&ac108_card->snd_card, PREFIX "routing");
		if(ret)
			goto card_parse_end;
	}

	/* Factor to mclk, used in hw_params */
	of_property_read_u32(node, PREFIX "mclk-fs", &ac108_card->mclk_fs);

	/* Single/Muti DAI link(s) & New style of DT node */
	if(dai_link)
	{
		;
	}
	else
	{
		/* For single DAI link & old style of DT node */
		ret = ac108_card_dai_link_of(node, ac108_card, 0, true);
		if(ret < 0)
			goto card_parse_end;
	}

	ret = asoc_simple_parse_card_name(&ac108_card->snd_card, PREFIX);
	if(ret < 0)
		goto card_parse_end;

	ret = ac108_card_parse_aux_devs(node, ac108_card);

	ac108_card->channels_playback_default = 0;
	ac108_card->channels_playback_override = 2;
	ac108_card->channels_capture_default = 0;
	ac108_card->channels_capture_override = 2;
	of_property_read_u32(node, PREFIX "channels-playback-default",
							&ac108_card->channels_playback_default);
	of_property_read_u32(node, PREFIX "channels-playback-override",
							&ac108_card->channels_playback_override);
	of_property_read_u32(node, PREFIX "channels-capture-default",
							&ac108_card->channels_capture_default);
	of_property_read_u32(node, PREFIX "channels-capture-override",
							&ac108_card->channels_capture_override);


card_parse_end:
	of_node_put(dai_link);

	return ret;
}

static int ac108_card_probe(struct platform_device *pdev)
{
	struct ac108_card_priv *ac108_card;
	struct ac108_dai_props *dai_props;
	struct snd_soc_dai_link *dai_link;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0, num;

	dev_dbg(&pdev->dev, "%s - %s entern\n", __func__, pdev->name);

	/* Get the number of DAI links */
	if(np && of_get_child_by_name(np, "dai-link"))
		num = of_get_child_count(np);
	else
		num = 1;

	/* allocate the private data and the DAI link array */
	ac108_card = devm_kzalloc(&pdev->dev, sizeof(*ac108_card), GFP_KERNEL);
	if(!ac108_card)
	{
		dev_dbg(&pdev->dev, "Unable to alloc ac108 card private data\n");
		return -ENOMEM;
	}

	dai_props = devm_kzalloc(&pdev->dev, sizeof(*dai_props) * num, GFP_KERNEL);
	dai_link = devm_kzalloc(&pdev->dev, sizeof(*dai_link) * num, GFP_KERNEL);
	if(!dai_props || !dai_link)
	{
		dev_dbg(&pdev->dev, "Unable to alloc dai props and link data\n");
		return -ENOMEM;
	}

	ac108_card->dai_props = dai_props;
	ac108_card->dai_link = dai_link;

	/* Init snd_soc_card */
	ac108_card->snd_card.owner 		= THIS_MODULE;
	ac108_card->snd_card.dev 		= &pdev->dev;
	ac108_card->snd_card.dai_link	= ac108_card->dai_link;
	ac108_card->snd_card.num_links	= num;

	if(np && of_device_is_available(np))
	{
		ret = ac108_card_parse_of(np, ac108_card);
		if(ret < 0)
		{
			if(ret != -EPROBE_DEFER)
				dev_err(&pdev->dev, "parse error %d\n", ret);
			goto err;
		}
	}

//	spin_lock_init(&ac108_card->lock);

	snd_soc_card_set_drvdata(&ac108_card->snd_card, ac108_card);

	ret = devm_snd_soc_register_card(&pdev->dev, &ac108_card->snd_card);
	if(ret < 0)
		goto err;

	dev_dbg(&pdev->dev, "%s - initialization done\n", __func__);

	return ret;

err:
	asoc_simple_clean_reference(&ac108_card->snd_card);

	return ret;
}

static int ac108_card_remove(struct platform_device *pdev)
{

	dev_dbg(&pdev->dev, "%s - initialization done\n", __func__);
	
	return 0;
}

static const struct of_device_id ac108_card_of_match[] = {
	{ .compatible = "ac108-card", },
	{},
};
MODULE_DEVICE_TABLE(of, ac108_card_of_match);

static struct platform_driver ac108_card = {
	.driver = {
		.name = "ac108-card",
		.pm = &snd_soc_pm_ops,
		.of_match_table = ac108_card_of_match,
	},
	.probe 	= ac108_card_probe,
	.remove = ac108_card_remove,
};

module_platform_driver(ac108_card);

MODULE_DESCRIPTION("AC108 Card driver");
MODULE_AUTHOR("GinPot<GinPot@xx.com>");
MODULE_LICENSE("GPL");
