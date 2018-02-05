/**
 * @file ILI9341.c
 * @author zaltora  (https://github.com/Zaltora)
 * @copyright MIT License.
 */
//TODO: remove this
#define USE_ILI9341 1
/*********************
 *      INCLUDES
 *********************/
#include "ILI9341.h"
#if USE_ILI9341 != 0

#include <stdbool.h>
#include "lvgl/lv_core/lv_vdb.h"


//TODO: remove this
#define ILI9341_DEBUG 1

/*********************
 *      DEFINES
 *********************/
#ifndef ILI9341_SPI4_SUPPORT
#define ILI9341_SPI4_SUPPORT 1
#endif

#ifndef ILI9341_SPI3_SUPPORT
#define ILI9341_SPI3_SUPPORT 1
#endif

#ifndef ILI9341_MANUAL_DC
#define ILI9341_MANUAL_DC 1
#endif

#ifndef ILI9341_MANUAL_CS
#define ILI9341_MANUAL_CS 1
#endif

/* utility */
#define BIT_MASK(a, b) (((unsigned) -1 >> (31 - (b))) & ~((1U << (a)) - 1))

/* ILI9341 regular commands */
#define ILI9341_NO_OPERATION            (0x00)
#define ILI9341_SOFT_RESET              (0x01)
#define ILI9341_DIS_INFOS               (0x04)
#define ILI9341_DIS_STATUS              (0x09)
#define ILI9341_DIS_POWER_MODE          (0x0A)
#define ILI9341_DIS_MADCTL              (0x0B)
#define ILI9341_DIS_PIXEL_FMT           (0x0C)
#define ILI9341_DIS_IMAGE_FMT           (0x0D)
#define ILI9341_DIS_SIGNAL_MODE         (0x0E)
#define ILI9341_DIS_SELF_DIA_RESULT     (0x0F)
#define ILI9341_SLEEP_ON                (0x10)
#define ILI9341_SLEEP_OFF               (0x11)
#define ILI9341_PARTIAL_MODE            (0x12)
#define ILI9341_NORMAL_MODE             (0x13)
#define ILI9341_INVERSION_OFF           (0x20)
#define ILI9341_INVERSION_ON            (0x21)
#define ILI9341_GAMMA_SET               (0x26)
#define ILI9341_DIS_OFF                 (0x28)
#define ILI9341_DIS_ON                  (0x29)
#define ILI9341_COLUMN_ADDR_SET         (0x2A)
#define ILI9341_PAGE_ADDR_SET           (0x2B)

#define ILI9341_MEMORY_WRITE            (0x2C)
#define ILI9341_COLOR_SET               (0x2D)
#define ILI9341_MEMORY_READ             (0x2E)
#define ILI9341_PARTIAL_AREA            (0x30)
#define ILI9341_VERT_SCROLLING          (0x33)
#define ILI9341_TEARING_LINE_OFF        (0x34)
#define ILI9341_TEARING_LINE_ON         (0x35)
#define ILI9341_MEMORY_ACCES_CTR        (0x36)
#define ILI9341_VERT_SCROLL_START_ADDR  (0x37)
#define ILI9341_IDLE_MODE_OFF           (0x38)
#define ILI9341_IDLE_MODE_ON            (0x39)
#define ILI9341_PIXEL_FMT_SET           (0x3A)
#define ILI9341_WRITE_MEMORY_CONTINUE   (0x3C)
#define ILI9341_READ_MEMORY_CONTINUE    (0x3E)
#define ILI9341_SET_TEAR_SCANLINE       (0x44)
#define ILI9341_GET_SCANLINE            (0x45)
#define ILI9341_DIS_SET_BRIGHTNESS      (0x51)

#define ILI9341_DIS_GET_BRIGHTNESS      (0x52)
#define ILI9341_WRITE_CTRL_DIS          (0x53)
#define ILI9341_READ_CTRL_DIS           (0x54)
#define ILI9341_WRITE_ADAP_BRIGTH_CTR   (0x55)
#define ILI9341_READ_ADAP_BRIGTH_CTR    (0x56)
#define ILI9341_WRITE_CABC_MIN_BRIGTH   (0x5E)
#define ILI9341_READ_CABC_MIN_BRIGTH    (0x5F)
#define ILI9341_READ_ID1                (0xDA)
#define ILI9341_READ_ID2                (0xDB)
#define ILI9341_READ_ID3                (0xDC)

