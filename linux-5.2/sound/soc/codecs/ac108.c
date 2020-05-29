#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "ac108.h"

static struct ac108_priv *ac108;

static const struct real_val_to_reg_val ac108_samp_res[] = {
	{ 8,			1 },
	{ 12,			2 },
	{ 16,			3 },
	{ 20,			4 },
	{ 24,			5 },
	{ 28,			6 },
	{ 32,			7 },
};

static const struct real_val_to_reg_val ac108_sample_rate[] = {
	{ 8000,			0 },
	{ 11025,		1 },
	{ 12000,		2 },
	{ 16000,		3 },
	{ 22050,		4 },
	{ 24000,		5 },
	{ 32000,		6 },
	{ 44100,		7 },
	{ 48000,		8 },
	{ 96000,		9 },
};

static const unsigned ac108_bclkdivs[] = {
	0,		1,		2,		4,
	6,		8,		12,		16,
	24,		32,		48,		64,
	96,		128,	176,	192,
};


/* FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ;	M1[0,31],  M2[0,1],  N[0,1023],  K1[0,31],  K2[0,1] */
static const struct pll_div ac108_pll_div_list[] = {	
	{ 400000,   _FREQ_24_576K, 0,  0, 614, 4, 1 },	
	{ 512000,   _FREQ_24_576K, 0,  0, 960, 9, 1 }, //_FREQ_24_576K/48	
	{ 768000,   _FREQ_24_576K, 0,  0, 640, 9, 1 }, //_FREQ_24_576K/32	
	{ 800000,   _FREQ_24_576K, 0,  0, 614, 9, 1 },	
	{ 1024000,  _FREQ_24_576K, 0,  0, 480, 9, 1 }, //_FREQ_24_576K/24	
	{ 1600000,  _FREQ_24_576K, 0,  0, 307, 9, 1 },	
	{ 2048000,  _FREQ_24_576K, 0,  0, 240, 9, 1 }, /* accurate,  8000 * 256 */	
	{ 3072000,  _FREQ_24_576K, 0,  0, 160, 9, 1 }, /* accurate, 12000 * 256 */	
	{ 4096000,  _FREQ_24_576K, 2,  0, 360, 9, 1 }, /* accurate, 16000 * 256 */	
	{ 6000000,  _FREQ_24_576K, 4,  0, 410, 9, 1 },	
	{ 12000000, _FREQ_24_576K, 9,  0, 410, 9, 1 },	
	{ 13000000, _FREQ_24_576K, 8,  0, 340, 9, 1 },	
	{ 15360000, _FREQ_24_576K, 12, 0, 415, 9, 1 },	
	{ 16000000, _FREQ_24_576K, 12, 0, 400, 9, 1 },	
	{ 19200000, _FREQ_24_576K, 15, 0, 410, 9, 1 },	
	{ 19680000, _FREQ_24_576K, 15, 0, 400, 9, 1 },	
	{ 24000000, _FREQ_24_576K, 4,  0, 128,24, 0 }, // accurate, 24M -> 24.576M */	

	{ 400000,   _FREQ_22_579K, 0,  0, 566, 4, 1 },	
	{ 512000,   _FREQ_22_579K, 0,  0, 880, 9, 1 },	
	{ 768000,   _FREQ_22_579K, 0,  0, 587, 9, 1 },	
	{ 800000,   _FREQ_22_579K, 0,  0, 567, 9, 1 },	
	{ 1024000,  _FREQ_22_579K, 0,  0, 440, 9, 1 },	
	{ 1600000,  _FREQ_22_579K, 1,  0, 567, 9, 1 },	
	{ 2048000,  _FREQ_22_579K, 0,  0, 220, 9, 1 },	
	{ 3072000,  _FREQ_22_579K, 0,  0, 148, 9, 1 },	
	{ 4096000,  _FREQ_22_579K, 2,  0, 330, 9, 1 },	
	{ 6000000,  _FREQ_22_579K, 2,  0, 227, 9, 1 },	
	{ 12000000, _FREQ_22_579K, 8,  0, 340, 9, 1 },	
	{ 13000000, _FREQ_22_579K, 9,  0, 350, 9, 1 },	
	{ 15360000, _FREQ_22_579K, 10, 0, 325, 9, 1 },	
	{ 16000000, _FREQ_22_579K, 11, 0, 340, 9, 1 },	
	{ 19200000, _FREQ_22_579K, 13, 0, 330, 9, 1 },	
	{ 19680000, _FREQ_22_579K, 14, 0, 345, 9, 1 },	
	{ 24000000, _FREQ_22_579K, 24, 0, 588,24, 0 }, // accurate, 24M -> 22.5792M */	

	{ _FREQ_24_576K / 1,   _FREQ_24_576K, 9,  0, 200, 9, 1 }, //_FREQ_24_576K	
	{ _FREQ_24_576K / 2,   _FREQ_24_576K, 9,  0, 400, 9, 1 }, /*12288000,accurate, 48000 * 256 */	
	{ _FREQ_24_576K / 4,   _FREQ_24_576K, 4,  0, 400, 9, 1 }, /*6144000, accurate, 24000 * 256 */	
	{ _FREQ_24_576K / 16,  _FREQ_24_576K, 0,  0, 320, 9, 1 }, //1536000	
	{ _FREQ_24_576K / 64,  _FREQ_24_576K, 0,  0, 640, 4, 1 }, //384000	
	{ _FREQ_24_576K / 96,  _FREQ_24_576K, 0,  0, 960, 4, 1 }, //256000	
	{ _FREQ_24_576K / 128, _FREQ_24_576K, 0,  0, 512, 1, 1 }, //192000	
	{ _FREQ_24_576K / 176, _FREQ_24_576K, 0,  0, 880, 4, 0 }, //140000
	{ _FREQ_24_576K / 192, _FREQ_24_576K, 0,  0, 960, 4, 0 }, //128000
	
	{ _FREQ_22_579K / 1,   _FREQ_22_579K, 9,  0, 200, 9, 1 }, //_FREQ_22_579K	
	{ _FREQ_22_579K / 2,   _FREQ_22_579K, 9,  0, 400, 9, 1 }, /*11289600,accurate, 44100 * 256 */	
	{ _FREQ_22_579K / 4,   _FREQ_22_579K, 4,  0, 400, 9, 1 }, /*5644800, accurate, 22050 * 256 */	
	{ _FREQ_22_579K / 16,  _FREQ_22_579K, 0,  0, 320, 9, 1 }, //1411200	
	{ _FREQ_22_579K / 64,  _FREQ_22_579K, 0,  0, 640, 4, 1 }, //352800	
	{ _FREQ_22_579K / 96,  _FREQ_22_579K, 0,  0, 960, 4, 1 }, //235200	
	{ _FREQ_22_579K / 128, _FREQ_22_579K, 0,  0, 512, 1, 1 }, //176400	
	{ _FREQ_22_579K / 176, _FREQ_22_579K, 0,  0, 880, 4, 0 }, //128290	
	{ _FREQ_22_579K / 192, _FREQ_22_579K, 0,  0, 960, 4, 0 }, //117600	
	
	{ _FREQ_22_579K / 6,   _FREQ_22_579K, 2,  0, 360, 9, 1 }, //3763200	
	{ _FREQ_22_579K / 8,   _FREQ_22_579K, 0,  0, 160, 9, 1 }, /*2822400, accurate, 11025 * 256 */	
	{ _FREQ_22_579K / 12,  _FREQ_22_579K, 0,  0, 240, 9, 1 }, //1881600	
	{ _FREQ_22_579K / 24,  _FREQ_22_579K, 0,  0, 480, 9, 1 }, //940800	
	{ _FREQ_22_579K / 32,  _FREQ_22_579K, 0,  0, 640, 9, 1 }, //705600	
	{ _FREQ_22_579K / 48,  _FREQ_22_579K, 0,  0, 960, 9, 1 }, //470400
};

