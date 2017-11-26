/**
 * @file FT5406EE8.h
 * 
 */

#ifndef FT5406EE8_H
#define FT5406EE8_H

/*********************
 *      INCLUDES
 *********************/
#include "lv_drv_conf.h"
#if USE_FT5406EE8

#include <stdbool.h>
#include <stdint.h>

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
bool ft5406ee8_get(int16_t * x, int16_t * y);

/**********************
 *      MACROS
 **********************/

#endif

#endif
