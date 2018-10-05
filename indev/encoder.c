/**
 * @file encoder.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "encoder.h"
#if USE_ENCODER

#include "lvgl/lv_core/lv_group.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static int16_t enc_diff = 0;
static lv_indev_state_t state = LV_INDEV_STATE_REL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the encoder
 */
void encoder_init(void)
{
    /*Nothing to init*/
}

/**
 * Get encoder (i.e. mouse wheel) ticks difference and pressed state
 * @param data store the read data here
 * @return false: all ticks and button state are handled
 */
bool encoder_read(lv_indev_data_t * data)
{
    data->state = state;
    data->enc_diff = enc_diff;
    enc_diff = 0;

    return false;       /*No more data to read so return false*/
}

/**
 * It is called periodically from the SDL thread to check mouse wheel state
 * @param event describes the event
 */
void encoder_handler(SDL_Event * event)
{
    switch(event->type) {
        case SDL_MOUSEWHEEL:
            // Scroll down (y = -1) means positive encoder turn,
            // so invert it
            enc_diff += -(event->wheel.y);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(event->button.button == SDL_BUTTON_MIDDLE) {
                state = LV_INDEV_STATE_PR;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if(event->button.button == SDL_BUTTON_MIDDLE) {
                state = LV_INDEV_STATE_REL;
            }
            break;
        default:
            break;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
