/**
 * @file libinput.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "libinput_drv.h"
#if USE_LIBINPUT || USE_BSD_LIBINPUT

#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <libinput.h>
#include <pthread.h>

#if USE_BSD_LIBINPUT
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct input_device {
  libinput_capability capabilities;
  char *path;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool rescan_devices(void);
static bool add_scanned_device(char *path, libinput_capability capabilities);
static void reset_scanned_devices(void);
static void *libinput_poll_worker(void* data);

static void read_pointer(libinput_drv_state_t *state, struct libinput_event *event);
static void read_keypad(libinput_drv_state_t *state, struct libinput_event *event);

static int open_restricted(const char *path, int flags, void *user_data);
static void close_restricted(int fd, void *user_data);

/**********************
 *  STATIC VARIABLES
 **********************/
static struct input_device *devices = NULL;
static size_t num_devices = 0;

static libinput_drv_state_t default_state = { .event_lock = PTHREAD_MUTEX_INITIALIZER, };

static const int timeout = 100; // ms
static const nfds_t nfds = 1;

static const struct libinput_interface interface = {
  .open_restricted = open_restricted,
  .close_restricted = close_restricted,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Determine the capabilities of a specific libinput device.
 * @param device the libinput device to query
 * @return the supported input capabilities
 */
libinput_capability libinput_query_capability(struct libinput_device *device) {
  libinput_capability capability = LIBINPUT_CAPABILITY_NONE;
  if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD)
      && (libinput_device_keyboard_has_key(device, KEY_ENTER) || libinput_device_keyboard_has_key(device, KEY_KPENTER)))
  {
    capability |= LIBINPUT_CAPABILITY_KEYBOARD;
  }
  if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER)) {
    capability |= LIBINPUT_CAPABILITY_POINTER;
  }
  if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)) {
    capability |= LIBINPUT_CAPABILITY_TOUCH;
  }
  return capability;
}

/**
 * Find connected input device with specific capabilities
 * @param capabilities required device capabilities
 * @param force_rescan erase the device cache (if any) and rescan the file system for available devices
 * @return device node path (e.g. /dev/input/event0) for the first matching device or NULL if no device was found.
 *         The pointer is safe to use until the next forceful device search.
 */
char *libinput_find_dev(libinput_capability capabilities, bool force_rescan) {
  char *path = NULL;
  libinput_find_devs(capabilities, &path, 1, force_rescan);
  return path;
}

/**
 * Find connected input devices with specific capabilities
 * @param capabilities required device capabilities
 * @param devices pre-allocated array to store the found device node paths (e.g. /dev/input/event0). The pointers are
 *                safe to use until the next forceful device search.
 * @param count maximum number of devices to find (the devices array should be at least this long)
 * @param force_rescan erase the device cache (if any) and rescan the file system for available devices
 * @return number of devices that were found
 */
size_t libinput_find_devs(libinput_capability capabilities, char **found, size_t count, bool force_rescan) {
  if ((!devices || force_rescan) && !rescan_devices()) {
    return 0;
  }

  size_t num_found = 0;

  for (size_t i = 0; i < num_devices && num_found < count; ++i) {
    if (devices[i].capabilities & capabilities) {
      found[num_found] = devices[i].path;
      num_found++;
    }
  }

  return num_found;
}

/**
 * Reconfigure the device file for libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 * @param dev_name input device node path (e.g. /dev/input/event0)
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file(char* dev_name)
{
  return libinput_set_file_state(&default_state, dev_name);
}

/**
 * Reconfigure the device file for libinput using a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state the driver state to configure
 * @param dev_name input device node path (e.g. /dev/input/event0)
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file_state(libinput_drv_state_t *state, char* dev_name)
{
  // This check *should* not be necessary, yet applications crashes even on NULL handles.
  // citing libinput.h:libinput_path_remove_device:
  // > If no matching device exists, this function does nothing.
  if (state->libinput_device) {
    libinput_path_remove_device(state->libinput_device);
    state->libinput_device = libinput_device_unref(state->libinput_device);
  }

  state->libinput_device = libinput_path_add_device(state->libinput_context, dev_name);
  if(!state->libinput_device) {
    perror("unable to add device to libinput context:");
    return false;
  }
  state->libinput_device = libinput_device_ref(state->libinput_device);
  if(!state->libinput_device) {
    perror("unable to reference device within libinput context:");
    return false;
  }

  return true;
}

/**
 * Prepare for reading input via libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 */
void libinput_init(void)
{
  memset(&default_state, 0, sizeof(libinput_drv_state_t));
  libinput_init_state(&default_state, LIBINPUT_NAME);
}