/**
 * snd_ac108_get_volsw - single mixer get callback
 * @kcontrol: mixer control
 * @ucontrol: control element information
 *
 * Callback to get the value of a single mixer control, or a double mixer
 * control that spans 2 registers.
 *
 * Returns 0 for success.
 */
static int snd_ac108_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol
){
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int mask = (1 << fls(mc->max)) - 1;
	unsigned int invert = mc->invert;
	int ret;
	unsigned int val;

	if ((ret = regmap_read(ac108->i2cmap, mc->reg, &val)) < 0)
		return ret;

	val = ((val >> mc->shift) & mask) - mc->min;
	if (invert) {
		val = mc->max - val;
	}
	ucontrol->value.integer.value[0] = val;
	return 0;
}

/**
 * snd_ac108_put_volsw - single mixer put callback
 * @kcontrol: mixer control
 * @ucontrol: control element information
 *
 * Callback to set the value of a single mixer control, or a double mixer
 * control that spans 2 registers.
 *
 * Returns 0 for success.
 */
static int snd_ac108_put_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol
){
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int sign_bit = mc->sign_bit;
	unsigned int val, mask = (1 << fls(mc->max)) - 1;
	unsigned int invert = mc->invert;
	int ret;

	if (sign_bit)
		mask = BIT(sign_bit + 1) - 1;

	val = ((ucontrol->value.integer.value[0] + mc->min) & mask);
	if (invert) {
		val = mc->max - val;
	}

	mask = mask << mc->shift;
	val = val << mc->shift;

	ret = regmap_update_bits(ac108->i2cmap, mc->reg, mask, val);
	return ret;
}


#define SOC_AC108_SINGLE_TLV(xname, reg, shift, max, invert, chip, tlv_array) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		 SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw, .get = snd_ac108_get_volsw,\
	.put = snd_ac108_put_volsw, \
	.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert, chip) }

