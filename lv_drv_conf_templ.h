/**
 * @file lv_drv_conf.h
 * 
 */

#if 0 /*Remove this to enable the content*/

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

/*********************
 *  DISPLAY DRIVERS
 *********************/

/*-----------------------------
 *  Monitor of PC
 *----------------------------*/
#define USE_MONITOR    0
#if USE_MONITOR
#define MONITOR_HOR_RES   800
#define MONITOR_VER_RES   480
#endif

/*----------------
 *    SSD1963
 *--------------*/
#define USE_SSD1963   0
#if USE_SSD1963
#define SSD1963_PAR_CS    PAR_CSX
#define SSD1963_RST_PORT  IO_PORTX
#define SSD1963_RST_PIN   IO_PINX
#define SSD1963_BL_PORT   IO_PORTX
#define SSD1963_BL_PIN    IO_PINX
/*Display settings*/
#define SSD1963_HDP     479
#define SSD1963_HT      531
#define SSD1963_HPS     43
#define SSD1963_LPS     8
#define SSD1963_HPW     10
#define SSD1963_VDP     271
#define SSD1963_VT      288
#define SSD1963_VPS     12
#define SSD1963_FPS     4
#define SSD1963_VPW     10
#define SSD1963_HS_NEG  0   /*Negative hsync*/
#define SSD1963_VS_NEG  0   /*Negative vsync*/
#define SSD1963_ORI     0   /*0, 90, 180, 270*/
#define SSD1963_LV_COLOR_DEPTH 16
#endif

/*----------------
 *    R61581
 *--------------*/
#define USE_R61581   0
#if USE_R61581 != 0
#define R61581_PAR_CS    PAR_CSX
#define R61581_RS_PORT   IO_PORTX
#define R61581_RS_PIN    IO_PINX
#define R61581_RST_PORT  IO_PORTX
#define R61581_RST_PIN   IO_PINX
#define R61581_RST_PORT  IO_PORTX
#define R61581_RST_PIN   IO_PINX
#define R61581_BL_PORT   IO_PORTX
#define R61581_BL_PIN    IO_PINX
/*Display settings*/
#define R61581_HOR_RES     480
#define R61581_VER_RES     320
#define R61581_HDP     479
#define R61581_HT      531
#define R61581_HPS     43
#define R61581_LPS     8
#define R61581_HPW     10
#define R61581_VDP     271
#define R61581_VT      319
#define R61581_VPS     12
#define R61581_FPS     4
#define R61581_VPW     10
#define R61581_HS_NEG  0   /*Negative hsync*/
#define R61581_VS_NEG  0   /*Negative vsync*/
#define R61581_ORI     180   /*0, 90, 180, 270*/
#define R61581_LV_COLOR_DEPTH 16
#endif

/*------------------------------
 *  ST7565 (Monochrome, low res.)
 *-----------------------------*/
#define USE_ST7565  0
#if USE_ST7565 != 0
#define ST7565_DRV      HW_SPIX_CSX
#define ST7565_RST_PORT IO_PORTX
#define ST7565_RST_PIN  IO_PINX
#define ST7565_RS_PORT  IO_PORTX
#define ST7565_RS_PIN   IO_PINX
#endif  /*USE_ST7565*/

/*-----------------------------------------
 *  Linux frame buffer device (/dev/fbx)
 *-----------------------------------------*/
#define USE_FBDEV       0
#if USE_FBDEV != 0
#define FBDEV_PATH  "/dev/fb0"
#endif

/*====================
 * Input devices
 *===================*/

/*--------------
 *    XPT2046
 *--------------*/
#define USE_XPT2046     0
#if USE_XPT2046
#define XPT2046_SPI_DRV     HW_SPIX_CSX
#define XPT2046_IRQ_PORT    IO_PORTX
#define XPT2046_IRQ_PIN     IO_PINX
#define XPT2046_HOR_RES     480
#define XPT2046_VER_RES     320
#define XPT2046_X_MIN       200
#define XPT2046_Y_MIN       200 
#define XPT2046_X_MAX       3800
#define XPT2046_Y_MAX       3800
#define XPT2046_AVG         4 
#define XPT2046_INV         0 
#endif

/*-----------------
 *    FT5406EE8
 *-----------------*/
#define USE_FT5406EE8   0
#if USE_FT5406EE8
/*No settings*/
#endif

/*-------------------------------
 *    Mouse or touchpad on PC
 *------------------------------*/
#define USE_MOUSE       0
#if USE_MOUSE
/*No settings*/
#endif


/*-------------------------------
 *   Keyboard of a PC
 *------------------------------*/
#define USE_KEYBOARD    0
#if USE_KEYBOARD
/*No settings*/
#endif

#endif  /*LV_DRV_CONF_H*/

#endif /*Remove this to enable the content*/