/**
 * Prepare for reading input via libinput using the a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state driver state to initialize
 * @param path input device node path (e.g. /dev/input/event0)
 */
void libinput_init_state(libinput_drv_state_t *state, char* path)
{
  state->libinput_device = NULL;
  state->libinput_context = libinput_path_create_context(&interface, NULL);

  if(path == NULL || !libinput_set_file_state(state, path)) {
      fprintf(stderr, "unable to add device \"%s\" to libinput context: %s\n", path ? path : "NULL", strerror(errno));
      return;
  }
  state->fd = libinput_get_fd(state->libinput_context);

  /* prepare poll */
  state->fds[0].fd = state->fd;
  state->fds[0].events = POLLIN;
  state->fds[0].revents = 0;

  pthread_create(&state->worker_thread, NULL, libinput_poll_worker, state);

#if USE_XKB
  xkb_init_state(&(state->xkb_state));
#endif
}

/**
 * De-initialise a previously initialised driver state and free any dynamically allocated memory. Use this function if you want to
 * reuse an existing driver state.
 * @param state driver state to de-initialize
 */
void libinput_deinit_state(libinput_drv_state_t *state)
{
  if (state->fd)
    state->deinit = true;

  /* Give worker thread a whole second to quit */
  for (int i = 0; i < 100; i++) {
    if (!state->deinit)
      break;
    usleep(10000);
  }

  if (state->deinit) {
    fprintf(stderr, "libinput worker thread did not quit in time!\n");
    pthread_cancel(state->worker_thread);
  }

  if (state->libinput_device) {
    libinput_path_remove_device(state->libinput_device);
    libinput_device_unref(state->libinput_device);
  }

  if (state->libinput_context) {
    libinput_unref(state->libinput_context);
  }

#if USE_XKB
  xkb_deinit_state(&(state->xkb_state));
#endif

  lv_memzero(state, sizeof(libinput_drv_state_t));
}

/**
 * Read available input events via libinput using the default driver state. Use this function if you only want
 * to connect a single device.
 * @param indev_drv driver object itself
 * @param data store the libinput data here
 */
void libinput_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  libinput_read_state(&default_state, indev_drv, data);
}

libinput_lv_event_t *get_event(libinput_drv_state_t *state)
{
  if (state->start == state->end) {
    return NULL;
  }

  libinput_lv_event_t *evt = &state->points[state->start];

  if (++state->start == MAX_EVENTS)
    state->start = 0;

  return evt;
}

bool event_pending(libinput_drv_state_t *state)
{
  return state->start != state->end;
}

libinput_lv_event_t *new_event(libinput_drv_state_t *state)
{
  libinput_lv_event_t *evt = &state->points[state->end];

  if (++state->end == MAX_EVENTS)
    state->end = 0;

  /* We have overflowed the buffer, start overwriting
   * old events.
   */
  if (state->end == state->start) {
    LV_LOG_INFO("libinput: overflowed event buffer!");
    if (++state->start == MAX_EVENTS)
      state->start = 0;
  }

  memset(evt, 0, sizeof(libinput_lv_event_t));

  return evt;
}

static void *libinput_poll_worker(void* data)
{
  libinput_drv_state_t * state = (libinput_drv_state_t *)data;
  struct libinput_event *event;
  int rc = 0;

  LV_LOG_INFO("libinput: poll worker started");

  while (true) {
    rc = poll(state->fds, nfds, timeout);
    switch (rc){
      case -1:
        perror(NULL);
        __attribute__((fallthrough));
      case 0:
        if (state->deinit) {
          state->deinit = false; /* Signal that we're done */
          return NULL;
        }
        continue;
      default:
        break;
    }
    libinput_dispatch(state->libinput_context);
    pthread_mutex_lock(&state->event_lock);
    while((event = libinput_get_event(state->libinput_context)) != NULL) {
      read_pointer(state, event);
      read_keypad(state, event);
      libinput_event_destroy(event);
    }
    pthread_mutex_unlock(&state->event_lock);
    LV_LOG_INFO("libinput: event read");
  }

  return NULL;
}

/**
 * Read available input events via libinput using a specific driver state. Use this function if you want to
 * connect multiple devices.
 * @param state the driver state to use
 * @param indev_drv driver object itself
 * @param data store the libinput data here
 */
