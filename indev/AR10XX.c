/**
 * @file AR10XX.c
 * @author zaltora  (https://github.com/Zaltora)
 * @copyright MIT License.
 */
/*********************
 *      INCLUDES
 *********************/
#include "AR10XX.h"
#if USE_AR10XX != 0

#include "string.h"

/*********************
 *      DEFINES
 *********************/
#ifndef AR10XX_VERIFY_ANSWER
#define AR10XX_VERIFY_ANSWER (1)
#endif

#if (AR10XX_COMPONENT != 10) && (AR10XX_COMPONENT != 11) && (AR10XX_COMPONENT != 20) && (AR10XX_COMPONENT != 21)
#error "AR10XX_COMPONENT: This library don't support this component version"
#endif

//REGISTER
#define AR10XX_SPE_USE1                 (0x00)
#define AR10XX_SPE_USE2                 (0x01)
#define AR10XX_TOUCH_TRESHOLD           (0x02)
#define AR10XX_SENSITIVITY_FILTER       (0x03)
#define AR10XX_SAMPLING_FAST            (0x04)
#define AR10XX_SAMPLING_SLOW            (0x05)
#define AR10XX_ACC_FILTER_FAST          (0x06)
#define AR10XX_ACC_FILTER_SLOW          (0x07)
#define AR10XX_SPEED_TRESHOLD           (0x08)
#define AR10XX_SPE_USE3                 (0x09)
#define AR10XX_SLEEP_DELAY              (0x0A)
#define AR10XX_PENUP_DELAY              (0x0B)
#define AR10XX_TOUCHMODE                (0x0C)
#define AR10XX_TOUCHOPTIONS             (0x0D)
#define AR10XX_CALIB_INSET              (0x0E)
#define AR10XX_PEN_STATE_REPORT_DELAY   (0x0F)
#define AR10XX_SPE_USE4                 (0x10)
#define AR10XX_TOUCH_REPORT_DELAY       (0x11)
#define AR10XX_SPE_USE5                 (0x12)

//COMMAND
#define AR10XX_GET_VERSION               (0x10)
#define AR10XX_ENABLE_TOUCH              (0x12)
#define AR10XX_DISABLE_TOUCH             (0x13)
#define AR10XX_CALIBRATE_MODE            (0x14)
#define AR10XX_REGISTER_READ             (0x20)
#define AR10XX_REGISTER_WRITE            (0x21)
#define AR10XX_REGISTER_START_ADDR_REQ   (0x22)
#define AR10XX_REGISTERS_WRITE_TO_EEPROM (0x23)
#define AR10XX_EEPROM_READ               (0x28)
#define AR10XX_EEPROM_WRITE              (0x29)
#define AR10XX_EEPROM_WRITE_TO_REGISTERS (0x2B)

//OTHER
#define AR10XX_CMD_HEADER                (0x55)
#define AR10XX_I2C_CMD_REG               (0x00)
#define AR10XX_EEPROM_USER_ADDR_START    (0x80)
#define AR10XX_EEPROM_USER_ADDR_END      (0xFF)
#define AR10XX_MAX_TRANSFERT_BYTE        (8)

#define AR10X0_EEPROM_ADDR_FAC1          (0x00)
#define AR10X1_EEPROM_ADDR_FAC1          (0x01)
#define AR10X1_EEPROM_ADDR_FAC2          (0x29)

#define _POS_ARRAY_HEADER                (0)
#define _POS_ARRAY_SIZE                  (1)
#define _POS_ARRAY_ERROR                 (2)
#define _POS_ARRAY_CMD                   (3)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
   uint8_t pen;
   uint16_t x;
   uint16_t y;
} ar10xx_read_t ;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int inline _spi_transaction(const ar10xx_t *dev, uint8_t* data_in, uint8_t* data_out, uint8_t len, uint8_t wordsize);
static int inline _i2c_send(const ar10xx_t *dev, uint8_t reg, const uint8_t* data, uint8_t len);
static int inline _i2c_receive(const ar10xx_t *dev, uint8_t* data, uint8_t len);

static int _sendData(const ar10xx_t *dev, uint8_t* data_out, uint32_t len);
static int _receiveData(const ar10xx_t *dev, uint8_t* data_in, uint32_t len);

static int _read_pos(const ar10xx_t *dev, ar10xx_read_t* pos);
static void _wait_cmd_answer(ar10xx_t *dev);
static int _send_register_setting(ar10xx_t *dev, uint8_t cmd, uint8_t value);

