/**
 * @file ILI9341.h
 * @author zaltora  (https://github.com/Zaltora)
 * @copyright MIT License.
 * @warn ILI9341 Datasheet can be incomplete or got some errors. Take care !!
 */

#ifndef ILI9341_H
#define ILI9341_H

//TODO: remove this
#define USE_ILI9341 1

/*********************
 *      INCLUDES
 *********************/
//#include "../../lv_drv_conf.h"
#if USE_ILI9341 != 0

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "lvgl/lv_misc/lv_color.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * I/O protocols
 */
typedef enum
{
    ILI9341_PROTO_8080_8BIT = 0, //!<  8080 8 bits
    ILI9341_PROTO_8080_16BIT, //!<  8080 16 bits
    ILI9341_PROTO_8080_9BIT, //!<  8080 9 bits
    ILI9341_PROTO_8080_18BIT, //!<  8080 18 bits
    ILI9341_PROTO_SERIAL_9BIT, //!<  SPI 9 bits
    ILI9341_PROTO_SERIAL_8BIT, //!<  SPI 8 bits
} ili9341_protocol_t;


/**
 * Device descriptor
 */
typedef struct
{
    ili9341_protocol_t protocol;
    union
    {
#if (ILI9341_PAR_SUPPORT)
        void* par_dev;
#endif
#if (ILI9341_SPI4_SUPPORT) || (ILI9341_SPI3_SUPPORT)
        void* spi_dev;
#endif
    };

#if (ILI9341_MANUAL_CS)
		uint8_t cs_pin;
#endif
#if (ILI9341_SPI4_SUPPORT) && (ILI9341_MANUAL_DC)
    uint8_t dc_pin;               //!< Data/Command GPIO pin, used by ILI9341_PROTO_SPI4
#endif
    uint16_t width;                //!< Screen width, 320 or 240
    uint16_t height;               //!< Screen height, 240 or 320
} ili9341_t;


//register details
typedef struct
{
    uint16_t power_ctrl : 2;
    uint16_t drv_ena : 1;   //< For VCOM driving ability enhancement, DRV_ena = 1: Enable
    uint16_t pceq : 1; //<  PC and EQ operation for power saving ( 0 disable ; 1 enable )
    uint16_t drv_vmh : 3 ;
    uint16_t drv_vml : 3 ;
    uint16_t dc_ena  : 1 ; //< Discharge path enable. Enable high for ESD protection, 1: enable
} ili9341_pwr_ctrl_b_t;

typedef struct
{
    uint16_t cp23_soft_start : 2 ; //< soft start control
    uint16_t cp1_soft_start : 2 ;
    uint16_t en_ddvdh : 2 ; //< power on sequence control
    uint16_t en_vcl : 2 ;
    uint16_t en_vgl : 2 ;
    uint16_t en_vgh : 2 ;
    uint16_t ddvdh_enh : 1 ; //< DDVDH enhance mode(only for 8 external capacitors)
} ili9341_pwr_seq_ctrl_t;

typedef struct
{
    uint8_t now : 1 ; //< gate driver non-overlap timing control
    uint8_t cr : 1 ; //< CR timing control
    uint8_t eq : 1 ; //< EQ timing control
    uint8_t pc : 2 ; //< pre-charge timing control
} ili9341_timing_ctrl_a_t;

typedef struct
{
    uint8_t reg_vd : 3 ; //< vcore control
    uint8_t vbc : 3 ; //<  ddvdh control
} ili9341_pwr_ctrl_a_t;

typedef struct
{
    uint8_t ratio : 2 ; //< ratio control (00 / 01 reserved)
} ili9341_pump_ratio_ctrl_t;

typedef struct
{
    uint8_t vg_sw_t1 : 2 ;
    uint8_t vg_sw_t2 : 2 ;
    uint8_t vg_sw_t3 : 2 ;
    uint8_t vg_sw_t4 : 2 ;
} ili9341_timing_ctrl_b_t;

typedef struct
{
    uint8_t vrh : 6 ;
} ili9341_pwr_ctrl_1_t;

typedef struct
{
    uint8_t bt : 3 ;
} ili9341_pwr_ctrl_2_t;

typedef struct
{
    uint8_t vmh : 7 ;
    uint8_t :1 ;
    uint8_t vml : 7 ;
    uint8_t :1 ;
} ili9341_vcom_ctrl_1_t;

typedef struct
{
    uint8_t vmf : 7 ;
    uint8_t nvm : 1 ;
} ili9341_vcom_ctrl_2_t;

typedef struct
{
    uint8_t : 2 ;
    uint8_t mh : 1 ;
    uint8_t bgr : 1 ;
    uint8_t ml : 1 ;
    uint8_t mv : 1 ;
    uint8_t mx : 1 ;
    uint8_t my : 1 ;
} ili9341_mem_ctrl_t;

typedef struct
{
    uint16_t vsp ;
} ili9341_vert_scroll_start_t;

typedef struct
{
    uint8_t dbi : 3 ;
    uint8_t : 1 ;
    uint8_t dpi : 3 ;
    uint8_t : 1 ;
} ili9341_px_fmt_t;

typedef struct
{
    uint8_t diva : 2 ;
    uint8_t rtna : 5 ;
} ili9341_frame_rate_ctrl_t;

