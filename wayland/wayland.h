/**
 * @file wayland
 *
 */

#ifndef WAYLAND_H
#define WAYLAND_H

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

#if USE_WAYLAND

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if LV_USE_USER_DATA == 0
#error "Support for user data is required by wayland driver. Set LV_USE_USER_DATA to 1 in lv_conf.h"
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
void lv_wayland_init(void);
void lv_wayland_deinit(void);
void lv_wayland_cycle(void);
void lv_wayland_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
void lv_wayland_pointer_read(lv_indev_drv_t * drv, lv_indev_data_t * data);
void lv_wayland_pointeraxis_read(lv_indev_drv_t * drv, lv_indev_data_t * data);
void lv_wayland_keyboard_read(lv_indev_drv_t * drv, lv_indev_data_t * data);
void lv_wayland_touch_read(lv_indev_drv_t * drv, lv_indev_data_t * data);

/**********************
 *      MACROS
 **********************/

#endif /* USE_WAYLAND */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WAYLAND_H */