/**********************
 *  STATIC VARIABLES
 **********************/
static ar10xx_t* _device;

/**********************
 *      MACROS
 **********************/
#if AR10XX_DEBUG
#include <stdio.h>
#define debug(fmt, ...) printf("%s: " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#if AR10XX_ERR_CHECK
#define err_control(fn) do { int err; if((err = fn)) return err; } while (0)
#else
#define err_control(fn) (fn)
#endif

//buffer format to write into a register. (See datasheet)
#define reg_bufsize(len) (6 + len)
#define reg_write_format(reg, len, ... ) { AR10XX_CMD_HEADER, 0x04 + len, AR10XX_REGISTER_WRITE, 0x00, reg, len, ## __VA_ARGS__ }

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int ar10xx_init(ar10xx_t *dev)
{
    _device = dev;
    err_control(ar10xx_enable_touch(dev));
    return 0;
}

int ar10xx_factory_setting(ar10xx_t *dev)
{
#if (AR10XX_COMPONENT == 10) || (AR10XX_COMPONENT == 20)
    uint8_t data[7] = { AR10XX_CMD_HEADER, 0x05, AR10XX_EEPROM_WRITE, 0x00, AR10X0_EEPROM_ADDR_FAC1, 0x01, 0xFF };
#else
    uint8_t data[7] = { AR10XX_CMD_HEADER, 0x05, AR10XX_EEPROM_WRITE, 0x00, AR10X1_EEPROM_ADDR_FAC1, 0x01, 0xFF };
#endif
    err_control(_sendData(dev, data, sizeof(data)));
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    if(data[2])
    {
        return data[2];
    }
#endif

#if (AR10XX_COMPONENT == 11) || (AR10XX_COMPONENT == 21)
    data[4] = AR10X1_EEPROM_ADDR_FAC2;
    err_control(_sendData(dev, data, sizeof(data)));
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    if(data[2])
    {
        return data[2];
    }
#endif
#endif
    return 0;
}

int ar10xx_enable_touch( ar10xx_t *dev)
{
    uint8_t data[4] = { AR10XX_CMD_HEADER, 0x01, AR10XX_ENABLE_TOUCH, 0 };
    err_control(_sendData(dev, data, 3));
#if AR10XX_USE_IRQ
    dev->count_irq = 0; //reset irq
#endif
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    if(!data[2])
    {
        dev->opt_enable = 1;
    }
    return data[2];
#else
    dev->opt_enable = 1;
    return 0;
#endif
}

int ar10xx_disable_touch( ar10xx_t *dev)
{
    uint8_t data[4] = { AR10XX_CMD_HEADER, 0x01, AR10XX_DISABLE_TOUCH , 0 };
    err_control(_sendData(dev, data, 3));
#if AR10XX_USE_IRQ
    dev->count_irq = 0; //reset irq
#endif
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    if(!data[2])
    {
        dev->opt_enable = 0;
    }
    return data[2];
#else
    dev->opt_enable = 0;
    return 0;
#endif
}

int ar10xx_get_version( ar10xx_t *dev, ar10xx_id_t* version)
{
    uint8_t data[7] = { AR10XX_CMD_HEADER, 0x01, AR10XX_GET_VERSION };
    err_control(_sendData(dev, data, 3));
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 7));
    memcpy((void*)version, (void*)&data[4], 3);
    return data[2];
}

int ar10xx_save_configs(ar10xx_t *dev)
{
    uint8_t data[4] = { AR10XX_CMD_HEADER, 0x01, AR10XX_REGISTERS_WRITE_TO_EEPROM, 0 };
    err_control(_sendData(dev, data, 3));
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    return data[2];
#else
    return 0;
#endif
}

int ar10xx_load_configs(ar10xx_t *dev)
{
    uint8_t data[4] = { AR10XX_CMD_HEADER, 0x01, AR10XX_EEPROM_WRITE_TO_REGISTERS, 0 };
    err_control(_sendData(dev, data, 3));
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    return data[2];
#else
    return 0;
#endif
}

