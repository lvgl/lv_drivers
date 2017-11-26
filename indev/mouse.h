/**
 * @file mouse.h
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

/*********************
 *      INCLUDES
 *********************/

#include "lv_drv_conf.h"

#if USE_MOUSE
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
void mouse_init(void);
bool mouse_get(int16_t * x, int16_t * y);

/**********************
 *      MACROS
 **********************/

#endif

#endif
