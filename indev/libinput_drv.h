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
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_LIBINPUT || USE_BSD_LIBINPUT

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include <poll.h>
#include <pthread.h>

#if USE_XKB
#include "xkb.h"
#endif /* USE_XKB */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
  LIBINPUT_CAPABILITY_NONE     = 0,
  LIBINPUT_CAPABILITY_KEYBOARD = 1U << 0,
  LIBINPUT_CAPABILITY_POINTER  = 1U << 1,
  LIBINPUT_CAPABILITY_TOUCH    = 1U << 2
} libinput_capability;

typedef struct {
  lv_indev_state_t pressed;
  int key_val;
  lv_point_t point;
} libinput_lv_event_t;

#define MAX_EVENTS 32
typedef struct {
  int fd;
  struct pollfd fds[1];

  /* The points array is implemented as a circular LIFO queue */
  libinput_lv_event_t points[MAX_EVENTS]; /* Event buffer */
  libinput_lv_event_t slots[2]; /* Realtime state of up to 2 fingers to handle multitouch */

  /* Pointer devices work a bit differently in libinput which requires us to store their last known state */
  lv_point_t pointer_position;
  bool pointer_button_down;

  int start; /* Index of start of event queue */
  int end; /* Index of end of queue*/
  libinput_lv_event_t last_event; /* Report when no new events
                                   * to keep indev state consistent
                                   */
  bool deinit; /* Tell worker thread to quit */
  pthread_mutex_t event_lock;
  pthread_t worker_thread;

  struct libinput *libinput_context;
  struct libinput_device *libinput_device;

#if USE_XKB
  xkb_drv_state_t xkb_state;
#endif /* USE_XKB */
} libinput_drv_state_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Determine the capabilities of a specific libinput device.
 * @param device the libinput device to query
 * @return the supported input capabilities
 */
libinput_capability libinput_query_capability(struct libinput_device *device);
/**
 * Find connected input device with specific capabilities
 * @param capabilities required device capabilities
 * @param force_rescan erase the device cache (if any) and rescan the file system for available devices
 * @return device node path (e.g. /dev/input/event0) for the first matching device or NULL if no device was found.
 *         The pointer is safe to use until the next forceful device search.
 */
char *libinput_find_dev(libinput_capability capabilities, bool force_rescan);
/**
 * Find connected input devices with specific capabilities
 * @param capabilities required device capabilities
 * @param devices pre-allocated array to store the found device node paths (e.g. /dev/input/event0). The pointers are
 *                safe to use until the next forceful device search.
 * @param count maximum number of devices to find (the devices array should be at least this long)
 * @param force_rescan erase the device cache (if any) and rescan the file system for available devices
 * @return number of devices that were found
 */
size_t libinput_find_devs(libinput_capability capabilities, char **found, size_t count, bool force_rescan);
/**
 * Prepare for reading input via libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 */
void libinput_init(void);
/**
 * Prepare for reading input via libinput using a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state driver state to initialize
 * @param path input device node path (e.g. /dev/input/event0)
 */
void libinput_init_state(libinput_drv_state_t *state, char* path);
/**
 * De-initialise a previously initialised driver state and free any dynamically allocated memory. Use this function if you want to
 * reuse an existing driver state.
 * @param state driver state to de-initialize
 */
void libinput_deinit_state(libinput_drv_state_t *state);
/**
 * Reconfigure the device file for libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 * @param dev_name input device node path (e.g. /dev/input/event0)
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file(char* dev_name);
/**
 * Reconfigure the device file for libinput using a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state the driver state to configure
 * @param dev_name input device node path (e.g. /dev/input/event0)
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file_state(libinput_drv_state_t *state, char* dev_name);
/**
 * Read available input events via libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 * @param indev_drv driver object itself
 * @param data store the libinput data here
 */
void libinput_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
/**
 * Read available input events via libinput using a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state the driver state to use
 * @param indev_drv driver object itself
 * @param data store the libinput data here
 */
void libinput_read_state(libinput_drv_state_t * state, lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

/**********************
 *      MACROS
 **********************/

#endif /* USE_LIBINPUT || USE_BSD_LIBINPUT */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_LIBINPUT_H */