/* ILI9341 Extended commands */

#define ILI9341_RGB_INT_SIG_CTR         (0xB0)
#define ILI9341_FRAME_CTR_NORMAL        (0xB1)
#define ILI9341_FRAME_CTR_IDLE          (0xB2)
#define ILI9341_FRAME_CTR_PARTIAL       (0xB3)
#define ILI9341_DIS_INVERSION_CTR       (0xB4)
#define ILI9341_BLANK_PORCH_CTR         (0xB5)
#define ILI9341_DIS_FUNCTION_CTR        (0xB6)
#define ILI9341_ENTRY_MODE_SET          (0xB7)
#define ILI9341_BACKLIGTH_CTR_1         (0xB8)
#define ILI9341_BACKLIGTH_CTR_2         (0xB9)
#define ILI9341_BACKLIGTH_CTR_3         (0xBA)
#define ILI9341_BACKLIGTH_CTR_4         (0xBB)
#define ILI9341_BACKLIGTH_CTR_5         (0xBC)
#define ILI9341_BACKLIGTH_CTR_7         (0xBE)
#define ILI9341_BACKLIGTH_CTR_8         (0xBF)
#define ILI9341_PWR_CTR_1               (0xC0)
#define ILI9341_PWR_CTR_2               (0xC1)
#define ILI9341_VCOM_CTR_1              (0xC5)
#define ILI9341_VCOM_CTR_2              (0xC7)
#define ILI9341_NV_MEMORY_WRITE         (0xD0)
#define ILI9341_NV_MEMORY_PROTECT_KEY   (0xD1)
#define ILI9341_NV_MEMORY_STATUS        (0xD2)
#define ILI9341_READ_ID4                (0xD3)
#define ILI9341_POS_GAMMA_COR           (0xE0)
#define ILI9341_NEG_GAMMA_COR           (0xE1)
#define ILI9341_DIG_GAMMA_CTR1          (0xE2)
#define ILI9341_DIG_GAMMA_CTR2          (0xE3)
#define ILI9341_UNKNOW                  (0xEF)
#define ILI9341_INTERFACE_CTR           (0xF6)

#define ILI9341_PWR_CTR_A               (0xCB)
#define ILI9341_PWR_CTR_B               (0xCF)
#define ILI9341_TIMING_CTR_A            (0xE8)
#define ILI9341_UNKNOW_2                (0xE9)  //This cmd exist ?
#define ILI9341_TIMING_CTR_B            (0xEA)
#define ILI9341_PWR_ON_SEQ_CTR          (0xED)
#define ILI9341_ENABLE_3G               (0xF2)
#define ILI9341_PUMP_RATIO_CTR          (0xF7)


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int _sendCommand(uint8_t cmd);
static int _sendCommandData(uint8_t cmd, uint8_t* data, uint8_t len);

/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *      MACROS
 **********************/
#if ILI9341_DEBUG
#include <stdio.h>
#define debug(fmt, ...) printf("%s: " fmt "\n", "ILI9341", ## __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ili9341_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if (x2 < 0 || y2 < 0 || x1 > LV_HOR_RES - 1 || y1 > LV_VER_RES - 1)
    {
        lv_flush_ready();
        return;
    }

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > LV_HOR_RES - 1 ? LV_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > LV_VER_RES - 1 ? LV_VER_RES - 1 : y2;


    lv_flush_ready();
}

void ili9341_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/

    /*Return if the area is out the screen*/
    if (x2 < 0 || y2 < 0 || x1 > LV_HOR_RES - 1 || y1 > LV_VER_RES - 1)
    {
        return;
    }

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > LV_HOR_RES - 1 ? LV_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > LV_VER_RES - 1 ? LV_VER_RES - 1 : y2;

}

void ili9341_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if (x2 < 0 || y2 < 0 || x1 > LV_HOR_RES - 1 || y1 > LV_VER_RES - 1)
    {
        return;
    }

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > LV_HOR_RES - 1 ? LV_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > LV_VER_RES - 1 ? LV_VER_RES - 1 : y2;

}

