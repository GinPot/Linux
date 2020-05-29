#ifndef		__AC108_H_
#define		__AC108_H_


#define AC108_CHANNELS_MAX	4
#define AC108_RATES			(SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT)
#define AC108_FORMATS		(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

#define SYSCLK_SRC_MCLK		0
#define SYSCLK_SRC_PLL		1

#define _FREQ_24_576K		24576000
#define _FREQ_22_579K		22579200

/*** AC108 Codec Register Define ***/

/* Chip Reset */
#define CHIP_RST				0x00
#define CHIP_RST_VAL			0x12

/* Power Control */
#define PWR_CTRL6				0x06
#define PWR_CTRL7				0x07
#define PWR_CTRL9				0x09

/* PLL Configure Control */
#define PLL_CTRL1				0x10
#define PLL_CTRL2				0x11
#define PLL_CTRL3				0x12
#define PLL_CTRL4				0x13
#define PLL_CTRL5				0x14
#define PLL_CTRL6				0x16
#define PLL_CTRL7				0x17
#define PLL_LOCK_CTRL			0x18

/* System Clock Control */
#define SYSCLK_CTRL				0x20
#define MOD_CLK_EN				0x21
#define MOD_RST_CTRL			0x22

/* I2S Common Control */
#define I2S_CTRL				0x30
#define I2S_BCLK_CTRL			0x31
#define I2S_LRCK_CTRL1			0x32
#define I2S_LRCK_CTRL2			0x33
#define I2S_FMT_CTRL1			0x34
#define I2S_FMT_CTRL2			0x35
#define I2S_FMT_CTRL3			0x36

/* I2S TX1 Control */
#define I2S_TX1_CTRL1			0x38
#define I2S_TX1_CTRL2			0x39
#define I2S_TX1_CTRL3			0x3A
#define I2S_TX1_CHMP_CTRL1		0x3C
#define I2S_TX1_CHMP_CTRL2		0x3D
#define I2S_TX1_CHMP_CTRL3		0x3E
#define I2S_TX1_CHMP_CTRL4		0x3F

/* ADC Common Control */
#define ADC_SPRC				0x60

/* HPF Control */
#define HPF_EN					0x66

/*** AC108 Codec Register Bit Define ***/

/* PWR_CTRL6 */
#define LDO33ANA_ENABLE			0

/* PWR_CTRL7 */
#define VREF_SEL				3
#define VREF_FASTSTART_ENABLE	1
#define VREF_ENABLE				0

/* PWR_CTRL9 */
#define VREFP_FASTSTART_ENABLE	7
#define VREFP_RESCTRL			5
#define IGEN_TRIM				1
#define VREFP_ENABLE			0

/* SYSCLK_CTRL */
#define PLLCLK_EN				7
#define PLLCLK_SRC				4
#define SYSCLK_SRC				3
#define SYSCLK_EN				0

/* MOD_CLK_EN & MOD_RST_CTRL*/
#define I2S						7
#define ADC_DIGITAL				4
#define MIC_OFFSET_CALIBRATION	1
#define ADC_ANALOG				0

/*I2S_CTRL*/	
#define BCLK_IOEN			7
#define LRCK_IOEN			6
#define SDO2_EN				5
#define SDO1_EN				4
#define TXEN				2
#define RXEN				1
#define GEN					0
	
/* I2S_BCLK_CTRL */	
#define BCLK_POLARITY		4
#define BCLKDIV				0
	
/* I2S_LRCK_CTRL1 */	
#define LRCK_POLARITY		4
#define LRCK_PERIODH		0
	
/* I2S_FMT_CTRL1 */	
#define ENCD_SEL			6
#define MODE_SEL			4
#define TX2_OFFSET			3
#define TX1_OFFSET			2
#define TX_SLOT_HIZ			1
#define TX_STATE			0
	
/* I2S_FMT_CTRL2 */	
#define SLOT_WIDTH_SEL		4
#define SAMPLE_RESOLUTION	0
	
/* I2S_FMT_CTRL3 */	
#define TX_MLS				7
#define SEXT				5
#define LRCK_WIDTH			2
#define TX_PDM				0

/* ADC_SPRC */
#define ADC_FS_I2S1			0

/* PLL_CTRL2 */
#define PLL_PREDIV2			5
#define PLL_PREDIV1			0

/* PLL_CTRL3 */
#define PLL_LOOPDIV_MSB		0

/* PLL_CTRL4 */
#define PLL_LOOPDIV_LSB		0

/* PLL_CTRL5 */
#define PLL_POSTDIV2		5
#define PLL_POSTDIV1		0

/* PLL_LOCK_CTRL */
#define PLL_LOCK_EN			0

