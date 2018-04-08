 /**
 * @file AR10XX.h
 * @author zaltora  (https://github.com/Zaltora)
 * @copyright MIT License.
 * XXX: I2C mode, SDO is THE IRQ signal
 * XXX: SPI mode, SIQ is THE IRQ signal
 * XXX: Factory AR1021 write FF to 0x01 and 0x29  of EEPROM, then rebooot
 */

#ifndef AR10XX_H
#define AR10XX_H
#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_drv_conf.h"
#if USE_AR10XX != 0

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "lvgl/lv_misc/lv_color.h"

/*********************
 *      DEFINES
 *********************/

//Update config localy, then update component.
#ifndef AR10XX_BUFFERED
#define AR10XX_BUFFERED (0)
#endif

//error list from AR10XX
#define AR10XX_ERR_SUCCESS               (0x00)
#define AR10XX_ERR_CMD_UNRECOGNIZED      (0x01)
#define AR10XX_ERR_HEADER_UNRECOGNIZED   (0x03)
#define AR10XX_ERR_CMD_TIMEOUT           (0x04)
#define AR10XX_ERR_CANCEL_CALIB_MODE     (0xFC)

#define AR10XX_I2C_ADDR             (0x4D)
#define AR10XX_CMD_IRQ_DELAY_MAX    (500)  //in ms
#define AR10XX_CMD_ANSWER_WAIT      (1000) //in us
#define AR10XX_EEPROM_USER_SIZE     (128)
/**********************
 *      TYPEDEFS
 **********************/

/**
 * I/O protocols
 */
typedef enum
{
    AR10XX_PROTO_I2C = 0, //!<  I2C 2 Wire
    AR10XX_PROTO_SPI, //!<  SPI 4 Wire
} ar10xx_protocol_t;

/**
 * Device descriptor
 */
typedef struct
{
    ar10xx_protocol_t protocol;    //!< Protocol used
    union
    {
#if (AR10XX_I2C_SUPPORT)
        lv_i2c_handle_t i2c_dev;   //!< I2C device descriptor
#endif
#if (AR10XX_SPI_SUPPORT)
        lv_spi_handle_t spi_dev;   //!< SPI device descriptor
#endif
    };
    lv_gpio_handle_t irq_pin;      //!< Interupt pin to detect new data (optionnal) //FIXME: Needed ?
    volatile uint8_t flag_irq;     //!< Store Interupt information
    uint8_t flag_enable;  //FIXME: Needed to prevent user to write into register ?
} ar10xx_t;

typedef struct {
    uint16_t type ;
    uint8_t controler_type : 6 ;
    uint8_t resolution : 2 ;
} ar10xx_id_t;


typedef struct
{



} ar10xx_regmap_t ;

typedef struct
{
   uint8_t pen;
   uint16_t X;
   uint16_t Y;
} ar10xx_read_t ;

/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

//Touch need be disable when you you use this API
// 0 - 255
int ar10xx_set_touch_treshold( ar10xx_t *dev, uint8_t value);
//0 - 10
int ar10xx_set_sensitivity_filter( ar10xx_t *dev, uint8_t value);
//1,4,8,16,32,64,128
int ar10xx_set_sampling_fast( ar10xx_t *dev, uint8_t value);
//1,4,8,16,32,64,128
int ar10xx_set_sampling_slow( ar10xx_t *dev, uint8_t value);
//1 - 8
int ar10xx_set_accuracy_filter_fast( ar10xx_t *dev, uint8_t value);
//1 - 8
int ar10xx_set_accuracy_filter_slow( ar10xx_t *dev, uint8_t value);
// 0 - 255
int ar10xx_set_speed_treshold( ar10xx_t *dev, uint8_t value);
// 0 - 255  (* 100 ms )
int ar10xx_set_sleep_delay( ar10xx_t *dev, uint8_t value);
// 0 - 255  (* 240 us )
int ar10xx_set_penup_delay( ar10xx_t *dev, uint8_t value);
//complex register
int ar10xx_set_touch_mode( ar10xx_t *dev, uint8_t value);
//complex register
int ar10xx_set_touch_options( ar10xx_t *dev, uint8_t value);
//0 - 40
int ar10xx_set_calibration_inset( ar10xx_t *dev, uint8_t value);
//0 - 255
int ar10xx_set_pen_state_report_delay( ar10xx_t *dev, uint8_t value);
//0 - 255
int ar10xx_set_touch_report_delay( ar10xx_t *dev, uint8_t value);

//

int ar10xx_get_version( ar10xx_t *dev, ar10xx_id_t* version);
int ar10xx_disable_touch( ar10xx_t *dev);
int ar10xx_enable_touch( ar10xx_t *dev);


//Set default value for touch screen (need reboot after this)
int ar10xx_factory_setting(ar10xx_t *dev);

//save registers to EEPROM
int ar10xx_save_configs(ar10xx_t *dev);

//load EEPROM to registers
int ar10xx_load_configs(ar10xx_t *dev);

//read configs from registers
int ar10xx_read_configs(ar10xx_t *dev);

//write configs to registers
int ar10xx_write_configs(ar10xx_t *dev);

//read touch controller
int ar10xx_read_touch(ar10xx_t *dev, ar10xx_read_t data);

//Read user data (128 byte max)
int ar10xx_eeprom_read(ar10xx_t *dev, uint8_t addr, uint8_t* buf, uint8_t size);

//write user data  (128 byte max)
int ar10xx_eeprom_write(ar10xx_t *dev, uint8_t addr, const uint8_t* data, uint8_t size);

//int ar10xx_need_read(const ar10xx_t *dev); //FIXME: better a generic lvgl_need_read(); ( need ba a irq count)


#ifdef __cplusplus
}
#endif
#endif /* USE_AR10XX*/
#endif //AR10XX_H
