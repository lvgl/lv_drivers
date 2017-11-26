/**
 * @file sdl_kb.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "keyboard.h"
#if USE_KEYBOARD

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static uint32_t keycode_to_ascii(uint32_t sdl_key);

/**********************
 *  STATIC VARIABLES
 **********************/
static uint32_t last_key;
static bool is_pressed;
static bool reported;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the eyboard
 */
void keyboard_init(void)
{
    /*Nothing to init*/
}

/**
 * Get the last pressed or released character from the PC's keyboard
 * @param key point to variable to store the key. (set to 0 if nothing happened)
 * @return true: the left mouse button is pressed, false: released
 */
bool keyboard_read(uint32_t *key)
{
    if(reported){
        *key = 0;
        return false;
    }

    *key = keycode_to_ascii(last_key);
    reported = true;

    return is_pressed;
}


void keyboard_handler(SDL_Event *event)
{
    /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
    switch( event->type ){
        case SDL_KEYDOWN:
            last_key = event->key.keysym.sym;
            is_pressed = true;
            reported = false;
            break;
        case SDL_KEYUP:
            is_pressed = false;
            reported = false;
            break;
        default:
            break;

    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static uint32_t keycode_to_ascii(uint32_t sdl_key)
{
    switch(sdl_key) {
        case SDLK_KP_PLUS: return '+';
        case SDLK_KP_MINUS: return '-';
        case SDLK_KP_ENTER: return '\n';
        case '\r': return '\n';
        default: return sdl_key;
    }
}
#endif