/* Perform default init routine according */
int ili9341_init(const ili9341_t *dev)
{
    int err;
    switch (dev->height)
    {
    case 240:
        break;
    case 320:
        break;
    default:
        debug("Unsupported screen height");
        return -ENOTSUP;
    }

    switch (dev->protocol)
    {
#if (ILI9341_SPI4_SUPPORT)
    case ILI9341_PROTO_SERIAL_8BIT:
        break;
#endif
#if (ILI9341_SPI3_SUPPORT)
    case ILI9341_PROTO_SERIAL_9BIT:
        break;
#endif
    default:
        debug("Unsupported protocol");
        return -EPROTONOSUPPORT;
    }


    if((err = ili9341_unknow(dev)))
        return err;

    ili9341_pwr_ctrl_b_t pwr_ctrl_b_config = { 0 };
    pwr_ctrl_b_config.dc_ena = 1 ;
    pwr_ctrl_b_config.pceq = 1 ;

    if((err = ili9341_power_control_b(dev, pwr_ctrl_b_config)))
        return err;

    ili9341_pwr_seq_ctrl_t pwr_seq_ctrl_config  = { 0 };
    pwr_seq_ctrl_config.cp23_soft_start = 0 ;
    pwr_seq_ctrl_config.cp1_soft_start = 2 ;
    pwr_seq_ctrl_config.en_ddvdh = 3 ;
    pwr_seq_ctrl_config.en_vgh = 1 ;
    pwr_seq_ctrl_config.en_vgl = 2 ;
    pwr_seq_ctrl_config.ddvdh_enh = 1;

    if((err = ili9341_power_on_seq_ctrl(dev, pwr_seq_ctrl_config)))
        return err;

    ili9341_timing_ctrl_a_t timing_ctrl_a_config = { 0 };
    timing_ctrl_a_config.now = 1 ;

    if((err = ili9341_timing_ctrl_a(dev, timing_ctrl_a_config)))
        return err;

    ili9341_pwr_ctrl_a_t power_ctrl_a_config = { 0 };
    power_ctrl_a_config.reg_vd = 4 ;
    power_ctrl_a_config.vbc = 2 ;

    if((err = ili9341_pwr_ctrl_a(dev, power_ctrl_a_config)))
        return err;

    ili9341_pump_ratio_ctrl_t pump_ration_ctrl_config = { 0 };
    pump_ration_ctrl_config.ratio = 2 ;

    if((err = ili9341_pump_ratio_ctrl(dev, pump_ration_ctrl_config)))
        return err;

    ili9341_timing_ctrl_b_t timing_ctrl_b_config = { 0 };
    if((err = ili9341_timing_ctrl_b(dev, timing_ctrl_b_config)))
        return err;

    ili9341_pwr_ctrl_1_t pwr_ctrl_1_config = { 0 } ;
    pwr_ctrl_1_config.vrh = 0b100011;
    if((err = ili9341_pwr_ctrl_1(dev, pwr_ctrl_1_config)))
        return err;

    ili9341_pwr_ctrl_2_t pwr_ctrl_2_config = { 0 } ;
    if((err = ili9341_pwr_ctrl_2(dev, pwr_ctrl_2_config)))
        return err;

    ili9341_vcom_ctrl_1_t vcom_ctrl_1_config = { 0 } ;
    vcom_ctrl_1_config.vmh = 0b00111110;
    vcom_ctrl_1_config.vml = 0b00101000;
    if((err = ili9341_vcom_ctrl_1(dev, vcom_ctrl_1_config)))
        return err;

    ili9341_vcom_ctrl_2_t vcom_ctrl_2_config = { 0 } ;
    vcom_ctrl_2_config.vmf = 6 ;
    vcom_ctrl_2_config.nvm = 1 ;
    if((err = ili9341_vcom_ctrl_2(dev, vcom_ctrl_2_config)))
        return err;

    ili9341_mem_ctrl_t ili9341_mem_ctrl_config = { 0 } ;
    ili9341_mem_ctrl_config.mx = 1 ;
    ili9341_mem_ctrl_config.bgr = 1 ;
    if((err = ili9341_mem_ctrl(dev, ili9341_mem_ctrl_config)))
        return err;

    ili9341_vert_scroll_start_t ili9341_vert_scroll_start_config = { 0 };
    if((err = ili9341_vert_scroll_start(dev, ili9341_vert_scroll_start_config)))
        return err;

    ili9341_px_fmt_t ili9341_pixel_fmt_config = { 0 };
    ili9341_pixel_fmt_config.dbi = 0b101 ;
    ili9341_pixel_fmt_config.dpi = 0b101 ;
    if((err = ili9341_pixel_fmt(dev, ili9341_pixel_fmt_config)))
        return err;

    ili9341_frame_rate_ctrl_t ili9341_frame_rate_ctrl_config = { 0 };
    ili9341_frame_rate_ctrl_config.rtna = 0b11000 ;
    if((err = ili9341_frame_rate_ctrl(dev, ili9341_frame_rate_ctrl_config)))
        return err;

    ili9341_dis_fn_ctrl_t ili9341_dis_fn_ctrl_config = { 0 };
    ili9341_dis_fn_ctrl_config.ptg = 0b10 ;
    ili9341_dis_fn_ctrl_config.isc = 0b0010 ;
    ili9341_dis_fn_ctrl_config.rev = 0b1 ;
    ili9341_dis_fn_ctrl_config.nl = 0b100111;
    if((err = ili9341_display_fn_ctrl(dev, ili9341_dis_fn_ctrl_config)))
        return err;

    ili9341_ena_3g_t ili9341_ena_3g_config = { 0 };
    if((err = ili9341_enable_3g(dev, ili9341_ena_3g_config)))
        return err;

    ili9341_gamma_set_t ili9341_gamma_set_config = { 0 };
    ili9341_gamma_set_config.gamma_set = 0x01 ;
    if((err = ili9341_gamma_set(dev, ili9341_gamma_set_config)))
        return err;


    ili9341_gamma_cor_t ili9341_gamma_config = { 0 };
    ili9341_gamma_config.v63 = 0x0F ;
    ili9341_gamma_config.v62 = 0x31 ;
    ili9341_gamma_config.v61 = 0x2B ;
    ili9341_gamma_config.v59 = 0x0C ;
    ili9341_gamma_config.v57 = 0x0E ;
    ili9341_gamma_config.v50 = 0x08 ;
    ili9341_gamma_config.v43 = 0x4E ;
    ili9341_gamma_config.v36 = 0x0F ;
    ili9341_gamma_config.v27 = 0x01 ;
    ili9341_gamma_config.v20 = 0x37 ;
    ili9341_gamma_config.v13 = 0x07 ;
    ili9341_gamma_config.v6 = 0x10 ;
    ili9341_gamma_config.v4 = 0x03 ;
    ili9341_gamma_config.v2 = 0x0E ;
    ili9341_gamma_config.v1 = 0x09 ;
    ili9341_gamma_config.v0 = 0x00 ;
    if((err = ili9341_gamma_cor(dev, ili9341_gamma_pos, ili9341_gamma_config)))
        return err;

    ili9341_gamma_config.v63 = 0x00 ;
    ili9341_gamma_config.v62 = 0x0E ;
    ili9341_gamma_config.v61 = 0x14 ;
    ili9341_gamma_config.v59 = 0x03 ;
    ili9341_gamma_config.v57 = 0x11 ;
    ili9341_gamma_config.v50 = 0x07 ;
    ili9341_gamma_config.v43 = 0x31 ;
    ili9341_gamma_config.v36 = 0x0C ;
    ili9341_gamma_config.v27 = 0x01 ;
    ili9341_gamma_config.v20 = 0x48 ;
    ili9341_gamma_config.v13 = 0x08 ;
    ili9341_gamma_config.v6 = 0x0F ;
    ili9341_gamma_config.v4 = 0x0C ;
    ili9341_gamma_config.v2 = 0x31 ;
    ili9341_gamma_config.v1 = 0x36 ;
    ili9341_gamma_config.v0 = 0x0F ;
    if((err = ili9341_gamma_cor(dev, ili9341_gamma_neg, ili9341_gamma_config)))
        return err;

    //TODO: can we swap order for them ? to wait only 120 ms
    ili9341_sleep_out(dev);
    ili9341_displat_on(dev);

    return 0;
}