void libinput_read_state(libinput_drv_state_t * state, lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  LV_UNUSED(indev_drv);

  pthread_mutex_lock(&state->event_lock);

  libinput_lv_event_t *evt = get_event(state);
  data->continue_reading = event_pending(state);
  if (!evt)
    evt = &state->last_event;
  /* indev expects us to report the most recent state */
  else if (!data->continue_reading)
    state->last_event = *evt;

  data->point = evt->point;
  data->state = evt->pressed;
  data->key = evt->key_val;

  pthread_mutex_unlock(&state->event_lock);

  if (evt)
    LV_LOG_TRACE("libinput_read: %d//%d: (%04d,%04d): %d continue_reading? %d", state->start, state->end, data->point.x, data->point.y, data->state, data->continue_reading);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * rescan all attached evdev devices and store capable ones into the static devices array for quick later filtering
 * @return true if the operation succeeded
 */
static bool rescan_devices(void) {
  reset_scanned_devices();

  DIR *dir;
  struct dirent *ent;
  if (!(dir = opendir("/dev/input"))) {
    perror("unable to open directory /dev/input");
    return false;
  }

  struct libinput *context = libinput_path_create_context(&interface, NULL);

  while ((ent = readdir(dir))) {
    if (strncmp(ent->d_name, "event", 5) != 0) {
      continue;
    }

    /* 11 characters for /dev/input/ + length of name + 1 NUL terminator */
    char *path = malloc((11 + strlen(ent->d_name) + 1) * sizeof(char));
    if (!path) {
      perror("could not allocate memory for device node path");
      libinput_unref(context);
      reset_scanned_devices();
      return false;
    }
    strcpy(path, "/dev/input/");
    strcat(path, ent->d_name);

    struct libinput_device *device = libinput_path_add_device(context, path);
    if(!device) {
      perror("unable to add device to libinput context");
      free(path);
      continue;
    }

    /* The device pointer is guaranteed to be valid until the next libinput_dispatch. Since we're not dispatching events
     * as part of this function, we don't have to increase its reference count to keep it alive.
     * https://wayland.freedesktop.org/libinput/doc/latest/api/group__base.html#gaa797496f0150b482a4e01376bd33a47b */

    libinput_capability capabilities = libinput_query_capability(device);

    libinput_path_remove_device(device);

    if (capabilities == LIBINPUT_CAPABILITY_NONE) {
      free(path);
      continue;
    }

    if (!add_scanned_device(path, capabilities)) {
      free(path);
      libinput_unref(context);
      reset_scanned_devices();
      return false;
    }
  }

  libinput_unref(context);
  return true;
}

/**
 * add a new scanned device to the static devices array, growing its size when necessary
 * @param path device file path
 * @param capabilities device input capabilities
 * @return true if the operation succeeded
 */
static bool add_scanned_device(char *path, libinput_capability capabilities) {
  /* Double array size every 2^n elements */
  if ((num_devices & (num_devices + 1)) == 0) {
      struct input_device *tmp = realloc(devices, (2 * num_devices + 1) * sizeof(struct input_device));
      if (!tmp) {
        perror("could not reallocate memory for devices array");
        return false;
      }
      devices = tmp;
  }

  devices[num_devices].path = path;
  devices[num_devices].capabilities = capabilities;
  num_devices++;

  return true;
}

/**
 * reset the array of scanned devices and free any dynamically allocated memory
 */
static void reset_scanned_devices(void) {
  if (!devices) {
    return;
  }

  for (size_t i = 0; i < num_devices; ++i) {
    free(devices[i].path);
  }
  free(devices);

  devices = NULL;
  num_devices = 0;
}

/**
 * Handle libinput touch / pointer events
 * @param state driver state to use
 * @param event libinput event
 */
static void read_pointer(libinput_drv_state_t *state, struct libinput_event *event) {
  struct libinput_event_touch *touch_event = NULL;
  struct libinput_event_pointer *pointer_event = NULL;
  libinput_lv_event_t *evt = NULL;
  enum libinput_event_type type = libinput_event_get_type(event);

  /* We only care about these events */
  if (type != LIBINPUT_EVENT_TOUCH_MOTION &&
      type != LIBINPUT_EVENT_TOUCH_DOWN &&
      type != LIBINPUT_EVENT_TOUCH_UP &&
      type != LIBINPUT_EVENT_POINTER_MOTION &&
      type != LIBINPUT_EVENT_POINTER_BUTTON &&
      type != LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE)
    return;

  /* We need to read unrotated display dimensions directly from the driver because libinput won't account
   * for any rotation inside of LVGL */
  lv_disp_drv_t *drv = lv_disp_get_default()->driver;

  /* ignore more than 2 fingers as it will only confuse LVGL */
  touch_event = libinput_event_get_touch_event(event);
  if (touch_event && libinput_event_touch_get_slot(touch_event) > 1)
    return;

  evt = new_event(state);

  switch (type) {
    case LIBINPUT_EVENT_TOUCH_MOTION:
    case LIBINPUT_EVENT_TOUCH_DOWN: {
      lv_coord_t x = libinput_event_touch_get_x_transformed(touch_event, drv->physical_hor_res > 0 ? drv->physical_hor_res : drv->hor_res) - drv->offset_x;
      lv_coord_t y = libinput_event_touch_get_y_transformed(touch_event, drv->physical_ver_res > 0 ? drv->physical_ver_res : drv->ver_res) - drv->offset_y;
      if (x < 0 || x > drv->hor_res || y < 0 || y > drv->ver_res) {
        break; /* ignore touches that are out of bounds */
      }
      evt->point.x = x;
      evt->point.y = y;
      evt->pressed = LV_INDEV_STATE_PR;
      break;
    }
    case LIBINPUT_EVENT_TOUCH_UP:
      evt->pressed = LV_INDEV_STATE_REL;
      break;
    case LIBINPUT_EVENT_POINTER_MOTION:
      pointer_event = libinput_event_get_pointer_event(event);
      evt->point.x += libinput_event_pointer_get_dx(pointer_event);
      evt->point.y += libinput_event_pointer_get_dy(pointer_event);
      evt->point.x = LV_CLAMP(0, evt->point.x, drv->hor_res - 1);
      evt->point.y = LV_CLAMP(0, evt->point.y, drv->ver_res - 1);
      break;
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
      pointer_event = libinput_event_get_pointer_event(event);
      lv_coord_t x_pointer = libinput_event_pointer_get_absolute_x_transformed(pointer_event, drv->physical_hor_res > 0 ? drv->physical_hor_res : drv->hor_res) - drv->offset_x;
      lv_coord_t y_pointer = libinput_event_pointer_get_absolute_y_transformed(pointer_event, drv->physical_ver_res > 0 ? drv->physical_ver_res : drv->ver_res) - drv->offset_y;
      if (x_pointer < 0 || x_pointer > drv->hor_res || y_pointer < 0 || y_pointer > drv->ver_res) {
        break; /* ignore pointer events that are out of bounds */
      }
      evt->point.x = x_pointer;
      evt->point.y = y_pointer;
      break;
    case LIBINPUT_EVENT_POINTER_BUTTON:
      pointer_event = libinput_event_get_pointer_event(event);
      enum libinput_button_state button_state = libinput_event_pointer_get_button_state(pointer_event); 
      evt->pressed = button_state == LIBINPUT_BUTTON_STATE_RELEASED ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
      break;
    default:
      break;
  }
}

/**
 * Handle libinput keyboard events
 * @param state driver state to use
 * @param event libinput event
 */
static void read_keypad(libinput_drv_state_t *state, struct libinput_event *event) {
  struct libinput_event_keyboard *keyboard_event = NULL;
  enum libinput_event_type type = libinput_event_get_type(event);
  libinput_lv_event_t *evt = NULL;
  switch (type) {
    case LIBINPUT_EVENT_KEYBOARD_KEY:
      evt = new_event(state);
      keyboard_event = libinput_event_get_keyboard_event(event);
      enum libinput_key_state key_state = libinput_event_keyboard_get_key_state(keyboard_event);
      uint32_t code = libinput_event_keyboard_get_key(keyboard_event);
#if USE_XKB
      evt->key_val = xkb_process_key_state(&(state->xkb_state), code, key_state == LIBINPUT_KEY_STATE_PRESSED);
#else
      switch(code) {
        case KEY_BACKSPACE:
          evt->key_val = LV_KEY_BACKSPACE;
          break;
        case KEY_ENTER:
          evt->key_val = LV_KEY_ENTER;
          break;
        case KEY_PREVIOUS:
          evt->key_val = LV_KEY_PREV;
          break;
        case KEY_NEXT:
          evt->key_val = LV_KEY_NEXT;
          break;
        case KEY_UP:
          evt->key_val = LV_KEY_UP;
          break;
        case KEY_LEFT:
          evt->key_val = LV_KEY_LEFT;
          break;
        case KEY_RIGHT:
          evt->key_val = LV_KEY_RIGHT;
          break;
        case KEY_DOWN:
          evt->key_val = LV_KEY_DOWN;
          break;
        case KEY_TAB:
          evt->key_val = LV_KEY_NEXT;
          break;
        default:
          evt->key_val = 0;
          break;
      }
#endif /* USE_XKB */
      if (evt->key_val != 0) {
        /* Only record button state when actual output is produced to prevent widgets from refreshing */
        evt->pressed = (key_state == LIBINPUT_KEY_STATE_RELEASED) ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
      }
      break;
    default:
      break;
  }
}

static int open_restricted(const char *path, int flags, void *user_data)
{
  LV_UNUSED(user_data);
  int fd = open(path, flags);
  return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
  LV_UNUSED(user_data);
  close(fd);
}

#endif /* USE_LIBINPUT || USE_BSD_LIBINPUT */
