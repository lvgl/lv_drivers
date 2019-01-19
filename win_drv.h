/**
 * @file fbdev.h
 *
 */

#ifndef WINDRV_H
#define WINDRV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif

#if USE_WINDOWS
#include <windows.h>
#include <stdbool.h>

#include "lvgl/lv_misc/lv_color.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern bool lv_win_exit_flag;

HWND windrv_init(void);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_WINDOWS*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*FBDEV_H*/