int ar10xx_eeprom_read(ar10xx_t *dev, uint8_t addr, uint8_t* buf, uint8_t size)
{
    if(addr >= AR10XX_EEPROM_USER_ADDR_START)
    {
        return -1; //addr invalid
    }
    addr += AR10XX_EEPROM_USER_ADDR_START; //physical addr offset
    if(size > (AR10XX_EEPROM_USER_ADDR_END - addr + 1))
    {
        return -1; //size invalid
    }

    uint8_t data_out[6] = { AR10XX_CMD_HEADER, 0x04, AR10XX_EEPROM_READ, 0x00, addr , AR10XX_MAX_TRANSFERT_BYTE };
    uint8_t data_in[13] = { 0  };

    for(uint8_t i= size/AR10XX_MAX_TRANSFERT_BYTE ; i > 0 ; i--)
    {
        err_control(_sendData(dev, data_out, sizeof(data_out)));
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
#if (AR10XX_VERIFY_ANSWER)
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        memcpy((void*)buf, (void*)&data_in[4], AR10XX_MAX_TRANSFERT_BYTE);
        buf += AR10XX_MAX_TRANSFERT_BYTE;
        data_out[4] += AR10XX_MAX_TRANSFERT_BYTE;

    }
    if(size % AR10XX_MAX_TRANSFERT_BYTE)
    {
        data_out[5] = size % AR10XX_MAX_TRANSFERT_BYTE;
        err_control(_sendData(dev, data_out, sizeof(data_out)));
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, size % AR10XX_MAX_TRANSFERT_BYTE));
#if (AR10XX_VERIFY_ANSWER)
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        memcpy((void*)buf, (void*)&data_in[4], size % AR10XX_MAX_TRANSFERT_BYTE);
    }
    return 0;
}

int ar10xx_eeprom_write(ar10xx_t *dev, uint8_t addr, const uint8_t* buf, uint8_t size)
{
    if(addr >= AR10XX_EEPROM_USER_ADDR_START)
    {
        return -1; //addr invalid
    }
    addr += AR10XX_EEPROM_USER_ADDR_START; //physical addr offset
    if(size > (AR10XX_EEPROM_USER_ADDR_END - addr + 1))
    {
        return -1; //size invalid
    }

    uint8_t data_out[13] = { AR10XX_CMD_HEADER, 0x04 + AR10XX_MAX_TRANSFERT_BYTE, AR10XX_EEPROM_WRITE, 0x00, addr , AR10XX_MAX_TRANSFERT_BYTE };
    uint8_t data_in[4] = { 0 };

    for(uint8_t i= size/AR10XX_MAX_TRANSFERT_BYTE ; i > 0 ; i--)
    {
        memcpy((void*)&data_out[6],(void*)buf, AR10XX_MAX_TRANSFERT_BYTE);
        err_control(_sendData(dev, data_out, sizeof(data_out)));
#if (AR10XX_VERIFY_ANSWER)
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        buf += AR10XX_MAX_TRANSFERT_BYTE;
        data_out[4] += AR10XX_MAX_TRANSFERT_BYTE;

    }
    if(size % AR10XX_MAX_TRANSFERT_BYTE)
    {
        data_out[2] = 4 + size % AR10XX_MAX_TRANSFERT_BYTE;
        data_out[5] = size % AR10XX_MAX_TRANSFERT_BYTE;
        memcpy((void*)&data_out[6],(void*)buf, size % AR10XX_MAX_TRANSFERT_BYTE);
        err_control(_sendData(dev, data_out,  size % AR10XX_MAX_TRANSFERT_BYTE));
#if (AR10XX_VERIFY_ANSWER)
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
    }
    return 0;
}



int ar10xx_read_configs(ar10xx_t *dev, ar10xx_regmap_t* regmap)
{
    uint8_t data_out[6] = { AR10XX_CMD_HEADER, 0x04, AR10XX_REGISTER_READ, 0x00, 0x00 , AR10XX_MAX_TRANSFERT_BYTE };
    uint8_t data_in[13] = { 0  };
    uint8_t* ptr = regmap->reg_data;

    for(uint8_t i= sizeof(ar10xx_regmap_t)/AR10XX_MAX_TRANSFERT_BYTE ; i > 0 ; i--)
    {
        err_control(_sendData(dev, data_out, sizeof(data_out)));
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
#if (AR10XX_VERIFY_ANSWER)
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        memcpy((void*)ptr, (void*)&data_in[4], AR10XX_MAX_TRANSFERT_BYTE);
        ptr += AR10XX_MAX_TRANSFERT_BYTE;
        data_out[4] += AR10XX_MAX_TRANSFERT_BYTE;

    }
    if(sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE)
    {
        data_out[5] = sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE;
        err_control(_sendData(dev, data_out, sizeof(data_out)));
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE));
#if (AR10XX_VERIFY_ANSWER)
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        memcpy((void*)ptr, (void*)&data_in[4], sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE);
    }
    return 0;
}

