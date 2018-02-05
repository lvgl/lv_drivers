/**
 * @file SSD1963.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "ILI9340.h"
#if USE_ILI9340

#include <stdbool.h>
#include "lvgl/lv_core/lv_vdb.h"
#include LV_DRV_DISP_INCLUDE
#include LV_DRV_DELAY_INCLUDE

/*********************
 *      DEFINES
 *********************/
#define ILI9340_CMD_MODE     0
#define ILI9340_DATA_MODE    1

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static bool cmd_mode = true;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ili9340_init(void)
{
    DisplayResetEnable();
    LV_DRV_DELAY_MS(200);
    DisplayResetDisable();
    LV_DRV_DELAY_MS(200);
    
    W_CMD(0x0028);
    CMD_END();
    W_CMD(0x0011);
    CMD_END();
    W_CMD(0x00C0);
    W_DAT(0x0026);
    W_DAT(0x0004);
    CMD_END();
    W_CMD(0x00C1);
    W_DAT(0x0004);
    CMD_END();
    W_CMD(0x00C5);
    W_DAT(0x0034);
    W_DAT(0x0040);
    CMD_END();
    W_CMD(0x0036);
    W_DAT(0x0068);
    CMD_END();
    W_CMD(0x00B1);
    W_DAT(0x0000);
    W_DAT(0x0018);
    CMD_END();
    W_CMD(0x00B6);
    W_DAT(0x000A);
    W_DAT(0x00A2);
    CMD_END();
    W_CMD(0x00C7);
    W_DAT(0x00C0);
    CMD_END();
    W_CMD(0x003A);
    W_DAT(0x0055);
    CMD_END();

    W_CMD(0x00E0);
    W_DAT(0x001F);
    W_DAT(0x001B);
    W_DAT(0x0018);
    W_DAT(0x000B);
    W_DAT(0x000F);
    W_DAT(0x0009);
    W_DAT(0x0046);
    W_DAT(0x00B5);
    W_DAT(0x0037);
    W_DAT(0x000A);
    W_DAT(0x000C);
    W_DAT(0x0007);
    W_DAT(0x0007);
    W_DAT(0x0005);
    W_DAT(0x0000);
    CMD_END();
    W_CMD(0x00E1);
    W_DAT(0x0000);
    W_DAT(0x0024);
    W_DAT(0x0027);
    W_DAT(0x0004);
    W_DAT(0x0010);
    W_DAT(0x0006);
    W_DAT(0x0039);
    W_DAT(0x0074);
    W_DAT(0x0048);
    W_DAT(0x0005);
    W_DAT(0x0013);
    W_DAT(0x0038);
    W_DAT(0x0038);
    W_DAT(0x003A);
    W_DAT(0x001F);
    CMD_END();
    W_CMD(0x002A);
    W_DAT(0x0000);
    W_DAT(0x0000);
    W_DAT(0x0000);
    W_DAT(0x00EF);
    CMD_END();
    W_CMD(0x002B);
    W_DAT(0x0000);
    W_DAT(0x0000);
    W_DAT(0x0001);
    W_DAT(0x003F);
    CMD_END();
    W_CMD(0x0029);
    CMD_END();
    
    DisplayBacklightOn();
 
    LV_DRV_DELAY_MS(30);
    
}