static const DECLARE_TLV_DB_SCALE(tlv_adc_pga_gain, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(tlv_ch_digital_vol, -11925,75,0);


/* single ac108 */
static const struct snd_kcontrol_new ac108_snd_controls[] = {
	/* ### chip 0 ### */
	/*0x70: ADC1 Digital Channel Volume Control Register*/
	SOC_AC108_SINGLE_TLV("CH1 digital volume", ADC1_DVOL_CTRL, 0, 0xff, 0, 0, tlv_ch_digital_vol),
	/*0x71: ADC2 Digital Channel Volume Control Register*/
	SOC_AC108_SINGLE_TLV("CH2 digital volume", ADC2_DVOL_CTRL, 0, 0xff, 0, 0, tlv_ch_digital_vol),
	/*0x72: ADC3 Digital Channel Volume Control Register*/
	SOC_AC108_SINGLE_TLV("CH3 digital volume", ADC3_DVOL_CTRL, 0, 0xff, 0, 0, tlv_ch_digital_vol),
	/*0x73: ADC4 Digital Channel Volume Control Register*/
	SOC_AC108_SINGLE_TLV("CH4 digital volume", ADC4_DVOL_CTRL, 0, 0xff, 0, 0, tlv_ch_digital_vol),

	/*0x90: Analog PGA1 Control Register*/
	SOC_AC108_SINGLE_TLV("ADC1 PGA gain", ANA_PGA1_CTRL, ADC1_ANALOG_PGA, 0x1f, 0, 0, tlv_adc_pga_gain),
	/*0x91: Analog PGA2 Control Register*/
	SOC_AC108_SINGLE_TLV("ADC2 PGA gain", ANA_PGA2_CTRL, ADC2_ANALOG_PGA, 0x1f, 0, 0, tlv_adc_pga_gain),
	/*0x92: Analog PGA3 Control Register*/
	SOC_AC108_SINGLE_TLV("ADC3 PGA gain", ANA_PGA3_CTRL, ADC3_ANALOG_PGA, 0x1f, 0, 0, tlv_adc_pga_gain),
	/*0x93: Analog PGA4 Control Register*/
	SOC_AC108_SINGLE_TLV("ADC4 PGA gain", ANA_PGA4_CTRL, ADC4_ANALOG_PGA, 0x1f, 0, 0, tlv_adc_pga_gain),
};


static const struct snd_soc_dapm_widget ac108_dapm_widgets[] = {
	//input widgets
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_INPUT("MIC2P"),
	SND_SOC_DAPM_INPUT("MIC2N"),

	SND_SOC_DAPM_INPUT("MIC3P"),
	SND_SOC_DAPM_INPUT("MIC3N"),

	SND_SOC_DAPM_INPUT("MIC4P"),
	SND_SOC_DAPM_INPUT("MIC4N"),

	SND_SOC_DAPM_INPUT("DMIC1"),
	SND_SOC_DAPM_INPUT("DMIC2"),

	/*0xa0: ADC1 Analog Control 1 Register*/
	/*0xa1-0xa6:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 1 AAF", "Capture", 0, ANA_ADC1_CTRL1, ADC1_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 1 EN", ANA_ADC1_CTRL1, ADC1_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC1BIAS", ANA_ADC1_CTRL1, ADC1_MICBIAS_EN, 1),

	/*0xa7: ADC2 Analog Control 1 Register*/
	/*0xa8-0xad:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 2 AAF", "Capture", 0, ANA_ADC2_CTRL1, ADC2_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 2 EN", ANA_ADC2_CTRL1, ADC2_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC2BIAS", ANA_ADC2_CTRL1, ADC2_MICBIAS_EN, 1),

	/*0xae: ADC3 Analog Control 1 Register*/
	/*0xaf-0xb4:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 3 AAF", "Capture", 0, ANA_ADC3_CTRL1, ADC3_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 3 EN", ANA_ADC3_CTRL1, ADC3_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC3BIAS", ANA_ADC3_CTRL1, ADC3_MICBIAS_EN, 1),

	/*0xb5: ADC4 Analog Control 1 Register*/
	/*0xb6-0xbb:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 4 AAF", "Capture", 0, ANA_ADC4_CTRL1, ADC4_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 4 EN", ANA_ADC4_CTRL1, ADC4_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC4BIAS", ANA_ADC4_CTRL1, ADC4_MICBIAS_EN, 1),


	/*0x61: ADC Digital Part Enable Register*/
	SND_SOC_DAPM_SUPPLY("ADC EN", ADC_DIG_EN, 4,  1, NULL, 0),
	SND_SOC_DAPM_ADC("ADC1", "Capture", ADC_DIG_EN, 0,  1),
	SND_SOC_DAPM_ADC("ADC2", "Capture", ADC_DIG_EN, 1,  1),
	SND_SOC_DAPM_ADC("ADC3", "Capture", ADC_DIG_EN, 2,  1),
	SND_SOC_DAPM_ADC("ADC4", "Capture", ADC_DIG_EN, 3,  1),

	SND_SOC_DAPM_SUPPLY("ADC1 CLK", ANA_ADC4_CTRL7, ADC1_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC2 CLK", ANA_ADC4_CTRL7, ADC2_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC3 CLK", ANA_ADC4_CTRL7, ADC3_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC4 CLK", ANA_ADC4_CTRL7, ADC4_CLK_GATING, 1, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DSM EN", ANA_ADC4_CTRL6, DSM_DEMOFF, 1, NULL, 0),

	/*0x62:Digital MIC Enable Register*/
	SND_SOC_DAPM_MICBIAS("DMIC1 enable", DMIC_EN, 0, 0),
	SND_SOC_DAPM_MICBIAS("DMIC2 enable", DMIC_EN, 1, 0),
};

static const struct snd_soc_dapm_route ac108_dapm_routes[] = {

	{ "ADC1", NULL, "Channel 1 AAF" },
	{ "ADC2", NULL, "Channel 2 AAF" },
	{ "ADC3", NULL, "Channel 3 AAF" },
	{ "ADC4", NULL, "Channel 4 AAF" },

	{ "Channel 1 AAF", NULL, "MIC1BIAS" },
	{ "Channel 2 AAF", NULL, "MIC2BIAS" },
	{ "Channel 3 AAF", NULL, "MIC3BIAS" },
	{ "Channel 4 AAF", NULL, "MIC4BIAS" },

	{ "MIC1BIAS", NULL, "ADC1 CLK" },
	{ "MIC2BIAS", NULL, "ADC2 CLK" },
	{ "MIC3BIAS", NULL, "ADC3 CLK" },
	{ "MIC4BIAS", NULL, "ADC4 CLK" },


	{ "ADC1 CLK", NULL, "DSM EN" },
	{ "ADC2 CLK", NULL, "DSM EN" },
	{ "ADC3 CLK", NULL, "DSM EN" },
	{ "ADC4 CLK", NULL, "DSM EN" },


	{ "DSM EN", NULL, "ADC EN" },

	{ "Channel 1 EN", NULL, "DSM EN" },
	{ "Channel 2 EN", NULL, "DSM EN" },
	{ "Channel 3 EN", NULL, "DSM EN" },
	{ "Channel 4 EN", NULL, "DSM EN" },


	{ "MIC1P", NULL, "Channel 1 EN" },
	{ "MIC1N", NULL, "Channel 1 EN" },

	{ "MIC2P", NULL, "Channel 2 EN" },
	{ "MIC2N", NULL, "Channel 2 EN" },

	{ "MIC3P", NULL, "Channel 3 EN" },
	{ "MIC3N", NULL, "Channel 3 EN" },

	{ "MIC4P", NULL, "Channel 4 EN" },
	{ "MIC4N", NULL, "Channel 4 EN" },

};




static int ac108_set_sysclk(struct snd_soc_dai * codec_dai,
									int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = codec_dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	int ret = 0;

	dev_dbg(component->dev, "%s: ID %d, freq %u\n", __func__, clk_id, freq);

	freq = 24000000;
	clk_id = SYSCLK_SRC_PLL;

	switch(clk_id)
	{
		case SYSCLK_SRC_MCLK:
			ret = regmap_update_bits(ac108->i2cmap, SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_MCLK << SYSCLK_SRC);
			if(ret != 0)
				dev_err(component->dev, "Setting sysclk src mclk fail\n");
			break;
		case SYSCLK_SRC_PLL:
			ret = regmap_update_bits(ac108->i2cmap, SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_PLL << SYSCLK_SRC);
			if(ret != 0)
				dev_err(component->dev, "Setting sysclk src pll faill\n");
			break;
		default:
			return -EINVAL;
	}

	ac108->mclk = freq;
	ac108->clk_id = clk_id;

	return ret;
}

/*
 * The clock management related registers are Reg20h~Reg25h
 * The PLL management related registers are Reg10h~Reg18h
 */
static int ac108_config_pll(struct ac108_priv *ac108, unsigned rate, unsigned lrck_ratio)
{
	struct pll_div ac108_pll_div = { 0 };
	unsigned int i, ret = 0;

	dev_dbg(&ac108->i2c->dev, "%s - Enter\n", __func__);

//	dev_dbg(ac108->i2c->dev, "%s - pll_id: %d	source: %d	freq_in: %d	freq_out: %d\n", __func__, pll_id, source, freq_in, freq_out);

	if(ac108->clk_id == SYSCLK_SRC_PLL)
	{
		unsigned pll_src, pll_freq_in;

		if(0)
		{
			/* use mclk(24M) -> pll -> sysclk */
			pll_freq_in = ac108->mclk;//rate * lrck_ratio;//
			pll_src = 0x00;//0x01;//
		}
		else
		{
			/* use Bclk -> pll -> sysclk */
			pll_freq_in = rate * lrck_ratio;//48000*4*32;//
			pll_src = 0x01;

		}

		/* FOUT = ( FIN * N )  / [ ( M1 + 1 ) * ( M2 + 1 ) * ( K1 + 1 ) * ( K2 + 1 ) ] */
		for(i = 0; i < ARRAY_SIZE(ac108_pll_div_list); i++)
			if(ac108_pll_div_list[i].freq_in == pll_freq_in)
			{
				ac108_pll_div = ac108_pll_div_list[i];
				dev_dbg(&ac108->i2c->dev, "AC108 PLL freq_in match: %u, freq_out: %u\n", ac108_pll_div.freq_in, ac108_pll_div.freq_out);
				break;
			}

		/* 0x11, 0x12, 0x13, 0x14: Config PLL DIV param M1/M2/N/K1/K2 */
		ret = regmap_update_bits(ac108->i2cmap, PLL_CTRL5, 0x1F << PLL_POSTDIV1 | 0x01 << PLL_POSTDIV2, ac108_pll_div.k1 << PLL_POSTDIV1 | ac108_pll_div.k2 << PLL_POSTDIV2);
		ret |= regmap_update_bits(ac108->i2cmap, PLL_CTRL4, 0xFF << PLL_LOOPDIV_LSB, (unsigned char)ac108_pll_div.n << PLL_LOOPDIV_LSB);
		ret |= regmap_update_bits(ac108->i2cmap, PLL_CTRL3, 0x03 << PLL_LOOPDIV_MSB, (ac108_pll_div.n >> 8) << PLL_LOOPDIV_MSB);
		ret |= regmap_update_bits(ac108->i2cmap, PLL_CTRL2, 0x1F << PLL_PREDIV1 | 0x01 << PLL_PREDIV2, ac108_pll_div.m1 << PLL_PREDIV1 | ac108_pll_div.m2 << PLL_PREDIV2);

		/* 0x18: PLL clk lock enable */
		ret |= regmap_update_bits(ac108->i2cmap, PLL_LOCK_CTRL, 0x1<<PLL_LOCK_EN, 0x1<<PLL_LOCK_EN);						//PLL CLK lock enable
		
		/*
		 * 0x20: enable pll, pll source from mclk, sysclk source from pll, enable sysclk.
		 */
		ret |= regmap_update_bits(ac108->i2cmap, SYSCLK_CTRL, 0x01 << PLLCLK_EN | 0x03 << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN,
												0x01 << PLLCLK_EN | pll_src << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN);


		ret |= regmap_update_bits(ac108->i2cmap, PLL_CTRL1, 0x1<<PLL_EN | 0x1<<PLL_COM_EN, 0x1<<PLL_EN | 0x1<<PLL_COM_EN);	//PLL Common voltage Enable, PLL Enable
												
		if(ret != 0)
		{
			dev_dbg(&ac108->i2c->dev, "setting pll fail :%d\n", ret);
			return ret;
		}

		ac108->sysclk = ac108_pll_div.freq_out;
	}

	dev_dbg(&ac108->i2c->dev, "%s - Quit\n", __func__);
	
	return ret;
}

static int ac108_multi_chips_slots(struct ac108_priv *ac108, int slots)
{
	int i, ret;

	dev_dbg(&ac108->i2c->dev, "%s: Enter\n", __func__);

	/*
	 * codec0 enable slots 2,3,0,1 when 1 codec
	 *
	 * codec0 enable slots 6,7,0,1 when 2 codec
	 * codec1 enable slots 2,3,4,5
	 */
	for(i = 0; i < ac108->codec_cnt; i++)
	{
		/* rotate map, due to channels rotated by CPU_DAI */
		const unsigned vec_mask[] = {
			0x3 << 6 | 0x3,		//slots 6,7,0,1
			0xF <<2,			//slots 2,3,4,5
			0,
			0,
		};
		const unsigned vec_maps[] = {
			/*
			 * chip 0,
			 * mic 0 sample -> slot 6
			 * mic 1 sample -> slot 7
			 * mic 2 sample -> slot 0
			 * mic 3 sample -> slot 1
			 */
			 0x0 << 12 | 0x1 << 14 | 0x2 << 0 | 0x3 <<2,

			/*
			 * chip 1,
			 * mic 0 sample -> slot 2
			 * mic 1 sample -> slot 3
			 * mic 2 smaple -> slot 4
			 * mic 3 smaple -> slot 5
			 */
			 0x0 <<4 | 0x1 << 6 | 0x2 << 8 | 0x3 << 10,
			 0,
			 0,
		};
		unsigned vec;

		/* 0x38-0x3A I2S_TX1_CTRLx */
		if(ac108->codec_cnt == 1)
			vec = 0xF;
		else
			vec = vec_mask[i];
		ret = regmap_write(ac108->i2cmap, I2S_TX1_CTRL1, slots - 1);
		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CTRL2, (vec >> 0) & 0xFF);
		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CTRL3, (vec >> 8) & 0xFF);

		/* 0x3C-0x3F I2S_TX1_CHMP_CTRLx */
		if(ac108->codec_cnt == 1)
			vec = (0x0 << 0 | 0x1 << 2 | 0x2 << 4 | 0x3 << 6);
		else
			if(ac108->codec_cnt == 2)
				vec = vec_maps[i];

		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CHMP_CTRL1, (vec >> 0)  & 0xFF);
		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CHMP_CTRL2, (vec >> 8)  & 0xFF);
		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CHMP_CTRL3, (vec >> 16) & 0xFF);
		ret |= regmap_write(ac108->i2cmap, I2S_TX1_CHMP_CTRL4, (vec >> 24) & 0xFF);

		if(ret < 0)
		{
			dev_err(&ac108->i2c->dev, "%s: setting slots fail\n", __func__);
			return ret;
		}
	}
	return 0;
}