/*** Some Config Value ***/

/* I2S Format Selection */
#define PCM_FORMAT				0
#define LEFT_JUSTIFIED_FORMAT	1
#define RIGHT_JUSTIFIED_FORMAT	2

/* I2S BCLK POLARITY Control*/
#define BCLK_NORMAL_DRIVE_N_SAMPLE_P	0
#define BCLK_INVERT_DRIVE_P_SAMPLE_N	1

/* I2S LRCK POLARITY Control*/
#define LRCK_LEFT_LOW_RIGHT_HIGH		0
#define LRCK_LEFT_HIGH_RIGHT_LOW		1

/*PLL_CTRL1*/
#define PLL_IBIAS				4
#define PLL_NDET				3
#define PLL_LOCKED_STATUS		2
#define PLL_COM_EN				1
#define PLL_EN					0


//ADC Digital Channel Volume Control
#define ADC1_DVOL_CTRL		0x70
#define ADC2_DVOL_CTRL		0x71
#define ADC3_DVOL_CTRL		0x72
#define ADC4_DVOL_CTRL		0x73

//Analog PGA Control
#define ANA_PGA1_CTRL		0x90
#define ANA_PGA2_CTRL		0x91
#define ANA_PGA3_CTRL		0x92
#define ANA_PGA4_CTRL		0x93

/*ANA_PGA1_CTRL*/
#define ADC1_ANALOG_PGA			1
#define ADC1_ANALOG_PGA_STEP	0


/*ANA_PGA2_CTRL*/
#define ADC2_ANALOG_PGA			1
#define ADC2_ANALOG_PGA_STEP	0

/*ANA_PGA3_CTRL*/
#define ADC3_ANALOG_PGA			1
#define ADC3_ANALOG_PGA_STEP	0

/*ANA_PGA4_CTRL*/
#define ADC4_ANALOG_PGA			1
#define ADC4_ANALOG_PGA_STEP	0

//ADC2 Analog Control
#define ANA_ADC2_CTRL1		0xA7
#define ANA_ADC2_CTRL2		0xA8
#define ANA_ADC2_CTRL3		0xA9
#define ANA_ADC2_CTRL4		0xAA
#define ANA_ADC2_CTRL5		0xAB
#define ANA_ADC2_CTRL6		0xAC
#define ANA_ADC2_CTRL7		0xAD

//ADC1 Analog Control
#define ANA_ADC1_CTRL1		0xA0
#define ANA_ADC1_CTRL2		0xA1
#define ANA_ADC1_CTRL3		0xA2
#define ANA_ADC1_CTRL4		0xA3
#define ANA_ADC1_CTRL5		0xA4
#define ANA_ADC1_CTRL6		0xA5
#define ANA_ADC1_CTRL7		0xA6

//ADC3 Analog Control
#define ANA_ADC3_CTRL1		0xAE
#define ANA_ADC3_CTRL2		0xAF
#define ANA_ADC3_CTRL3		0xB0
#define ANA_ADC3_CTRL4		0xB1
#define ANA_ADC3_CTRL5		0xB2
#define ANA_ADC3_CTRL6		0xB3
#define ANA_ADC3_CTRL7		0xB4


/*ANA_ADC1_CTRL1*/
#define ADC1_PGA_BYPASS			7
#define ADC1_PGA_BYP_RCM		6
#define ADC1_PGA_CTRL_RCM		4
#define ADC1_PGA_MUTE			3
#define ADC1_DSM_ENABLE			2
#define ADC1_PGA_ENABLE			1
#define ADC1_MICBIAS_EN			0

/*ANA_ADC1_CTRL3*/
#define ADC1_ANA_CAL_EN			5
#define ADC1_SEL_OUT_EDGE		3
#define ADC1_DSM_DISABLE		2
#define ADC1_VREFP_DISABLE		1
#define ADC1_AAF_DISABLE		0

/*ANA_ADC1_CTRL6*/
#define PGA_CTRL_TC				6
#define PGA_CTRL_RC				4
#define PGA_CTRL_I_LIN			2
#define PGA_CTRL_I_IN			0

/*ANA_ADC1_CTRL7*/
#define PGA_CTRL_HI_Z			7
#define PGA_CTRL_SHORT_RF		6
#define PGA_CTRL_VCM_VG			4
#define PGA_CTRL_VCM_IN			0


/*ANA_ADC2_CTRL1*/
#define ADC2_PGA_BYPASS			7
#define ADC2_PGA_BYP_RCM		6
#define ADC2_PGA_CTRL_RCM		4
#define ADC2_PGA_MUTE			3
#define ADC2_DSM_ENABLE			2
#define ADC2_PGA_ENABLE			1
#define ADC2_MICBIAS_EN			0