void ili9340_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{

    /*Return if the area is out the screen*/
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > ILI9340_HOR_RES - 1) return;
    if(y1 > ILI9340_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > ILI9340_HOR_RES - 1 ? ILI9340_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > ILI9340_VER_RES - 1 ? ILI9340_VER_RES - 1 : y2;

    //Set the rectangular area
    W_CMD(0x002A);
    W_DAT(act_x1 >> 8);
    W_DAT(0x00FF & act_x1);
    W_DAT(act_x2 >> 8);
    W_DAT(0x00FF & act_x2);

    W_CMD(0x002B);
    W_DAT(act_y1 >> 8);
    W_DAT(0x00FF & act_y1);
    W_DAT(act_y2 >> 8);
    W_DAT(0x00FF & act_y2);

    W_CMD(0x2c);
    //CMD_END();
     int16_t i;
    uint16_t full_w = x2 - x1 + 1;

    DisplaySetData();
    
    LV_DRV_DISP_PAR_CS(0);
    
#if LV_COLOR_DEPTH == 16
    uint16_t act_w = act_x2 - act_x1 + 1;
    for(i = act_y1; i <= act_y2; i++) {
        LV_DRV_DISP_PAR_WR_ARRAY((uint16_t*)color_p, act_w);
        color_p += full_w;
    }
    LV_DRV_DISP_PAR_CS(1);
#else
    int16_t j;
    for(i = act_y1; i <= act_y2; i++) {
        for(j = 0; j <= act_x2 - act_x1 + 1; j++) {
            LV_DRV_DISP_PAR_WR_WORD(color_p[j]);
            color_p += full_w;
        }
    }
#endif

    lv_flush_ready();
}

void ili9340_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
    /*Return if the area is out the screen*/
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > ILI9340_HOR_RES - 1) return;
    if(y1 > ILI9340_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > ILI9340_HOR_RES - 1 ? ILI9340_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > ILI9340_VER_RES - 1 ? ILI9340_VER_RES - 1 : y2;
   
    //Set the rectangular area
    W_CMD(0x002A);
    W_DAT(act_x1 >> 8);
    W_DAT(0x00FF & act_x1);
    W_DAT(act_x2 >> 8);
    W_DAT(0x00FF & act_x2);

    W_CMD(0x002B);
    W_DAT(act_y1 >> 8);
    W_DAT(0x00FF & act_y1);
    W_DAT(act_y2 >> 8);
    W_DAT(0x00FF & act_y2);

    W_CMD(0x2c);
    
    LV_DRV_DISP_PAR_CS(0);
    DisplaySetData();

    uint16_t color16 = lv_color_to16(color);
    uint32_t size = (act_x2 - act_x1 + 1) * (act_y2 - act_y1 + 1);
    uint32_t i;
    for(i = 0; i < size; i++) {
        LV_DRV_DISP_PAR_WR_WORD(color16);
    }
    LV_DRV_DISP_PAR_CS(1);
}

void ili9340_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    
    /*Return if the area is out the screen*/
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > ILI9340_HOR_RES - 1) return;
    if(y1 > ILI9340_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > ILI9340_HOR_RES - 1 ? ILI9340_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > ILI9340_VER_RES - 1 ? ILI9340_VER_RES - 1 : y2;
   
    //Set the rectangular area
    W_CMD(0x002A);
    W_DAT(act_x1 >> 8);
    W_DAT(0x00FF & act_x1);
    W_DAT(act_x2 >> 8);
    W_DAT(0x00FF & act_x2);

    W_CMD(0x002B);
    W_DAT(act_y1 >> 8);
    W_DAT(0x00FF & act_y1);
    W_DAT(act_y2 >> 8);
    W_DAT(0x00FF & act_y2);

    W_CMD(0x2c);
     int16_t i;
    uint16_t full_w = x2 - x1 + 1;
    
    LV_DRV_DISP_PAR_CS(0);
    DisplaySetData();
    
#if LV_COLOR_DEPTH == 16
    uint16_t act_w = act_x2 - act_x1 + 1;
    for(i = act_y1; i <= act_y2; i++) {
        LV_DRV_DISP_PAR_WR_ARRAY((uint16_t*)color_p, act_w);
        color_p += full_w;
    }
    LV_DRV_DISP_PAR_CS(1);
#else
    int16_t j;
    for(i = act_y1; i <= act_y2; i++) {
        for(j = 0; j <= act_x2 - act_x1 + 1; j++) {
            LV_DRV_DISP_PAR_WR_WORD(color_p[j]);
            color_p += full_w;
        }
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
