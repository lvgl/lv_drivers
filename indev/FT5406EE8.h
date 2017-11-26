/**
 * @file FT5406EE8.h
 * 
 */

#ifndef FT5406EE8_H
#define FT5406EE8_H

/*********************
 *      INCLUDES
 *********************/
#include "lv_drv_conf.h"
#if USE_FT5406EE8

/*********************
 *      DEFINES
 *********************/

/***********************************/
/********** DEVICE MODES ***********/
/***********************************/
#define OPERAT_MD   0x00
#define TEST_MD     0x04
#define SYS_INF_MD  0x01

/***********************************/
/********* OPERATING MODE **********/
/***********************************/
#define DEVICE_MODE 0x00
#define GEST_ID     0x01
#define TD_STATUS   0x02

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ft5406ee8_init(void);
bool ft5406ee8_get(int16_t * x, int16_t * y);

/**********************
 *      MACROS
 **********************/

#endif

#endif
