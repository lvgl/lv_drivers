/**
 * @file ILI9340.h
 * 
 */

#ifndef ILI9340_H
#define ILI9340_H

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_drv_conf.h"
#if USE_ILI9340 != 0

#include <stdint.h>
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
void ili9340_init(void);
void ili9340_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);
void ili9340_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t  color);
void ili9340_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);

/**********************
 *      MACROS
 **********************/

#endif

#endif
