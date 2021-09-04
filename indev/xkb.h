/**
 * @file xkb.h
 *
 */

#ifndef XKB_H
#define XKB_H

#ifdef __cplusplus
extern "C" {
#endif

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

#if USE_XKB

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct xkb_rule_names;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialise the XKB system
 * @return true if the initialisation was successful
 */
bool xkb_init(void);
/**
 * Set a new keymap to be used for processing future key events
 * @param names XKB rule names structure (use NULL components for default values)
 * @return true if creating the keymap and associated state succeeded
 */
bool xkb_set_keymap(struct xkb_rule_names names);
/**
 * Process an evdev scancode
 * @param scancode evdev scancode to process
 * @param down true if the key was pressed, false if it was releases
 * @param state associated XKB state
 * @return the (first) UTF-8 character produced by the event or 0 if no output was produced
 */
uint32_t xkb_process_key(uint32_t scancode, bool down);

/**********************
 *      MACROS
 **********************/

#endif /* USE_XKB */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XKB_H */
