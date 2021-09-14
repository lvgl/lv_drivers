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
#include <poll.h>
#include <dirent.h>
#include <libinput.h>

#if USE_BSD_LIBINPUT
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

#if USE_XKB
#include "xkb.h"
#endif /* USE_XKB */

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

static void read_pointer(struct libinput_event *event);
static void read_keypad(struct libinput_event *event);

static int open_restricted(const char *path, int flags, void *user_data);
static void close_restricted(int fd, void *user_data);

/**********************
 *  STATIC VARIABLES
 **********************/
static struct input_device *devices = NULL;
static size_t num_devices = 0;

static int libinput_fd;
static int libinput_button;
static int libinput_key_val;
static const int timeout = 0; // do not block
static const nfds_t nfds = 1;
static struct pollfd fds[1];
static lv_point_t most_recent_touch_point = { .x = 0, .y = 0};

static struct libinput *libinput_context;
static struct libinput_device *libinput_device;
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
 * find connected input device with specific capabilities
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
 * find connected input devices with specific capabilities
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
 * reconfigure the device file for libinput
 * @param dev_name set the libinput device filename
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 */
bool libinput_set_file(char* dev_name)
{
  // This check *should* not be necessary, yet applications crashes even on NULL handles.
  // citing libinput.h:libinput_path_remove_device:
  // > If no matching device exists, this function does nothing.
  if (libinput_device) {
    libinput_device = libinput_device_unref(libinput_device);
    libinput_path_remove_device(libinput_device);
  }

  libinput_device = libinput_path_add_device(libinput_context, dev_name);
  if(!libinput_device) {
    perror("unable to add device to libinput context:");
    return false;
  }
  libinput_device = libinput_device_ref(libinput_device);
  if(!libinput_device) {
    perror("unable to reference device within libinput context:");
    return false;
  }

  libinput_button = LV_INDEV_STATE_REL;
  libinput_key_val = 0;

  return true;
}

/**
 * Initialize the libinput interface
 */
void libinput_init(void)
{
  libinput_device = NULL;
  libinput_context = libinput_path_create_context(&interface, NULL);

  const char *path = LIBINPUT_NAME;
  if(path == NULL || !libinput_set_file(path)) {
      fprintf(stderr, "unable to add device \"%s\" to libinput context: %s\n", path ? path : "NULL", strerror(errno));
      return;
  }
  libinput_fd = libinput_get_fd(libinput_context);

  /* prepare poll */
  fds[0].fd = libinput_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

#if USE_XKB
  xkb_init();
#endif
}

/**
 * Get the current position and state of the libinput
 * @param indev_drv driver object itself
 * @param data store the libinput data here
 */
void libinput_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  struct libinput_event *event;
  int rc = 0;

  rc = poll(fds, nfds, timeout);
  switch (rc){
    case -1:
      perror(NULL);
    case 0:
      goto report_most_recent_state;
    default:
      break;
  }
  libinput_dispatch(libinput_context);
  while((event = libinput_get_event(libinput_context)) != NULL) {
    switch (indev_drv->type) {
      case LV_INDEV_TYPE_POINTER:
        read_pointer(event);
        break;
      case LV_INDEV_TYPE_KEYPAD:
        read_keypad(event);
        break;
      default:
        break;
    }
    libinput_event_destroy(event);
  }
report_most_recent_state:
  data->point.x = most_recent_touch_point.x;
  data->point.y = most_recent_touch_point.y;
  data->state = libinput_button;
  data->key = libinput_key_val;
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

    char *path = malloc((11 + strlen(ent->d_name)) * sizeof(char));
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

    libinput_capability capabilities = LIBINPUT_CAPABILITY_NONE;
    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD)
        && (libinput_device_keyboard_has_key(device, KEY_ENTER) || libinput_device_keyboard_has_key(device, KEY_KPENTER)))
    {
      capabilities |= LIBINPUT_CAPABILITY_KEYBOARD;
    }
    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER)) {
      capabilities |= LIBINPUT_CAPABILITY_POINTER;
    }
    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)) {
      capabilities |= LIBINPUT_CAPABILITY_TOUCH;
    }

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

  for (int i = 0; i < num_devices; ++i) {
    free(devices[i].path);
  }
  free(devices);

  devices = NULL;
  num_devices = 0;
}

/**
 * Handle libinput touch / pointer events
 * @param event libinput event
 */