static int ac108_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct snd_soc_component *component = dai->component;
//	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);

	dev_dbg(component->dev, "%s - Entern\n", __func__);

	return 0;
}

static int ac108_set_pll(struct snd_soc_dai *dai, int pll_id, int source, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;

	dev_dbg(component->dev, "%s - Enter\n", __func__);

	return 0;
}

static int ac108_hw_params(struct snd_pcm_substream *substream,
							struct snd_pcm_hw_params *params,
							struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	unsigned int i, channels, samp_res, rate = 0, ret = 0;
//	unsigned bclkdiv;

	dev_dbg(component->dev, "%s()  stream=%s play:%d capt:%d\n", __func__,
										snd_pcm_stream_str(substream),
										dai->playback_active, dai->capture_active);

	channels = params_channels(params);

	/* Master mode, to clear cpu_dai fifos, output bclk witchout lrck */
	switch(params_format(params))
	{
		case SNDRV_PCM_FORMAT_S8:
			samp_res = 0;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			samp_res = 2;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			samp_res = 3;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			samp_res = 4;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			samp_res = 6;
			break;
		default:
			dev_err(component->dev, "AC108 don't supported the sample resolution: %u\n", params_format(params));
			return -EINVAL;
	}

	for(i = 0; i < ARRAY_SIZE(ac108_sample_rate); i++)
		if(ac108_sample_rate[i].real_val == params_rate(params))
		{
			rate = i;
			break;
		}
	if(i >= ARRAY_SIZE(ac108_sample_rate))
		return -EINVAL;
	
	dev_dbg(component->dev, "rate: %d, channels: %d, samp_res: %d\n",
								ac108_sample_rate[rate].real_val,
								channels,
								ac108_samp_res[samp_res].real_val);

	/*
	  * 0x32, 0x33
	  * It is used to program the number of BCLKs per channel of sample frame
	  * PCM mode: Number of BCLKs within(Left + Right) channel width;(bitwidth * channel - 1)
	  * I2S/Left-Justified/Right-Justified mode: Number of BCLKs within each individual channel width(Left or Right: 32 or 24 or 16 so...)
	  */
	if(ac108->i2s_mode != PCM_FORMAT)
	{
		ret = regmap_update_bits(ac108->i2cmap, I2S_LRCK_CTRL1, 0X03 << LRCK_PERIODH, 0x00);
		ret |= regmap_write(ac108->i2cmap, I2S_LRCK_CTRL2, ((ac108_samp_res[samp_res].real_val - 1) & 0xFF));
		if(ret < 0)
		{
			dev_err(component->dev, "%s: not pcm_mode: setting lrck width  fail\n", __func__);
			return ret;
		}
	}
	else
	{
		unsigned div;

		div = ac108_samp_res[samp_res].real_val * channels -1;
		ret = regmap_update_bits(ac108->i2cmap, I2S_LRCK_CTRL1, 0X03 << LRCK_PERIODH, (div >> 8) << LRCK_PERIODH);
		ret |= regmap_write(ac108->i2cmap, I2S_LRCK_CTRL2, (div & 0xFF));
		if(ret < 0)
		{
			dev_err(component->dev, "%s: pcm_mode: setting lrck width  fail\n", __func__);
			return ret;
		}
	}

	/*
	 * 0x35:
	 * set slot width and sample resolution
	 */
	ret = regmap_update_bits(ac108->i2cmap, I2S_FMT_CTRL2, 0X07 << SAMPLE_RESOLUTION | 0x07 << SLOT_WIDTH_SEL, 
														  ac108_samp_res[samp_res].reg_val << SAMPLE_RESOLUTION | ac108_samp_res[samp_res].reg_val << SLOT_WIDTH_SEL);
	if(ret < 0)
	{
		dev_err(component->dev, "%s: setting slot width and sample resolution fail\n", __func__);
		return ret;
	}

	/*
	 * 0x60:
	 * set adc sample rate
	 */
	ret = regmap_update_bits(ac108->i2cmap, ADC_SPRC, 0X0F << ADC_FS_I2S1, ac108_sample_rate[rate].reg_val << ADC_FS_I2S1);
	ret |= regmap_write(ac108->i2cmap, HPF_EN, 0x0F);
	if(ret < 0)
	{
		dev_err(component->dev, "%s: setting adc sample rate fail\n", __func__);
		return ret;
	}

	ac108_config_pll(ac108, ac108_sample_rate[rate].real_val, ac108_samp_res[samp_res].real_val * channels * 4);

	/*
	 * master mode only
	 */
	//bclkdiv = ac108->sysclk / (ac108_sample_rate[rate].real_val * channels * ac108_samp_res[samp_res].real_val);
	//for(i = 0; i < ARRAY_SIZE(ac108_bclkdivs) - 1; i++)
	//	if(ac108_bclkdivs[i] >= bclkdiv)
	//		break;
	//ret = regmap_update_bits(ac108->i2cmap, I2S_BCLK_CTRL, 0x0F << BCLKDIV, i << BCLKDIV);
	if(ret < 0)
	{
		dev_err(component->dev, "%s: setting bclk div field\n", __func__);
		return ret;
	}
	
	/*
	 * slots allocation for each chip
	 */
	ac108_multi_chips_slots(ac108, channels);

	/* 0x21: Module clock enable <I2S, ADC digital, MIC offset Calibration, ADC analog> */
	ret = regmap_write(ac108->i2cmap, MOD_CLK_EN, 1 << I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);
	
	/* 0x22: Module reset de-asserted <I2S, ADC digital, MIC offset Calibration, ADC analog> */
	ret = regmap_write(ac108->i2cmap, MOD_RST_CTRL, 1 << I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);

	dev_dbg(component->dev, "%s - quit\n", __func__);

	return 0;
}

