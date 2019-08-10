/**
 * @file bsd_fbdev.h
 *
 */

#ifndef BSD_FBDEV_H
#define BSD_FBDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_BSD_FBDEV

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int bsd_fbdev_init(void);
int bsd_fbdev_exit(void);
void bsd_fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);


/**********************
 *      MACROS
 **********************/

#endif  /*USE_BSD_FBDEV*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*BSD_FBDEV_H*/
