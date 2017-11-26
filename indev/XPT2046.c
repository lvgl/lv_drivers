/**
 * @file XPT2046.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "XPT2046.h"
#if USE_XPT2046

#include "hw/per/spi.h"
#include "hw/per/io.h"
#include "hw/per/tick.h"
#include <stddef.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define CMD_X_READ  0b10010000
#define CMD_Y_READ  0b11010000

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y);
static void xpt2046_avg(int16_t * x, int16_t * y);

/**********************
 *  STATIC VARIABLES
 **********************/
int16_t avg_buf_x[XPT2046_AVG];
int16_t avg_buf_y[XPT2046_AVG];
uint8_t avg_last;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/**
 * Initialize the XPT2046
 */
void xpt2046_init(void)
{
    io_set_pin_dir(XPT2046_IRQ_PORT, XPT2046_IRQ_PIN, IO_DIR_IN);
}

/**
 * Get the current position and state of the touchpad
 * @param x point to variable, the x coordinate will be stored here
 * @param y point to variable, the y coordinate will be stored here
 * @return true: the touchpad is pressed, false: released
 */
bool xpt2046_get(int16_t * x, int16_t * y)
{ 
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    bool valid = true;
    uint8_t data;
    
    *x = 0;
    *y = 0;
    
    if(io_get_pin(XPT2046_IRQ_PORT, XPT2046_IRQ_PIN) == 0) {
        spi_cs_en(XPT2046_SPI_DRV);

        data = CMD_X_READ;  /*Read x*/
        spi_xchg(XPT2046_SPI_DRV, &data, NULL, sizeof(data));
        
        data = 0;        /*Read x MSB*/
        spi_xchg(XPT2046_SPI_DRV, &data, &data, sizeof(data));
        *x = data << 8;
        
        data = CMD_Y_READ; /*Until x LSB converted y command can be sent*/
        spi_xchg(XPT2046_SPI_DRV, &data, &data, sizeof(data));        
        *x += data;
        
        data = 0;   /*Read y MSB*/
        spi_xchg(XPT2046_SPI_DRV, &data, &data, sizeof(data));
        *y = data << 8;
        
        data = 0;   /*Read y LSB*/
        spi_xchg(XPT2046_SPI_DRV, &data, &data, sizeof(data));
        *y += data;
        
        /*Normalize Data*/
        *x = *x >> 3;
        *y = *y >> 3;
        xpt2046_corr(x, y);
        xpt2046_avg(x, y);
        
        last_x = *x;
        last_y = *y;
        
        spi_cs_dis(XPT2046_SPI_DRV);
    } else {
        *x = last_x;
        *y = last_y;
        avg_last = 0;
        valid = false;
    }
    
    return valid;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y)
{
#if XPT2046_XY_SWAP != 0
    int16_t swap_tmp;
    swap_tmp = *x;
    *x = *y;
    *y = swap_tmp;
#endif
    
    if((*x) > XPT2046_X_MIN) (*x) -= XPT2046_X_MIN;
    else (*x) = 0;
    
    if((*y) > XPT2046_Y_MIN) (*y) -= XPT2046_Y_MIN;
    else (*y) = 0;

    (*x) = (uint32_t) ((uint32_t)(*x) * XPT2046_HOR_RES) /
                             (XPT2046_X_MAX - XPT2046_X_MIN);

    (*y) = (uint32_t) ((uint32_t)(*y) * XPT2046_VER_RES) /
                             (XPT2046_Y_MAX - XPT2046_Y_MIN);
    
#if XPT2046_X_INV != 0
    (*x) =  XPT2046_HOR_RES - (*x);
#endif
    
#if XPT2046_Y_INV != 0
    (*y) =  XPT2046_VER_RES - (*y);
#endif
    

}


static void xpt2046_avg(int16_t * x, int16_t * y)
{
    /*Shift out the oldest data*/
    uint8_t i;
    for(i = XPT2046_AVG - 1; i > 0 ; i--) {
        avg_buf_x[i] = avg_buf_x[i - 1];
        avg_buf_y[i] = avg_buf_y[i - 1];
    }

    /*Insert the new point*/
    avg_buf_x[0] = *x;
    avg_buf_y[0] = *y;
    if(avg_last < XPT2046_AVG) avg_last++;
    
    /*Sum the x and y coordinates*/
    int32_t x_sum = 0;
    int32_t y_sum = 0;
    for(i = 0; i < avg_last ; i++) {
        x_sum += avg_buf_x[i];
        y_sum += avg_buf_y[i];
    }

    /*Normalize the sums*/
    (*x) = (int32_t)x_sum / avg_last;
    (*y) = (int32_t)y_sum / avg_last;
}

#endif
