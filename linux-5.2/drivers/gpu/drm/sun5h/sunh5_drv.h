#ifndef	__SUNH5_DRV_H__
#define __SUNH5_DRV_H__


struct sunh5_drv {
	struct list_head engine_list;
	struct list_head frontend_list;
	struct list_head tcon_list;
};


#endif 	/* __SUNH5_DRV_H__ */
