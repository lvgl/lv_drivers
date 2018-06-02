/**
 * @file lv_drv_common.h
 * 
 */ 

#ifndef LV_DRV_COMMON_H
#define LV_DRV_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_drv_conf.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *     TYPEDEFS
 **********************/
typedef enum
{
    LV_ROT_DEGREE_0 = 0,
    LV_ROT_DEGREE_90 = 1,
    LV_ROT_DEGREE_180 = 2,
    LV_ROT_DEGREE_270 = 3,
} lv_rotation_t;

/**********************
 *  GLOBAL MACROS
 **********************/
#define GET_POINT_CALIB(p, size, min, max)   (((int32_t)p - (int32_t)min) * (int32_t)size) / ((int32_t)max - (int32_t)min);
#define SWAP(x, y)    do { typeof(x) SWAP = x; x = y; y = SWAP; } while (0)

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
static inline int i2c_send(const lv_i2c_handle_t i2c_dev, uint8_t reg, uint8_t* data, uint8_t len)
{
    return lv_i2c_write(i2c_dev, &reg, data, len);
}

static inline int par_send(const lv_spi_handle_t spi_dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_par_wr_cs(spi_dev, false);
    lv_par_wr_dc(spi_dev, dc); /* command mode */
    int err = lv_par_write(spi_dev, data, len, wordsize);
    lv_par_wr_cs(spi_dev, true);
    return err;
}

static inline int spi3wire_send(const lv_spi_handle_t spi_dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_spi_wr_cs(spi_dev, false);
    lv_spi_set_preemble(spi_dev, LV_SPI_COMMAND, dc, 1);
    int err = lv_spi_transaction(spi_dev, NULL, data, len, wordsize);
    lv_spi_clr_preemble(spi_dev, LV_SPI_COMMAND);
    lv_spi_wr_cs(spi_dev, true);
    return err;
}

static inline int spi4wire_send(const lv_spi_handle_t spi_dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_spi_wr_cs(spi_dev, false);
    lv_spi_wr_dc(spi_dev, dc); /* command mode */
    int err = lv_spi_transaction(spi_dev, NULL, data, len, wordsize);
    lv_spi_wr_cs(spi_dev, true);
    return err;
}



#ifdef __cplusplus
}
#endif
#endif /*LV_DRV_COMMON_H */