int ili9341_unknow(const ili9341_t *dev)
{
    uint8_t data[3] = { 0x03, 0x80, 0x02  };
    return _sendCommandData(ILI9341_UNKNOW, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_power_control_b(const ili9341_t *dev, ili9341_pwr_ctrl_b_t config)
{
    uint8_t data[3] = { 0 };
    data[0] = 0x00 ;
    data[1] = 0x81 | config.power_ctrl << 3 | config.drv_ena << 5 | config.pceq << 6 ;
    data[2] = 0x20 | config.drv_vmh | (config.drv_vml & BIT_MASK(0,0)) << 3  |
        (config.drv_vml & BIT_MASK(1,2)) << 5 | config.dc_ena << 4;
    return _sendCommandData(ILI9341_PWR_CTR_B, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_power_on_seq_ctrl(const ili9341_t *dev, ili9341_pwr_seq_ctrl_t config)
{
    uint8_t data[4] = { 0 };
    data[0] = 0x44 | config.cp23_soft_start | config.cp1_soft_start << 4;
    data[1] = config.en_ddvdh | config.en_vcl << 4 ;
    data[2] = config.en_vgl | config.en_vgh << 4 ;
    data[3] = 0x01 | config.ddvdh_enh << 7 ;
    return _sendCommandData(ILI9341_PWR_ON_SEQ_CTR, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_timing_ctrl_a(const ili9341_t *dev, ili9341_timing_ctrl_a_t config)
{
    uint8_t data[3] = { 0 };
    data[0] = 0x84 | config.now ;
    data[1] = config.cr | config.eq <<4 ;
    data[2] = 0x78 | config.pc ;
    return _sendCommandData(ILI9341_TIMING_CTR_A, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_pwr_ctrl_a(const ili9341_t *dev, ili9341_pwr_ctrl_a_t config)
{
    uint8_t data[5] = { 0 };
    data[0] = 0x39 ;
    data[1] = 0x2C ;
    data[2] = 0x00 ;
    data[3] = 0x30 | config.reg_vd ;
    data[4] = config.vbc ;
    return _sendCommandData(ILI9341_PWR_CTR_A, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_pump_ratio_ctrl(const ili9341_t *dev, ili9341_pump_ratio_ctrl_t config)
{
    if (config.ratio <= 1)
    {
       return -ENOTSUP; //Reserved area
    }
    uint8_t data[1] = { 0 };
    data[0] = config.ratio << 4;
    return _sendCommandData(ILI9341_PUMP_RATIO_CTR, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_timing_ctrl_b(const ili9341_t *dev, ili9341_timing_ctrl_b_t config)
{
    uint8_t data[2] = { 0 };
    data[0] = *(uint8_t*)&config;
    return _sendCommandData(ILI9341_TIMING_CTR_B, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_pwr_ctrl_1(const ili9341_t *dev, ili9341_pwr_ctrl_1_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = config.vrh ;
    return _sendCommandData(ILI9341_PWR_CTR_1, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_pwr_ctrl_2(const ili9341_t *dev, ili9341_pwr_ctrl_2_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = 0x10 | config.bt ;
    return _sendCommandData(ILI9341_PWR_CTR_2, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_vcom_ctrl_1(const ili9341_t *dev, ili9341_vcom_ctrl_1_t config)
{
    uint8_t data[2] = { 0 };
    data[0] = config.vmh ;
    data[1] = config.vml ;
    return _sendCommandData(ILI9341_VCOM_CTR_1, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_vcom_ctrl_2(const ili9341_t *dev, ili9341_vcom_ctrl_2_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = *(uint8_t*)&config ;
    return _sendCommandData(ILI9341_VCOM_CTR_2, data, sizeof(data));
}

int ili9341_mem_ctrl(const ili9341_t *dev, ili9341_mem_ctrl_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = *(uint8_t*)&config & BIT_MASK(2,7);
    return _sendCommandData(ILI9341_MEMORY_ACCES_CTR, data, sizeof(data));
}

int ili9341_vert_scroll_start(const ili9341_t *dev, ili9341_vert_scroll_start_t config)
{
    uint8_t data[2] = { 0 };
    data[0] = config.vsp >> 8 ;
    data[1] = config.vsp & 0x0F ;
    return _sendCommandData(ILI9341_VERT_SCROLL_START_ADDR, data, sizeof(data));
}

int ili9341_pixel_fmt(const ili9341_t *dev, ili9341_px_fmt_t config)
{
    if (config.dbi != 5 && config.dbi != 6)
    {
       return -ENOTSUP; //Reserved area
    }
    else if (config.dpi != 5 && config.dpi != 6)
    {
       return -ENOTSUP; //Reserved area
    }
    uint8_t data[1] = { 0 };
    data[0] = config.dbi | config.dpi << 4 ;
    return _sendCommandData(ILI9341_PIXEL_FMT_SET, data, sizeof(data));
}

int ili9341_frame_rate_ctrl(const ili9341_t *dev, ili9341_frame_rate_ctrl_t config)
{
    uint8_t data[2] = { 0 };
    data[0] = config.diva ;
    data[1] = config.rtna ;
    return _sendCommandData(ILI9341_FRAME_CTR_NORMAL, data, sizeof(data));
}

int ili9341_display_fn_ctrl(const ili9341_t *dev, ili9341_dis_fn_ctrl_t config)
{
    uint8_t data[3] = { 0 };
    data[0] = config.pt | config.ptg << 2 ;
    data[1] = config.isc | config.sm << 4 | config.ss << 5 |
        config.gs << 6 | config.rev << 7 ;
    data[2] = config.nl ;
    //data[3] = config.pcdiv ;  //FIXME: This data is needed ? datasheet said no ?
    return _sendCommandData(ILI9341_DIS_FUNCTION_CTR, data, sizeof(data));
}

/**
 *  @pre EXTC should be high to enable this CMD
 */
int ili9341_enable_3g(const ili9341_t *dev, ili9341_ena_3g_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = 0x02 | config.ena_3g ;
    return _sendCommandData(ILI9341_ENABLE_3G, data, sizeof(data));
}

int ili9341_gamma_set(const ili9341_t *dev, ili9341_gamma_set_t config)
{
    uint8_t data[1] = { 0 };
    data[0] = config.gamma_set ;
    return _sendCommandData(ILI9341_GAMMA_SET, data, sizeof(data));
}

int ili9341_gamma_cor(const ili9341_t *dev, ili9341_gamma_type_e type, ili9341_gamma_cor_t config)
{
    uint8_t data[15] = { 0 };
    data[0] = config.v63 ;
    data[1] = config.v62 ;
    data[2] = config.v61 ;
    data[3] = config.v59 ;
    data[4] = config.v57 ;
    data[5] = config.v50 ;
    data[6] = config.v43 ;
    data[7] = config.v27 | config.v36 << 4 ;
    data[8] = config.v20 ;
    data[9] = config.v13 ;
    data[10] = config.v6 ;
    data[11] = config.v4 ;
    data[12] = config.v2 ;
    data[13] = config.v1 ;
    data[14] = config.v0 ;

    return _sendCommandData( type == ili9341_gamma_pos ?
        ILI9341_POS_GAMMA_COR : ILI9341_NEG_GAMMA_COR,   data, sizeof(data));
}

int ili9341_sleep_out(const ili9341_t *dev)
{
    //TODO: Wait 120 ms ? (what is the best way to do it ?
    return _sendCommand(ILI9341_SLEEP_OFF);
}

int ili9341_displat_on(const ili9341_t *dev)
{
    return _sendCommand(ILI9341_DIS_ON);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int _sendCommand(uint8_t cmd)
{
    debug("cmd: %02X",cmd);
    return -EIO;
}

static int _sendCommandData(uint8_t cmd, uint8_t* data, uint8_t len)
{
#if ILI9341_DEBUG
    printf("ILI9341: cmd: %02X data: ", cmd);
    for (uint8_t i = 0 ; i < len ; i++)
    {
        printf("%02X ",data[i]);
    }
    printf("\n");
#endif
    return -EIO;
}

#endif
