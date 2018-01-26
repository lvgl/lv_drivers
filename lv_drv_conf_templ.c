/**
 * @file lv_drv_conf.c
 * 
 */

/*
 * This file is optionnal. If you dont want to inline all HAL functions
 */
#if 0 /*Remove this to enable the content*/

/*********************
 *      INCLUDES
 *********************/
#include "lv_drv_conf.h"

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

/**********************
 *      MACROS
 **********************/

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

/*-------------------
 *  Delay API
 *-------------------*/
#if LV_DRIVER_ENABLE_DELAY
inline void lv_delay_us(uint32_t us)
{
    //Do the dependant port here
}

inline void lv_delay_ms(uint32_t ms)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  Common API
 *-------------------*/
#if LV_DRIVER_ENABLE_COMMON
inline void lv_gpio_write(uint16_t pin, uint8_t val)
{
    //Do the dependant port here
}

inline uint8_t lv_gpio_read(uint16_t pin)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  I2C API
 *-------------------*/
#if LV_DRIVER_ENABLE_I2C
inline int lv_i2c_write(void* i2c_dev, const uint8_t* reg, const void* data_out, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_read(void* i2c_dev, const uint8_t* reg, void* data_in, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_write16(void* i2c_dev, const uint16_t* reg, const void* data_out, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_read16(void* i2c_dev, const uint16_t* reg, void* data_in, uint16_t datalen)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  SPI API
 *-------------------*/
#if LV_DRIVER_ENABLE_SPI
inline int lv_spi_transaction(void* spi_dev, void* data_in, const void* data_out, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_spi_repeat(void* spi_dev, const void* template, uint32_t repeats, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_spi_set_command(void* spi_dev, uint32_t value, uint8_t bits)
{
    //Do the dependant port here
}

inline int lv_spi_set_address(void* spi_dev, uint32_t value, uint8_t bits)
{
    //Do the dependant port here 
}

inline int lv_spi_set_dummy(void* spi_dev, uint8_t bits)
{
    //Do the dependant port here  
}

inline int lv_spi_clear_command(void* spi_dev)
{
    //Do the dependant port here  
}

inline int lv_spi_clear_address(void* spi_dev)
{
    //Do the dependant port here
}

inline int lv_spi_clear_dummy(void* spi_dev)
{
    //Do the dependant port here 
}

#endif
/*-------------------
 *  Parallel API
 *-------------------*/
#if LV_DRIVER_ENABLE_PAR
inline int lv_par_write(void* par_dev, const void* data_out, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_par_read(void* par_dev, void* data_in, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}
#endif

#endif /*Remove this to enable the content*/