/*ANA_ADC2_CTRL3*/
#define ADC2_ANA_CAL_EN			5
#define ADC2_SEL_OUT_EDGE		3
#define ADC2_DSM_DISABLE		2
#define ADC2_VREFP_DISABLE		1
#define ADC2_AAF_DISABLE		0

/*ANA_ADC2_CTRL6*/
#define PGA_CTRL_IBOOST			7
#define PGA_CTRL_IQCTRL			6
#define PGA_CTRL_OABIAS			4
#define PGA_CTRL_CMLP_DIS		3
#define PGA_CTRL_PDB_RIN		2
#define PGA_CTRL_PEAKDET		0

/*ANA_ADC2_CTRL7*/
#define AAF_LPMODE_EN			7
#define AAF_STG2_IB_SEL			4
#define AAFDSM_IB_DIV2			3
#define AAF_STG1_IB_SEL			0


/*ANA_ADC3_CTRL1*/
#define ADC3_PGA_BYPASS			7
#define ADC3_PGA_BYP_RCM		6
#define ADC3_PGA_CTRL_RCM		4
#define ADC3_PGA_MUTE			3
#define ADC3_DSM_ENABLE			2
#define ADC3_PGA_ENABLE			1
#define ADC3_MICBIAS_EN			0

/*ANA_ADC3_CTRL3*/
#define ADC3_ANA_CAL_EN			5
#define ADC3_INVERT_CLK			4
#define ADC3_SEL_OUT_EDGE		3
#define ADC3_DSM_DISABLE		2
#define ADC3_VREFP_DISABLE		1
#define ADC3_AAF_DISABLE		0

/*ANA_ADC3_CTRL7*/
#define DSM_COMP_IB_SEL			6
#define DSM_OTA_CTRL			4
#define DSM_LPMODE				3
#define DSM_OTA_IB_SEL			0


/*ANA_ADC4_CTRL1*/
#define ADC4_PGA_BYPASS			7
#define ADC4_PGA_BYP_RCM		6
#define ADC4_PGA_CTRL_RCM		4
#define ADC4_PGA_MUTE			3
#define ADC4_DSM_ENABLE			2
#define ADC4_PGA_ENABLE			1
#define ADC4_MICBIAS_EN			0

/*ANA_ADC4_CTRL3*/
#define ADC4_ANA_CAL_EN			5
#define ADC4_SEL_OUT_EDGE		3
#define ADC4_DSM_DISABLE		2
#define ADC4_VREFP_DISABLE		1
#define ADC4_AAF_DISABLE		0

/*ANA_ADC4_CTRL6*/
#define DSM_DEMOFF				5
#define DSM_EN_DITHER			4
#define DSM_VREFP_LPMODE		2
#define DSM_VREFP_OUTCTRL		0

/*ANA_ADC4_CTRL7*/
#define CK8M_EN					5
#define OSC_EN					4
#define ADC4_CLK_GATING			3
#define ADC3_CLK_GATING			2
#define ADC2_CLK_GATING			1
#define ADC1_CLK_GATING			0

//ADC Common Control
#define ADC_SPRC			0x60
#define ADC_DIG_EN			0x61
#define DMIC_EN				0x62
#define ADC_DSR				0x63
#define ADC_FIR				0x64
#define ADC_DDT_CTRL		0x65

//ADC4 Analog Control
#define ANA_ADC4_CTRL1		0xB5
#define ANA_ADC4_CTRL2		0xB6
#define ANA_ADC4_CTRL3		0xB7
#define ANA_ADC4_CTRL4		0xB8
#define ANA_ADC4_CTRL5		0xB9
#define ANA_ADC4_CTRL6		0xBA
#define ANA_ADC4_CTRL7		0xBB

/*ANA_ADC4_CTRL7*/
#define CK8M_EN					5
#define OSC_EN					4
#define ADC4_CLK_GATING			3
#define ADC3_CLK_GATING			2
#define ADC2_CLK_GATING			1
#define ADC1_CLK_GATING			0


struct ac108_priv {
	struct i2c_client *i2c;
	struct regmap *i2cmap;

	int codec_cnt;
	
	spinlock_t lock;
	
	unsigned int mclk;
	unsigned int sysclk;
	unsigned int clk_id;
	unsigned char i2s_mode;
};

struct real_val_to_reg_val {
	unsigned int real_val;
	unsigned int reg_val;
};

struct pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m1;
	unsigned int m2;
	unsigned int n;
	unsigned int k1;
	unsigned int k2;
};


#endif /* __AC108_H_ */
