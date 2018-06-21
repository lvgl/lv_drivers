/**
 * @file FT5406EE8.h
 * 
 */

#ifndef FT5406EE8_H
#define FT5406EE8_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_drv_conf.h"
#if USE_FT5406EE8

#include <stdbool.h>
#include <stdint.h>
#include "lvgl/lv_hal/lv_hal_indev.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ft5406ee8_init(void);
bool ft5406ee8_read(lv_indev_data_t * data);

/**********************
 *      MACROS
 **********************/

#endif /* USE_FT5406EE8 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FT5406EE8_H */
