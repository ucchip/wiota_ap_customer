#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "at.h"
#include "ati_prs.h"
#include "drivers/pin.h"
#include "uc_wiota_api.h"

#ifdef UC8088_FACTORY_MODE
enum factory_uart_write_read_type
{
    FACTORY_UART_WRITE = 0,
    FACTORY_UART_READ,
};

enum factory_command_type
{
    FACTORY_WIOTA = 0,
    FACTORY_GPIO = 1,
    FACTORY_I2C = 2,
    FACTORY_AD = 3,
    FACTORY_DA = 4,
    FACTORY_UART1 = 5,
    FACTORY_PWM = 6,
};

#define DAC_DEV_NAME "dac"
#define ADC_DEV_NAME "adc"
#define AHT10_I2C_BUS_NAME "hw_i2c"
#define UART1_DEV_NMAE "uart1"
#define PWM_DEV_NAME "pwm2"
#define AT24C02_ADDR 0xA0

#ifdef AT_USING_SERVER

static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_uint8_t buf[8];
    struct rt_i2c_msg msgs;
    rt_uint32_t buf_size = 1;

    buf[0] = reg; //cmd
    if (data != RT_NULL)
    {
        buf[1] = data[0];
        buf[2] = data[1];
        buf[3] = data[2];
        buf[4] = data[3];
        buf_size = 5;
    }

    msgs.addr = AT24C02_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = buf_size;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AT24C02_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static int at_factory_test_i2c(void)
{
    rt_device_t dev;
    unsigned char set_data[4] = {1, 2, 3, 4};
    unsigned char get_data[4] = {0};
    int num = 0;

    dev = rt_device_find(AHT10_I2C_BUS_NAME);
    if (RT_NULL == dev)
    {
        at_server_printfln("rt_device_find i2c fail\n");
        return 1;
    }

    if (RT_EOK != write_reg((struct rt_i2c_bus_device *)dev, 0, set_data))
    {
        at_server_printfln("write_reg i2c fail\n");
        return 2;
    }

    if (RT_EOK != read_regs((struct rt_i2c_bus_device *)dev, 4, get_data))
    {
        at_server_printfln("read_regs i2c fail\n");
        return 3;
    }

    for (num = 0; num < 4; num++)
    {
        if (set_data[num] != get_data[num])
        {
            at_server_printfln("i2c data match fail. num=%d, %d!= %d\n", num, set_data[num], get_data[num]);
            return 4;
        }
    }

    return 0;
}

static int at_factory_test_ad(unsigned int channel)
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value;

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (RT_NULL == adc_dev)
    {
        at_server_printfln("ad find %s  fail\n", ADC_DEV_NAME);
        return -1;
    }

    rt_adc_enable(adc_dev, channel);
    value = rt_adc_read(adc_dev, channel);
    rt_adc_disable(adc_dev, channel);

    return value;
}

static int at_factory_test_da(unsigned int channel, unsigned int value)
{
    rt_dac_device_t dac_dev;

    dac_dev = (rt_dac_device_t)rt_device_find(DAC_DEV_NAME);
    if (RT_NULL == dac_dev)
    {
        rt_kprintf("da find fail\n");
        return -1;
    }

    rt_dac_enable(dac_dev, channel);

    rt_dac_write(dac_dev, channel, value);

    //rt_dac_disable(dac_dev, channel);

    return 0;
}

static int at_factory_test_uart1(int cmd)
{
    static rt_device_t serial;
    unsigned char send_data[4] = {"1234"};
    unsigned char recv_data[4] = {0};

    serial = rt_device_find(UART1_DEV_NMAE);
    if (!serial)
    {
        rt_kprintf("uart1 find fail\n");
        return 1;
    }

    if (RT_EOK != rt_device_open(serial, RT_DEVICE_OFLAG_RDWR))
    {
        rt_kprintf("uart open fail\n");
        return 2;
    }

    rt_device_write(serial, 0, send_data, sizeof(send_data) / sizeof(unsigned char));

    if (rt_device_read(serial, 0, recv_data, sizeof(recv_data) / sizeof(unsigned char)) < 1)
    {
        rt_kprintf("uart read fail\n");
        return 3;
    }
    at_server_printfln("uart %s\n", recv_data);
    rt_device_close(serial);

    return rt_strcmp((const char *)send_data, (const char *)recv_data);
}

static int at_factory_test_pwm(unsigned int period, unsigned int pulse)
{
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (RT_NULL == pwm_dev)
    {
        rt_kprintf("find %s failed!\n", PWM_DEV_NAME);
        return 1;
    }

    rt_pwm_enable(pwm_dev, 1);
    rt_pwm_set(pwm_dev, 1, period, pulse);
    // rt_pwm_disable(pwm_dev, channel);

    return 0;
}
#endif

#ifdef AT_USING_SERVER
static at_result_t at_factory_setup(const char *args)
{
    int type = 0, data = 0, data1 = 0;
    const char *argc = args;

    args = parse((char *)(++args), "d,d,d", &type, &data, &data1);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (type == FACTORY_WIOTA && data == 6)
    {
        argc = parse((char *)(++argc), "d,d,y", &type, &data, &data1);
        if (!argc)
        {
            return AT_RESULT_PARSE_FAILE;
        }
    }

    rt_kprintf("type %d, data %d, data1 %d\n", type, data, data1);
    switch (type)
    {
    case FACTORY_WIOTA:
    {
        if (0 != ap_pgw_handle_factory_msg(data, data1))
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    case FACTORY_GPIO:
    {
        rt_base_t pin = data;
        rt_base_t value = data1 & 0x1;

        rt_pin_mode(pin, 0);
        rt_pin_write(pin, value);
        break;
    }
    case FACTORY_I2C:
    {
        if (at_factory_test_i2c())
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    case FACTORY_AD:
    {
        unsigned int ch = data;

        int result = at_factory_test_ad(ch);
        if (result < 0)
        {
            return AT_RESULT_FAILE;
        }
        at_server_printfln("+FACTORY=%d,%d", type, result);
        break;
    }
    case FACTORY_DA:
    {
        unsigned int ch = data;
        unsigned int val = data1;

        if (at_factory_test_da(ch, val) < 0)
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    case FACTORY_UART1:
    {
        int cmd = data;
        if (0 != at_factory_test_uart1(cmd))
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    case FACTORY_PWM:
    {
        unsigned int period = data;
        unsigned int pulse = data1;

        if (at_factory_test_pwm(period, pulse))
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    default:
        return AT_RESULT_CMD_ERR;
    }
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT+FACTORY", "=<type>,<data>,<data1>", RT_NULL, RT_NULL, at_factory_setup, RT_NULL);
#endif // #ifdef AT_USING_SERVER

#endif