static void read_pointer(struct libinput_event *event) {
  struct libinput_event_touch *touch_event = NULL;
  struct libinput_event_pointer *pointer_event = NULL;
  enum libinput_event_type type = libinput_event_get_type(event);
  switch (type) {
    case LIBINPUT_EVENT_TOUCH_MOTION:
    case LIBINPUT_EVENT_TOUCH_DOWN:
      touch_event = libinput_event_get_touch_event(event);
      most_recent_touch_point.x = libinput_event_touch_get_x_transformed(touch_event, LV_HOR_RES);
      most_recent_touch_point.y = libinput_event_touch_get_y_transformed(touch_event, LV_VER_RES);
      libinput_button = LV_INDEV_STATE_PR;
      break;
    case LIBINPUT_EVENT_TOUCH_UP:
      libinput_button = LV_INDEV_STATE_REL;
      break;
    case LIBINPUT_EVENT_POINTER_MOTION:
      pointer_event = libinput_event_get_pointer_event(event);
      most_recent_touch_point.x += libinput_event_pointer_get_dx(pointer_event);
      most_recent_touch_point.y += libinput_event_pointer_get_dy(pointer_event);
      most_recent_touch_point.x = most_recent_touch_point.x < 0 ? 0 : most_recent_touch_point.x;
      most_recent_touch_point.x = most_recent_touch_point.x > LV_HOR_RES - 1 ? LV_HOR_RES - 1 : most_recent_touch_point.x;
      most_recent_touch_point.y = most_recent_touch_point.y < 0 ? 0 : most_recent_touch_point.y;
      most_recent_touch_point.y = most_recent_touch_point.y > LV_VER_RES - 1 ? LV_VER_RES - 1 : most_recent_touch_point.y;
      break;
    case LIBINPUT_EVENT_POINTER_BUTTON:
      pointer_event = libinput_event_get_pointer_event(event);
      enum libinput_button_state button_state = libinput_event_pointer_get_button_state(pointer_event); 
      libinput_button = button_state == LIBINPUT_BUTTON_STATE_RELEASED ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
      break;
    default:
      break;
  }
}

/**
 * Handle libinput keyboard events
 * @param event libinput event
 */
static void read_keypad(struct libinput_event *event) {
  struct libinput_event_keyboard *keyboard_event = NULL;
  enum libinput_event_type type = libinput_event_get_type(event);
  switch (type) {
    case LIBINPUT_EVENT_KEYBOARD_KEY:
      keyboard_event = libinput_event_get_keyboard_event(event);
      enum libinput_key_state key_state = libinput_event_keyboard_get_key_state(keyboard_event);
      uint32_t code = libinput_event_keyboard_get_key(keyboard_event);
#if USE_XKB
      libinput_key_val = xkb_process_key(code, key_state == LIBINPUT_KEY_STATE_PRESSED);
#else
      switch(code) {
        case KEY_BACKSPACE:
          libinput_key_val = LV_KEY_BACKSPACE;
          break;
        case KEY_ENTER:
          libinput_key_val = LV_KEY_ENTER;
          break;
        case KEY_PREVIOUS:
          libinput_key_val = LV_KEY_PREV;
          break;
        case KEY_NEXT:
          libinput_key_val = LV_KEY_NEXT;
          break;
        case KEY_UP:
          libinput_key_val = LV_KEY_UP;
          break;
        case KEY_LEFT:
          libinput_key_val = LV_KEY_LEFT;
          break;
        case KEY_RIGHT:
          libinput_key_val = LV_KEY_RIGHT;
          break;
        case KEY_DOWN:
          libinput_key_val = LV_KEY_DOWN;
          break;
        case KEY_TAB:
          libinput_key_val = LV_KEY_NEXT;
          break;
        default:
          libinput_key_val = 0;
          break;
      }
#endif /* USE_XKB */
      if (libinput_key_val != 0) {
        /* Only record button state when actual output is produced to prevent widgets from refreshing */
        libinput_button = (key_state == LIBINPUT_KEY_STATE_RELEASED) ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
      }
      break;
    default:
      break;
  }
}

static int open_restricted(const char *path, int flags, void *user_data)
{
  int fd = open(path, flags);
  return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
  close(fd);
}

#endif /* USE_LIBINPUT || USE_BSD_LIBINPUT */
