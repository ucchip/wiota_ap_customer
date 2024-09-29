#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_adda.h"
#include "uc_uart.h"
#ifdef RT_USING_AT
#include "at.h"
#endif
#include "quick_connect.h"

extern void uc_wiota_register_callback(void);

//symbol_length：帧配置，取值0,1,2,3代表128,256,512,1024
//dlul_ratio：帧配置，取值0,1代表1:1和1:2
//帧配置，取值0,1,2,3代表1,2,4,8个上行group数量，在symbol_length为0/1/2/3时，group_number最高限制为3/2/1/0
static qc_config_t g_qc_cfg[QC_MODE_MAX] = {
    {0, 1, 1, UC_MCS_LEVEL_2, (22 + 20), (24 + 20)},
    {1, 0, 0, UC_MCS_LEVEL_3, (22 + 20), (24 + 20)},
    {1, 1, 1, UC_MCS_LEVEL_2, (22 + 20), (24 + 20)},
    {2, 0, 0, UC_MCS_LEVEL_0, (22 + 20), (24 + 20)},
    {3, 0, 0, UC_MCS_LEVEL_0, (22 + 20), (24 + 20)},

};

int wiota_quick_connect_start(unsigned char freq, qc_config_e mode)
{
    sub_system_config_t config;

    if (mode >= QC_MODE_MAX || freq > 200)
    {
        return 1;
    }

    uc_wiota_exit();
    uc_wiota_init();

    //配置参数
    uc_wiota_get_system_config(&config);

    config.freq_idx = freq;
    config.symbol_length = g_qc_cfg[mode].symbol_len;
    config.dlul_ratio = g_qc_cfg[mode].dlul_ratio;
    config.group_number = g_qc_cfg[mode].group_num;
    config.ap_tx_power = g_qc_cfg[mode].down_pow - 20;

    if (0 != uc_wiota_set_system_config(&config))
    {
        uc_wiota_exit();
        return 2;
    }

    uc_wiota_save_static_info(0);

    if (0 != uc_wiota_set_freq_info(freq))
    {
        uc_wiota_exit();
        return 3;
    }

    uc_wiota_run();

#ifdef RT_USING_AT
    uc_wiota_register_callback();
#endif

    if (0 != uc_wiota_set_data_rate(UC_RATE_NORMAL, g_qc_cfg[mode].mcs))
    {
        uc_wiota_exit();
        return 4;
    }

    return 0;
}

void wiota_quick_connect_stop(void)
{
    uc_wiota_exit();
}
