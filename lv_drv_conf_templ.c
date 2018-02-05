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
inline void lv_delay_us(const uint32_t us)
{
    //Do the dependant port here
}

inline void lv_delay_ms(const uint32_t ms)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  Common API
 *-------------------*/
#if LV_DRIVER_ENABLE_COMMON
inline void lv_gpio_write(lv_gpio_handle_t gpio, const uint8_t val)
{
    //Do the dependant port here
}

inline uint8_t lv_gpio_read(lv_gpio_handle_t gpio)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  I2C API
 *-------------------*/
#if LV_DRIVER_ENABLE_I2C
inline int lv_i2c_write(lv_i2c_handle_t i2c_dev, const uint8_t* reg, const void* data_out, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_read(lv_i2c_handle_t i2c_dev, const uint8_t* reg, void* data_in, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_write16(lv_i2c_handle_t i2c_dev, const uint16_t* reg, const void* data_out, uint16_t datalen)
{
    //Do the dependant port here
}

inline int lv_i2c_read16(lv_i2c_handle_t i2c_dev, const uint16_t* reg, void* data_in, uint16_t datalen)
{
    //Do the dependant port here
}
#endif
/*-------------------
 *  SPI API
 *-------------------*/
#if LV_DRIVER_ENABLE_SPI
inline int lv_spi_transaction(lv_spi_handle_t spi_dev, void* data_in, const void* data_out, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_spi_repeat(lv_spi_handle_t spi_dev, const void* template, uint32_t repeats, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_spi_set_command(lv_spi_handle_t spi_dev, uint32_t value, uint8_t bits)
{
    //Do the dependant port here
}

inline int lv_spi_set_address(lv_spi_handle_t spi_dev, uint32_t value, uint8_t bits)
{
    //Do the dependant port here 
}

inline int lv_spi_set_dummy(lv_spi_handle_t spi_dev, uint8_t bits)
{
    //Do the dependant port here  
}

inline int lv_spi_clear_command(lv_spi_handle_t spi_dev)
{
    //Do the dependant port here  
}

inline int lv_spi_clear_address(lv_spi_handle_t spi_dev)
{
    //Do the dependant port here
}

inline int lv_spi_clear_dummy(lv_spi_handle_t spi_dev)
{
    //Do the dependant port here 
}

#endif
/*-------------------
 *  Parallel API
 *-------------------*/
#if LV_DRIVER_ENABLE_PAR
inline int lv_par_write(lv_par_handle_t par_dev, const void* data_out, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}

inline int lv_par_read(lv_par_handle_t par_dev, void* data_in, uint16_t len, uint8_t word_size)
{
    //Do the dependant port here
}
#endif

#endif /*Remove this to enable the content*/
