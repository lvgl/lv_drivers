/**
 * @file XPT2046.h
 * 
 */

#ifndef XPT2046_H
#define XPT2046_H

/*********************
 *      INCLUDES
 *********************/
#include "lv_drv_conf.h"
#if USE_XPT2046 != 0

#include <stdint.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void xpt2046_init(void);
bool xpt2046_get(int16_t * x, int16_t * y);

/**********************
 *      MACROS
 **********************/

#endif

#endif
