/**
 * @file encoder.h
 *
 */

#ifndef ENCODER_H
#define ENCODER_H

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

#if USE_ENCODER

#include <stdbool.h>
#include "lvgl/lv_hal/lv_hal_indev.h"

#ifndef MONITOR_SDL_INCLUDE_PATH
#define MONITOR_SDL_INCLUDE_PATH <SDL2/SDL.h>
#endif

#include MONITOR_SDL_INCLUDE_PATH

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
 * Initialize the encoder
 */
void encoder_init(void);

/**
 * Get the mouse wheel position change.
 * @param data store the read data here
 * @return false: all ticks and button state are handled
 */
bool encoder_read(lv_indev_data_t * data);

/**
 * It is called periodically from the SDL thread to check a key is pressed/released
 * @param event describes the event
 */
void encoder_handler(SDL_Event *event);

/**********************
 *      MACROS
 **********************/

#endif /*USE_ENCODER*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ENCODER_H*/
