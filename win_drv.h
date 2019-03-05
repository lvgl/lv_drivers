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
#include "../lv_drv_conf.h"
#endif

#if USE_WINDOWS
#include <windows.h>
#include <stdbool.h>

#include "lvgl/lvgl.h"

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
extern lv_disp_t *lv_windows_disp;

HWND windrv_init(void);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_WINDOWS*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WIN_DRV_H*/
