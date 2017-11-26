/**
 * @file mouse.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "mouse.h"
#if USE_MOUSE != 0

#include <SDL2/SDL.h>

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
static bool left_button_down = false;
static int16_t last_x = 0;
static int16_t last_y = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the mouse
 */
void mouse_init(void)
{

}

/**
 * Get the current position and state of the mouse
 * @param x point to variable, the x coordinate will be stored here
 * @param y point to variable, the y coordinate will be stored here
 * @return true: the left mouse button is pressed, false: released
 */
bool mouse_get(int16_t * x, int16_t * y)
{
	*x = last_x;
	*y = last_y;

	return left_button_down;

}

/**
 * It will be called from the main SDL thread
 */
void mouse_handler(SDL_Event *event)
{
    switch (event->type) {
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT)
                left_button_down = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                left_button_down = true;
                last_x = event->motion.x;
                last_y = event->motion.y;
            }
            break;
        case SDL_MOUSEMOTION:
            last_x = event->motion.x;
            last_y = event->motion.y;

            break;
    }

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