int ar10xx_write_configs(ar10xx_t *dev, const ar10xx_regmap_t* regmap)
{

    uint8_t data_out[13] = { AR10XX_CMD_HEADER, 0x04 + AR10XX_MAX_TRANSFERT_BYTE, AR10XX_REGISTER_WRITE, 0x00, 0x00 , AR10XX_MAX_TRANSFERT_BYTE };
    uint8_t data_in[4] = { 0 };
    const uint8_t* ptr = regmap->reg_data;

    for(uint8_t i= sizeof(ar10xx_regmap_t)/AR10XX_MAX_TRANSFERT_BYTE ; i > 0 ; i--)
    {
        memcpy((void*)&data_out[6],(void*)ptr, AR10XX_MAX_TRANSFERT_BYTE);
        err_control(_sendData(dev, data_out, sizeof(data_out)));
#if (AR10XX_VERIFY_ANSWER)
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
        ptr += AR10XX_MAX_TRANSFERT_BYTE;
        data_out[4] += AR10XX_MAX_TRANSFERT_BYTE;

    }
    if(sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE)
    {
        data_out[2] = 4 + sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE;
        data_out[5] = sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE;
        memcpy((void*)&data_out[6],(void*)ptr, sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE);
        err_control(_sendData(dev, data_out,  sizeof(ar10xx_regmap_t) % AR10XX_MAX_TRANSFERT_BYTE));
#if (AR10XX_VERIFY_ANSWER)
        _wait_cmd_answer(dev);
        err_control(_receiveData(dev, data_in, sizeof(data_in)));
        if(data_in[2])
        {
            return data_in[2];
        }
#endif
    }
    return 0;
}

//Register set functions

int ar10xx_set_touch_treshold(ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev, AR10XX_TOUCH_TRESHOLD,value);
}

int ar10xx_set_sensitivity_filter(ar10xx_t *dev, uint8_t value)
{
    if (value > 10)
    {
        return -1;
    }
    return _send_register_setting(dev, AR10XX_SENSITIVITY_FILTER,value);
}

int ar10xx_set_sampling_fast(ar10xx_t *dev, ar10xx_sampling_t value)
{
    return _send_register_setting(dev, AR10XX_SAMPLING_FAST,value);
}

int ar10xx_set_sampling_slow(ar10xx_t *dev, ar10xx_sampling_t value)
{
    return _send_register_setting(dev, AR10XX_SAMPLING_SLOW,value);
}

int ar10xx_set_accuracy_filter_fast(ar10xx_t *dev, uint8_t value)
{
    if(!value || (value > 8))
    {
        return -1;
    }
    return _send_register_setting(dev, AR10XX_ACC_FILTER_FAST,value);
}

int ar10xx_set_accuracy_filter_slow(ar10xx_t *dev, uint8_t value)
{
    if(!value || (value > 8))
    {
        return -1;
    }
    return _send_register_setting(dev, AR10XX_ACC_FILTER_SLOW,value);
}

int ar10xx_set_speed_treshold(ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev,AR10XX_SPEED_TRESHOLD,value);
}

int ar10xx_set_sleep_delay(ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev, AR10XX_SLEEP_DELAY,value);
}

int ar10xx_set_penup_delay(ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev, AR10XX_PENUP_DELAY,value);
}

int ar10xx_set_touch_mode(ar10xx_t *dev, ar10xx_touchmode_t reg)
{
    return _send_register_setting(dev, AR10XX_TOUCHMODE, reg.value);
}

int ar10xx_set_touch_options(ar10xx_t *dev, ar10xx_touchoption_t reg)
{
    return _send_register_setting(dev, AR10XX_TOUCHMODE, reg.value);
}

int ar10xx_set_calibration_inset( ar10xx_t *dev, uint8_t value)
{
    if(value > 40)
    {
        return -1;
    }
    return _send_register_setting(dev, AR10XX_CALIB_INSET,value);
}

int ar10xx_set_pen_state_report_delay( ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev, AR10XX_PEN_STATE_REPORT_DELAY, value);
}

int ar10xx_set_touch_report_delay(ar10xx_t *dev, uint8_t value)
{
    return _send_register_setting(dev, AR10XX_TOUCH_REPORT_DELAY, value);
}

