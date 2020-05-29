#ifndef	__SUNH5_CSC_H__
#define __SUNH5_CSC_H__


struct sunh5_mixer;


/* VI channel CSC units offsets */
#define CCSC00_OFFSET				0xAA050
#define CCSC01_OFFSET				0xFA000
#define CCSC10_OFFSET				0xA0000
#define CCSC11_OFFSET				0xF0000

#define SUNH5_CSC_CTRL(base)		(base + 0x0)
#define SUNH5_CSC_COEFF(base, i)	(base + 0x10 + 4 * i)

#define SUNH5_CSC_CTRL_EN			BIT(0)



enum sunh5_csc_mode {
	SUNH5_CSC_MODE_OFF,
	SUNH5_CSC_MODE_YUV2RGB,
	SUNH5_CSC_MODE_YVU2RGB,
};



void sunh5_csc_set_ccsc_coefficients(struct sunh5_mixer *mixer, int layer, enum sunh5_csc_mode mode);
void sunh5_csc_enable_ccsc(struct sunh5_mixer *mixer, int layer, bool enable);



#endif	/* __SUNH5_CSC_H__ */