static int ac108_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;

	dev_dbg(component->dev, "%s - Enter\n", __func__);

	return 0;
}

static int ac108_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;

	dev_dbg(component->dev, "%s - Enter\n", __func__);
	
	return 0;
}

static int ac108_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *component = dai->component;

	dev_dbg(component->dev, "%s - Enter\n", __func__);

	return 0;
}

/**
* The power management related registers are Reg01h~Reg09h
* 0x01-0x05,0x08,use the default value
**/
static int ac108_configure_power(struct ac108_priv *ac108)
{
	int ret = 0;
	
	/*
	  * 0x06: Enable Analog LDO
	  */
	ret = regmap_update_bits(ac108->i2cmap, PWR_CTRL6, 0x01 << LDO33ANA_ENABLE, 0x01 << LDO33ANA_ENABLE);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Power: Setting Analog LDO faill\n");
		return ret;
	}

	/*
	  * 0x07: (needed for Analog LDO and micbias)
	  * Setting VREF voltage
	  * REF faststart disable, Enable VREF
	  */
	ret = regmap_update_bits(ac108->i2cmap, PWR_CTRL7, 0x01F << VREF_SEL | 0x01 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE,
													     0x013 << VREF_SEL | 0x00 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Power: Setting VREF faill\n");
		return ret;
	}

	/*
	  * 0x09:
	  * Disable fast-start circuit on VREFP
	  * VREFP_RESCTRL=00=1 MOhm
	  * IGEN_TRIM=100=+25%
	  * Enable VREFP (needed by all audio input channels)
	  */
	ret = regmap_update_bits(ac108->i2cmap, PWR_CTRL9, 0x01 << VREFP_FASTSTART_ENABLE | 0x03 << VREFP_RESCTRL | 0x07 << IGEN_TRIM | 0x01 << VREFP_ENABLE,
													     0x00 << VREFP_FASTSTART_ENABLE | 0x00 << VREFP_RESCTRL | 0x04 << IGEN_TRIM | 0x01 << VREFP_ENABLE);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Power: Setting VREFP faill\n");
		return ret;
	}

	return ret;
}