//Read register
bool ar10xx_input_get(lv_indev_data_t * data)
{
    /* check irq or basic read */
    //ar10xx_t* dev = (ar10xx_t*)data->user_data;
#if (AR10XX_USE_IRQ)
    if (!_device->count_irq) //no irq, return false
        return false;
#endif

    /*Get coordinate*/
    ar10xx_read_t position;
    _read_pos(_device, &position);

    /*Store the collected data*/
    data->point.x = position.x;
    data->point.y = position.y;
    data->state = position.pen ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; //Depend setting

    /* update irq reading */
#if (AR10XX_USE_IRQ)
    _device->count_irq--;
    if(_device->count_irq)
    {
        return true;
    }
#endif
    return false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
#if (AR10XX_SPI_SUPPORT)
static int inline _spi_transaction(const ar10xx_t *dev, uint8_t* data_in, uint8_t* data_out, uint8_t len, uint8_t wordsize)
{
    lv_spi_wr_cs(dev->spi_dev, false);
#if (AR10XX_ERR_CHECK)
    int err = lv_spi_transaction(dev->spi_dev, data_in, data_out, len, wordsize);
#else
    lv_spi_transaction(dev->spi_dev, data_in, data_out, len, wordsize);
#endif
    lv_spi_wr_cs(dev->spi_dev, true);
#if (AR10XX_ERR_CHECK)
    return err;
#else
    return 0;
#endif
}
#endif

#if (AR10XX_I2C_SUPPORT)
static int inline _i2c_send(const ar10xx_t *dev, uint8_t reg, const uint8_t* data, uint8_t len)
{
#if AR10XX_ERR_CHECK
    return lv_i2c_write(dev->i2c_dev, &reg, data, len);
#else
    lv_i2c_write(dev->i2c_dev, &reg, data, len);
    return 0;
#endif
}
static int inline _i2c_receive(const ar10xx_t *dev, uint8_t* data, uint8_t len)
{
#if AR10XX_ERR_CHECK
    return lv_i2c_read(dev->i2c_dev, NULL, data, len);
#else
    lv_i2c_read(dev->i2c_dev, NULL, data, len);
    return 0;
#endif
}
#endif


static int _read_pos(const ar10xx_t *dev, ar10xx_read_t* pos)
{
    err_control(_receiveData(dev, (uint8_t*)pos, 5));
    pos->pen &= 0x01 ;
    return 0;
}

static int _send_register_setting(ar10xx_t *dev, uint8_t cmd, uint8_t value)
{
    if(dev->opt_enable)// touch controller need be disable to change register safety
    {
        return -1;
    }
    uint8_t data[reg_bufsize(sizeof(value))] = reg_write_format(cmd, 1, value);
    err_control(_sendData(dev, data, sizeof(data)));
#if (AR10XX_VERIFY_ANSWER)
    _wait_cmd_answer(dev);
    err_control(_receiveData(dev, data, 4));
    return data[_POS_ARRAY_ERROR];
#else
    return 0;
#endif
}

static void _wait_cmd_answer(ar10xx_t *dev)
{
#if (AR10XX_USE_IRQ)
    uint32_t time = lv_get_ms();
    while((time+AR10XX_CMD_IRQ_DELAY_MAX) > lv_get_ms())
    {
        if(dev->count_irq) break;
    }
    dev->count_irq = 0;
#else
    //FIXME: Need to test it. datasheet don't specify time betwen send and receive cmd result
    lv_delay_us(AR10XX_CMD_ANSWER_WAIT);
#endif
}

static int _sendData(const ar10xx_t *dev, uint8_t* data_out, uint32_t len)
{
    switch(dev->protocol)
    {
#if (AR10XX_I2C_SUPPORT)
    case AR10XX_PROTO_I2C:
        err_control(_i2c_send(dev, AR10XX_I2C_CMD_REG, data_out, len));
        break;
#endif
#if (AR10XX_SPI_SUPPORT)
    case AR10XX_PROTO_SPI:
        err_control(_spi_transaction(dev, NULL, data_out, len, 1));
        break;
#endif
    default:
        return -EPROTONOSUPPORT;
        break;
    }
    return 0;
}

static int _receiveData(const ar10xx_t *dev, uint8_t* data_in, uint32_t len)
{
    switch(dev->protocol)
    {
#if (AR10XX_I2C_SUPPORT)
    case AR10XX_PROTO_I2C:
        err_control(_i2c_receive(dev, data_in, len));
        break;
#endif
#if (AR10XX_SPI_SUPPORT)
    case AR10XX_PROTO_SPI:
        err_control(_spi_transaction(dev, data_in, NULL, len, 1));
        break;
#endif
    default:
        return -EPROTONOSUPPORT;
        break;
    }
    return 0;
}

#endif
