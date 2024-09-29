#ifndef __QUICK_CONNECT_H__
#define __QUICK_CONNECT_H__

#define QC_SCAN_TIMEOUT 40000
#define QC_FULL_SCAN_TIMEOUT 500000

typedef enum
{
    QC_MODE_SHORT_DIS_MULTI_SEND = 0, //短距高并发//近距离，连接大量设备，高频上报场景/UCM200 / UCM200T
    QC_MODE_MID_DIS_HIGH_RATE = 1,    //中距高速率//室内整层覆盖，室外次公里级覆盖，高速率传输/UCM200 / UCM200T
    QC_MODE_MID_DIS_MULTI_SEND = 2,   //中距高并发//室内整层覆盖，室外次公里级覆盖，高频上报场景/UCM200 / UCM200T
    QC_MODE_LONG_DIS = 3,             //远距离通信//室内多层楼覆盖，室外公里级覆盖/UCM200T
    QC_MODE_GREAT_DIS = 4,            //超远距离通信//最远通信距离，传输时间较长/UCM200T
    QC_MODE_MAX,
} qc_config_e;

typedef struct
{
    unsigned char symbol_len; //符号长度
    unsigned char dlul_ratio; //上下行配比
    unsigned char group_num;  //
    unsigned char mcs;        //速率级别
    signed char up_pow;       //上行发射功率
    signed char down_pow;     //下行发送功率
} qc_config_t;

int wiota_quick_connect_start(unsigned char freq, qc_config_e mode);

void wiota_quick_connect_stop(void);

#endif