static int ac108_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_component *component = codec_dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	unsigned char tx_offset, brck_polarity, lrck_polarity;
	int ret = 0;

	dev_dbg(component->dev, "%s entern\n", __func__);

	/* AC108 config Master/Slave*/
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK)
	{
		case SND_SOC_DAIFMT_CBM_CFM:	/* AC108 Master */
			dev_dbg(component->dev, "AC108 set to work as Master\n");
			/**0x30:
			*chip is master mode, BCLK & LRCK output,enable SDO1_EN and SDO2_EN,
			*TXEN enable, RXEN disable, GEN enable
			**/
			ret = regmap_update_bits(ac108->i2cmap, I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x01 << TXEN | 0x01 << RXEN | 0x01 << GEN,
															0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x01 << TXEN | 0x01 << RXEN | 0x01 << GEN );
			if(ret != 0)
			{
				dev_err(component->dev, "AC108 setting master mode fail\n");
				return ret;
			}
			break;
		case SND_SOC_DAIFMT_CBS_CFS:	/* AC108 Slave */
			dev_dbg(component->dev, "AC108 set to work as Slave\n");
			/**0x30:
			*chip is slave mode, BCLK & LRCK input,enable SDO1_EN and SDO2_EN,
			*TXEN disable, RXEN enable, GEN enable
			**/
			ret = regmap_update_bits(ac108->i2cmap, I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x01 << TXEN | 0x01 << RXEN | 0x01 << GEN,
															0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x01 << TXEN | 0x01 << RXEN | 0x01 << GEN );
			if(ret != 0)
			{
				dev_err(component->dev, "AC108 setting slave mode fail\n");
				return ret;
			}
			break;
		default:
			dev_err(component->dev, "AC108 Master/Slave mode config error:%u\n", (fmt & SND_SOC_DAIFMT_MASTER_MASK) >> 12);
			return -EINVAL;
	}

	/* AC108 config I2S/LJ/RJ/PCM format */
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
			dev_dbg(component->dev, "AC108 config I2S format\n");
			ac108->i2s_mode = LEFT_JUSTIFIED_FORMAT;
			tx_offset = 1;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			dev_dbg(component->dev, "AC108 config RIGHT-JUSTIFIED format\n");
			ac108->i2s_mode = RIGHT_JUSTIFIED_FORMAT;
			tx_offset = 0;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			dev_dbg(component->dev, "AC108 config LEFT-JUSTIFIED format\n");
			ac108->i2s_mode = LEFT_JUSTIFIED_FORMAT;
			tx_offset = 0;
			break;
		case SND_SOC_DAIFMT_DSP_A:
			dev_dbg(component->dev, "AC108 config PCM-A format\n");
			ac108->i2s_mode = PCM_FORMAT;
			tx_offset = 1;
			break;
		case SND_SOC_DAIFMT_DSP_B:
			dev_dbg(component->dev, "AC108 config PCM-B format\n");
			ac108->i2s_mode = PCM_FORMAT;
			tx_offset = 0;
			break;
		default:
			dev_err(component->dev, "AC108 I2S format config error:%u\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
			return -EINVAL;
	}

	/* AC108 config BCLK & LRCK polarity*/
	switch(fmt & SND_SOC_DAIFMT_INV_MASK)
	{
		case SND_SOC_DAIFMT_NB_NF:
			dev_dbg(component->dev, "AC108 config BCLK & LRCK polarity: BCLK_normal, LRCK_normal\n");
			brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
			lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			dev_dbg(component->dev, "AC108 config BCLK & LRCK polarity: BCLK_normal, LRCK_invert\n");
			brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
			lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			dev_dbg(component->dev, "AC108 config BCLK & LRCK polarity: BCLK_invert, LRCK_normal\n");
			brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
			lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
			break;
		case SND_SOC_DAIFMT_IB_IF:
			dev_dbg(component->dev, "AC108 config BCLK & LRCK polarity: BCLK_invert, LRCK_invert\n");
			brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
			lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
			break;
		default:
			dev_dbg(component->dev, "AC108 config BCLK/LRCLK polarity error:%u\n", (fmt & SND_SOC_DAIFMT_INV_MASK) >> 8);
			return -EINVAL;
	}

	ret = ac108_configure_power(ac108);
	if(ret != 0)
	{
		dev_dbg(component->dev, "Power setting fail :%d\n", ret);
		return ret;
	}

	/*
	  * 0x31: 
	  * 0: normal mode, negative edge drive and positive edge sample
	  * 1: invert mode, positive edge drive and negative edge sample
	  */
	ret = regmap_update_bits(ac108->i2cmap, I2S_BCLK_CTRL, 0x01 << BCLK_POLARITY, brck_polarity<< BCLK_POLARITY);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Config BCLK polarity faill\n");
		return ret;
	}

	/*
	  * 0x32: same as 0x31
	  */
	ret = regmap_update_bits(ac108->i2cmap, I2S_LRCK_CTRL1, 0x01 << LRCK_POLARITY, lrck_polarity<< LRCK_POLARITY);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Config BCLK polarity faill\n");
		return ret;
	}

	/*
	  * 0x34: i2s format configure
	  * Encoding mode selection: disable
	  * Mode selection:
	  * TX2 offset tune, TX2data offset to LRCK
	  * TX1 offset tune, TX1data offset to LRCK
	  * normal mode for the last half cycle of BCLK in the slot
	  * turn to hi-z state(TDM) when not transferring slot
	  */
	ret = regmap_update_bits(ac108->i2cmap, I2S_FMT_CTRL1, 0x01 << ENCD_SEL | 0x03 << MODE_SEL | 0x01 << TX2_OFFSET | 0x01 << TX1_OFFSET | 0x01 << TX_SLOT_HIZ | 0x01 << TX_STATE,
														   0x00 << ENCD_SEL 				|
														   ac108->i2s_mode << MODE_SEL	|
														   tx_offset << TX2_OFFSET			|
														   tx_offset << TX1_OFFSET			|
														   0x00 << TX_SLOT_HIZ			|
														   0x00 << TX_STATE);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Config i2s format 1 faill\n");
		return ret;
	}

	/*
	  * 0x36: i2s format configure
	  * MSB/LSB first select: MSB first select
	  * Sign Extend in slot[sample resolution < slot width]: Transfer 0 after each sample in each slot
	  * OUT2_MUTE, OUT1_MUTE shoule be set in widget
	  * LRCK = 1 BCLK width
	  * Linear PCM
	  */
	ret = regmap_update_bits(ac108->i2cmap, I2S_FMT_CTRL3, 0x01 << TX_MLS | 0x03 << SEXT | 0x01 << LRCK_WIDTH | 0x03 << TX_PDM,
														   0x00 << TX_MLS | 0x03 << SEXT | 0x00 << LRCK_WIDTH | 0x00 << TX_PDM);
	if(ret != 0)
	{
		dev_err(&ac108->i2c->dev, "Config i2s format 2 faill\n");
		return ret;
	}

	ret = regmap_write(ac108->i2cmap, HPF_EN, 0x00);
	if(ret < 0)
		dev_err(&ac108->i2c->dev, "%s: Write val=0x%02x to reg=0x%02x fail\n", __func__, HPF_EN, 0x00);


	return ret;
}