typedef struct
{
    uint32_t pt : 2 ;
    uint32_t ptg : 2 ;
    uint32_t isc : 4 ;
    uint32_t sm : 1 ;
    uint32_t ss : 1 ;
    uint32_t gs : 1 ;
    uint32_t rev : 1 ;
    uint32_t nl : 6 ;
    uint32_t pcdiv : 6 ;
} ili9341_dis_fn_ctrl_t;

typedef struct
{
    uint8_t ena_3g : 1 ;
} ili9341_ena_3g_t;

typedef struct
{
    uint8_t gamma_set ;
} ili9341_gamma_set_t;

typedef struct
{
    uint8_t v63 : 4 ;
    uint8_t : 4 ;
    uint8_t v62 : 6 ;
    uint8_t : 2 ;
    uint8_t v61 : 6 ;
    uint8_t : 2 ;
    uint8_t v59 : 4 ;
    uint8_t :4 ;
    uint8_t v57 : 5 ;
    uint8_t : 3 ;
    uint8_t v50 : 4 ;
    uint8_t : 4 ;
    uint8_t v43 : 7 ;
    uint8_t : 1;
    uint8_t v36 : 4 ;
    uint8_t v27 : 4 ;
    uint8_t v20 : 7 ;
    uint8_t : 1 ;
    uint8_t v13 : 4 ;
    uint8_t : 4;
    uint8_t v6 : 5 ;
    uint8_t : 3 ;
    uint8_t v4 : 4 ;
    uint8_t : 4 ;
    uint8_t v2 : 6 ;
    uint8_t : 2 ;
    uint8_t v1 : 6 ;
    uint8_t : 2 ;
    uint8_t v0 : 4 ;
    uint8_t : 4 ;

} ili9341_gamma_cor_t;

typedef enum {
    ili9341_gamma_pos,
    ili9341_gamma_neg,
} ili9341_gamma_type_e;


/**********************
 *      MACROS
 **********************/


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/* Flush the content of the internal buffer the specific area on the display
 * This function is required only when LV_VDB_SIZE != 0 in lv_conf.h
 * @param x1 First x point coordinate
 * @param y1 First y point coordinate
 * @param x2 Second x point coordinate
 * @param y2 Second y point coordinate
 * @param color_p Pointer to VDB buffer (color sized)
 */
void ili9341_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);

/* Write a color (called 'fill') to the a specific area on the display
 * This function is required only when LV_VDB_SIZE == 0 in lv_conf.h
 * @param x1 First x point coordinate
 * @param y1 First y point coordinate
 * @param x2 Second x point coordinate
 * @param y2 Second y point coordinate
 * @param color Color to fill
 */
void ili9341_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t  color);

/* Write a pixel array (called 'map') to the a specific area on the display
 * This function is required only when LV_VDB_SIZE == 0 in lv_conf.h
 * @param x1 First x point coordinate
 * @param y1 First y point coordinate
 * @param x2 Second x point coordinate
 * @param y2 Second y point coordinate
 * @param color_p Pointer to VDB buffer (color sized)
 */
void ili9341_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);

/**
 * Default init for ILI9341
 * @param dev Pointer to device descriptor
 * @return Non-zero if error occured
 */
int ili9341_init(const ili9341_t *dev);




//Command
int ili9341_unknow(const ili9341_t *dev);
int ili9341_power_control_b(const ili9341_t *dev, ili9341_pwr_ctrl_b_t config);
int ili9341_power_on_seq_ctrl(const ili9341_t *dev, ili9341_pwr_seq_ctrl_t config);
int ili9341_timing_ctrl_a(const ili9341_t *dev, ili9341_timing_ctrl_a_t config);
int ili9341_pwr_ctrl_a(const ili9341_t *dev, ili9341_pwr_ctrl_a_t config);
int ili9341_pump_ratio_ctrl(const ili9341_t *dev, ili9341_pump_ratio_ctrl_t config);
int ili9341_timing_ctrl_b(const ili9341_t *dev, ili9341_timing_ctrl_b_t config);
int ili9341_pwr_ctrl_1(const ili9341_t *dev, ili9341_pwr_ctrl_1_t config);
int ili9341_pwr_ctrl_2(const ili9341_t *dev, ili9341_pwr_ctrl_2_t config);
int ili9341_vcom_ctrl_1(const ili9341_t *dev, ili9341_vcom_ctrl_1_t config);
int ili9341_vcom_ctrl_2(const ili9341_t *dev, ili9341_vcom_ctrl_2_t config);
int ili9341_mem_ctrl(const ili9341_t *dev, ili9341_mem_ctrl_t config);
int ili9341_vert_scroll_start(const ili9341_t *dev, ili9341_vert_scroll_start_t config);
int ili9341_pixel_fmt(const ili9341_t *dev, ili9341_px_fmt_t config);
int ili9341_frame_rate_ctrl(const ili9341_t *dev, ili9341_frame_rate_ctrl_t config);
int ili9341_display_fn_ctrl(const ili9341_t *dev, ili9341_dis_fn_ctrl_t config);
int ili9341_enable_3g(const ili9341_t *dev, ili9341_ena_3g_t config);
int ili9341_gamma_set(const ili9341_t *dev, ili9341_gamma_set_t config);
int ili9341_gamma_cor(const ili9341_t *dev, ili9341_gamma_type_e type, ili9341_gamma_cor_t config);

//No parameters commands
int ili9341_sleep_out(const ili9341_t *dev);
int ili9341_displat_on(const ili9341_t *dev);

///////////////////////////////////////////////////////////////////////////


#endif

#endif
