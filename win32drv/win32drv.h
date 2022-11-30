/**
 * @file win32drv.h
 *
 */

#ifndef LV_WIN32DRV_H
#define LV_WIN32DRV_H

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

#if USE_WIN32DRV

#include <windows.h>

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
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
/**
 * @brief True when the display windows were closed by user
*/
EXTERN_C bool lv_win32_quit_signal;

/**
 * @brief Create a display on WIN32
 * @param[in] hor_res Horizontal Resolution
 * @param[in] ver_res Vertical Resolution
 * @param[in] screen_title Title of the windows screen or NULL to use default
 * @param[in] icon_handle Icon for the windows screen
 * @return pointer to the new display or NULL if failed
*/
EXTERN_C lv_disp_t* lv_win32_create_disp(
    lv_coord_t hor_res,
    lv_coord_t ver_res,
    const wchar_t* screen_title,
    HICON icon_handle );

/**********************
 *      MACROS
 **********************/
#ifndef WIN32DRV_MAX_DISPLAYS
#define WIN32DRV_MAX_DISPLAYS 8 //!< Number of max displays allowed
#endif

#endif /*USE_WIN32DRV*/

#endif /*LV_WIN32DRV_H*/