static const struct snd_soc_dai_ops ac108_dai_ops = {
	/*DAI clocking configuration*/
	.set_sysclk		= ac108_set_sysclk,		//2
	.set_pll		= ac108_set_pll,
	.set_clkdiv		= ac108_set_clkdiv,

	/*ALSA PCM audio operations*/
	.hw_params 		= ac108_hw_params,		//3
	.prepare		= ac108_prepare,
	.trigger		= ac108_trigger,
	.digital_mute	= ac108_mute,

	/*DAI format configuration*/
	.set_fmt		= ac108_set_fmt,		//1
};

static struct snd_soc_dai_driver ac108_dai = {
	.name		= "ac108-codec",
	.capture	= {
		.stream_name 	= "Capture",
		.channels_min	= 1,
		.channels_max	= AC108_CHANNELS_MAX,
		.rates			= AC108_RATES,
		.formats		= AC108_FORMATS,
	},
	.ops		= &ac108_dai_ops,
};

static int ac108_codec_probe(struct snd_soc_component *component)
{
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);

	spin_lock_init(&ac108->lock);

	dev_dbg(component->dev, "%s - initialization done\n", __func__);

	return 0;
}

static void ac108_codec_remove(struct snd_soc_component *component)
{

	dev_dbg(component->dev, "%s - initialization done\n", __func__);
}
/*
static int ac108_codec_suspend(struct snd_soc_component *component)
{

	dev_dbg(component->dev, "%s - initialization done\n", __func__);
	
	return 0;
}

static int ac108_codec_resume(struct snd_soc_component *component)
{

	dev_dbg(component->dev, "%s - initialization done\n", __func__);
	
	return 0;
}

static int ac108_set_bias_level(struct snd_soc_component *component)
{

	dev_dbg(component->dev, "%s - initialization done\n", __func__);
	
	return 0;
}
*/
static const struct snd_soc_component_driver soc_component_dev_ac108 = {
	.probe			= ac108_codec_probe,
	.remove			= ac108_codec_remove,
//	.suspend			= ac108_codec_suspend,
//	.resume			= ac108_codec_resume,
//	.set_bias_level	= ac108_set_bias_level,

	.controls		= ac108_snd_controls,
	.num_controls		= ARRAY_SIZE(ac108_snd_controls),
	.dapm_widgets		= ac108_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ac108_dapm_widgets),
	.dapm_routes		= ac108_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ac108_dapm_routes),

};

