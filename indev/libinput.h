/**
 * @file libinput.h
 *
 */

#ifndef LVGL_LIBINPUT_H
#define LVGL_LIBINPUT_H

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

#if USE_LIBINPUT

#include <stdint.h>
#include <stdbool.h>
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

/**
 * Initialize the libinput
 */
void libinput_init(void);
/**
 * reconfigure the device file for libinput
 * @param dev_name set the libinput device filename
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file(char* dev_name);
/**
 * Get the current position and state of the libinput
 * @param data store the libinput data here
 * @return false: because the points are not buffered, so no more data to be read
 */
bool libinput_read(lv_indev_data_t * data);


/**********************
 *      MACROS
 **********************/

#endif /* USE_LIBINPUT */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_LIBINPUT_H */
