#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "at.h"
//#include "pin.h"
#include "drivers/pin.h"

enum factory_command_type
{
    FACTORY_WIOTA = 0, 
    FACTORY_GPIO, 
    FACTORY_I2C,
    FACTORY_AD,
    FACTORY_DA,
    FACTORY_UART1,
    FACTORY_PWM, 
    FACTORY_CAN,
};

static at_result_t at_factory_setup(const char* args)
{
    int type = 0, data = 0, data1 = 0;
    const char *req_expr = "=%d,%d,%d";

    if (1 != at_req_parse_args(args, req_expr, &type, &data, &data1) )
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch(type)
    {
        case FACTORY_WIOTA:
        {
            break;
        }
        case FACTORY_GPIO:
        {
            rt_base_t pin = data;
            rt_base_t value = data1 & 0x1;
            
            if (pin > 100)
            {
                int pin_num = 0;
                for(pin_num = 0; pin_num < 17; pin_num++)
                {
                    rt_pin_mode( (rt_base_t) pin_num, 0);
                    rt_pin_write( (rt_base_t) pin_num, value);
                }
            }
            else
            {
                rt_pin_mode( pin, 0);
                rt_pin_write( pin, value);   
            }
            break;
        }
        case FACTORY_I2C:
        {
            break;
        }
    }
    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+FACTORY", "=<type>,<data>,<data1>", RT_NULL, RT_NULL, at_factory_setup, RT_NULL);


