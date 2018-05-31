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
    LV_ROT_DEGREE_M270 = -3,
    LV_ROT_DEGREE_M180 = -2,
    LV_ROT_DEGREE_M90 = -1,
    LV_ROT_DEGREE_0 = 0,
    LV_ROT_DEGREE_P90 = 1,
    LV_ROT_DEGREE_P180 = 2,
    LV_ROT_DEGREE_P270 = 3,
} lv_rotation_t;

typedef enum
{
    LV_STAGE_TOPLEFT = 0,
    LV_STAGE_TOPRIGHT,
    LV_STAGE_BOTRIGHT,
    LV_STAGE_BOTLEFT,
} lv_indev_calib_state_t;

/**********************
 *  GLOBAL MACROS
 **********************/
#define GET_POINT_CALIB(p,r,p1,p2)   (((int32_t)p - (int32_t)p1) * (int32_t)r) / ((int32_t)p2 - (int32_t)p1);
#define  SWAP(x, y)    do { typeof(x) SWAP = x; x = y; y = SWAP; } while (0)

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

inline int par_send(const ili9341_t *dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_par_wr_cs(dev->spi_dev, false);
    lv_par_wr_dc(dev->spi_dev, dc); /* command mode */
    int err = lv_par_write(dev->spi_dev, data, len, wordsize);
    lv_par_wr_cs(dev->spi_dev, true);
    return err;
}

inline int spi3wire_send(const ili9341_t *dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_spi_wr_cs(dev->spi_dev, false);
    lv_spi_set_preemble(dev->spi_dev, LV_SPI_COMMAND, dc, 1);
    int err = lv_spi_transaction(dev->spi_dev, NULL, data, len, wordsize);
    lv_spi_clr_preemble(dev->spi_dev, LV_SPI_COMMAND);
    lv_spi_wr_cs(dev->spi_dev, true);
    return err;
}

inline int spi4wire_send(const ili9341_t *dev, bool dc, uint8_t* data, uint8_t len, uint8_t wordsize)
{
    lv_spi_wr_cs(dev->spi_dev, false);
    lv_spi_wr_dc(dev->spi_dev, dc); /* command mode */
    int err = lv_spi_transaction(dev->spi_dev, NULL, data, len, wordsize);
    lv_spi_wr_cs(dev->spi_dev, true);
    return err;
}



#ifdef __cplusplus
}
#endif
#endif /*LV_DRV_COMMON_H */
