/**
 * @file FT5406EE8.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "FT5406EE8.h"
#if USE_FT5406EE8

#include "hw/per/i2c.h"
#include "hw/per/io.h"
#include "hw/per/tick"
#include <stddef.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define FT5406EE8_I2C_ADR   0x38

#define FT5406EE8_FINGER_MAX 10

/*Register adresses*/
#define FT5406EE8_REG_DEVICE_MODE 0x00
#define FT5406EE8_REG_GEST_ID     0x01
#define FT5406EE8_REG_TD_STATUS   0x02
#define FT5406EE8_REG_XH          0x05
#define FT5406EE8_REG_XL          0x06
#define FT5406EE8_REG_YH          0x03
#define FT5406EE8_REG_YL          0x04

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool ft5406ee8_get_touch_num(void);
static bool ft5406ee8_read_finger1(int16_t * x,  int16_t * y);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * 
 */
void ft5406ee8_init(void)
{

}

bool ft5406ee8_get(int16_t * x, int16_t * y)
{   
    static int16_t x_last;
    static int16_t y_last;
    bool valid = true;   
    
    valid = ft5406ee8_get_touch_num();
    if(valid == true) {
        valid = ft5406ee8_read_finger1(x, y);
    }
     
     if(valid == true) {
        *x = (uint32_t)((uint32_t)*x * 320) / 2048;
        *y = (uint32_t)((uint32_t)*y * 240) / 2048; 
         
         
        x_last = *x;
        y_last = *y;
     }
     else {
        *x = x_last;
        *y = y_last;
    }

    return valid;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool ft5406ee8_get_touch_num(void)
{
    bool ok = true;
    uint8_t t_num;
    hw_res_t res;
    
    /* Read number of touched points */
    res = i2c_read(FT540EE8_I2C_DRV, 
                     FT5406EE8_I2C_ADR, FT5406EE8_REG_TD_STATUS, &t_num, 1);         
    
    if(res != HW_RES_OK) {
        ok = false;
        /*TODO log device error*/
    }
    
    /* Error if not touched or too much finger */
    if (t_num > FT5406EE8_FINGER_MAX || t_num == 0){
        ok = false;
    }

    return ok;
}

static bool ft5406ee8_read_finger1(int16_t * x, int16_t * y)
{
    bool valid = true;
    hw_res_t res = HW_RES_OK;
    
    uint8_t temp_xH = 0;
    uint8_t temp_xL = 0;
    uint8_t temp_yH = 0;
    uint8_t temp_yL = 0;
    
    /*Read Y High and low byte*/
    res = i2c_read(FT540EE8_I2C_DRV, 
                   FT5406EE8_I2C_ADR, FT5406EE8_REG_YH, &temp_yH, 1);

    if(res == HW_RES_OK) {
        res = i2c_read(FT540EE8_I2C_DRV, 
                       FT5406EE8_I2C_ADR, FT5406EE8_REG_YL, &temp_yL, 1);
    } else {
        valid = false;
    }
    
    /*The upper two bit must be 2 on valid press*/
    if(valid != false && ((temp_yH >> 6) & 0xFF) != 2) {
        valid = false;    /*mark the error*/
    }
    
    /*Read X High and low byte*/
    if(valid != false) {
        res = i2c_read(FT540EE8_I2C_DRV, 
                           FT5406EE8_I2C_ADR, FT5406EE8_REG_XH, &temp_xH, 1); 
        if(res == HW_RES_OK) {
            res = i2c_read(FT540EE8_I2C_DRV, 
                           FT5406EE8_I2C_ADR, FT5406EE8_REG_XL, &temp_xL, 1);   
        } else {
            valid = false;
        }
    }       
    
    /*Save the result*/
    if(valid != false) {
        *x = (temp_xH & 0x0F) << 8;
        *x += temp_xL;
        *y = (temp_yH & 0x0F) << 8;
        *y += temp_yL;
    } else {
        *x = 0;
        *y = 0;
    }
    
    if(res != HW_RES_OK) {
        /*TODO Log device error*/
    }
    
    return valid;
}

#endif