static ssize_t ac108_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dev_dbg(dev, "echo flag|reg|val > ac108\n");
	dev_dbg(dev, "eg: read=0  reg=0x10 val=0x00 ==> echo 01000 > ac108 (val default 0x00)\n");
	dev_dbg(dev, "eg: write=1 reg=0x10 val=0x55 ==> echo 11055 > ac108\n");

	return 0;
}

static ssize_t ac108_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct ac108_priv *ac108;
	int val = 0, ret, val_r=0;
	u8 val_w, reg, flag = 0;

	ac108 = dev_get_drvdata(dev);

	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 16) & 0xF;
	reg = (val >> 8) & 0xFF;

	if(flag)
	{
		val_w = val & 0xFF;

		ret = regmap_write(ac108->i2cmap, reg, val_w);
		if(ret < 0)
			dev_err(&ac108->i2c->dev, "Write val=0x%02x to reg=0x%02x fail\n", reg, val_w);
		else
			dev_dbg(dev, "Write: 0x%02x to reg 0x%02x\n", val_w, reg);
	}
	else
	{
		ret = regmap_read(ac108->i2cmap, reg, &val_r);
		if(ret < 0)
			dev_err(&ac108->i2c->dev, "Read reg=0x%02x fail\n", reg);
		else
			dev_dbg(dev, "Read: reg 0x%02x val = 0x%02x\n", reg, val_r);
	}

	return count;
}

static DEVICE_ATTR(ac108, 0644, ac108_show, ac108_store);
static struct attribute *ac108_debug_attrs[] = {
	&dev_attr_ac108.attr,
	NULL,
};

static struct attribute_group ac108_debug_attr_group = {
	.name 	= "ac108_debug",
	.attrs 	= ac108_debug_attrs,
};

static const struct regmap_config ac108_regmap = {
	.reg_bits 		= 8,
	.val_bits 		= 8,
	.reg_stride 	= 1,
	.max_register 	= 0xDF,
	.cache_type 	= REGCACHE_FLAT,
};
	
static int ac108_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *i2c_id)
{
	
	int ret = 0;
	
	ac108 = devm_kzalloc(&i2c->dev, sizeof(*ac108), GFP_KERNEL);
	if(ac108 == NULL)
	{
		dev_err(&i2c->dev, "Unable to alloc ac108 private data\n");
		return -ENOMEM;
	}

	ac108->i2c = i2c;
	dev_set_drvdata(&i2c->dev, ac108);

	ac108->i2cmap = devm_regmap_init_i2c(i2c, &ac108_regmap);
	if(IS_ERR(ac108->i2cmap))
	{
		ret = PTR_ERR(ac108->i2cmap);
		dev_err(&i2c->dev, "Fail to initialize i2cmap I/O: %d\n", ret);
		return ret;
	}

	regcache_cache_only(ac108->i2cmap, false);
	ret = regmap_write(ac108->i2cmap, CHIP_RST, CHIP_RST_VAL);
	//if(ret < 0)
	//{
	//	dev_err(&i2c->dev, "Fail to regmap write %d!\n", ret);
	//	return ret;
	//}
	msleep(1);

	ac108->codec_cnt++;

	ret = sysfs_create_group(&i2c->dev.kobj, &ac108_debug_attr_group);
	if(ret)
	{
		dev_err(&i2c->dev, "Failed to create attr group: %d\n", ret);
		return ret;
	}

	ret = devm_snd_soc_register_component(&i2c->dev, &soc_component_dev_ac108, &ac108_dai, 1);
	if(ret != 0)
	{
		dev_err(&i2c->dev, "Failed to register CODEC: %d\n", ret);
		return ret;
	}

	dev_dbg(&i2c->dev, "%s - initialization done\n", __func__);

	return ret;
}

static int ac108_i2c_remove(struct i2c_client *i2c)
{
	sysfs_remove_group(&i2c->dev.kobj, &ac108_debug_attr_group);
	
	dev_dbg(&i2c->dev, "%s - remove done\n", __func__);
	
	return 0;
}

static const struct i2c_device_id ac108_i2c_id[] = {
	{ "ac108", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ac108_i2c_id);

static const struct of_device_id ac108_of_match[] = {
	{ .compatible = "x-power,ac108"},
	{ }
};
MODULE_DEVICE_TABLE(of, ac108_of_match);

static struct i2c_driver ac108_i2c_driver = {
	.driver = {
		.name = "ac108-codec",
		.of_match_table = ac108_of_match,
	},
	.probe		= ac108_i2c_probe,
	.remove		= ac108_i2c_remove,
	.id_table	= ac108_i2c_id,
};

module_i2c_driver(ac108_i2c_driver);

MODULE_DESCRIPTION("ASOC AC108 driver");
MODULE_AUTHOR("GinPot <GinPot@xx.com>");
MODULE_LICENSE("GPL");
