#include "gpio.h"
#include "libNavProc.h"
#include "time.h"
#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>

extern void parse_at_cmd_sys(U08 *atCont, const U08 npara, const S16 atLen, U08 *paralen, U16 *parahead);
extern void split_at_cmd(const U08 *atCont, const S16 atLen, U08 *npara, U08 *paralen, U16 *paraHeadIdx);

#define GNSS_LNA_CONFIG GPIO_PIN_29

#define MAX_AT_PNUM (15)
#define GNSS_AT_LENGTH (50)
#define GPS_CMD_RST_TASSIST "$RST TASSIST\r\n"
#define GPS_CMD_TIMEING "$CFG EN TIMING\r\n"
#define GPS_CMD_FIX "$CFG EN FIX\r\n"
#define GPS_CMD_INTV "$CFG TIMING INTV 100\r\n"
#define GPS_CMD_TIMING_PRECTHER "$CFG TIMING PRECTHRE 1000\r\n"
#define GPS_CMD_STAT "$CFG QRY SYS STAT\r\n"
#define GPS_CMD_TRACKING "$CFG QRY SYS TRACKING\r\n"
#define GPS_CMD_QUERY_EN_FIX "$CFG QRY EN FIX\r\n"
#define GPS_CMD_POS "$CFG QRY TIMIING POS\r\n"
#define GPS_CMD_DEVICE_INFO "$DEVICEINFO\r\n"
#define GPS_CMD_DEVWRITE "$DEVWRITE 0 0 0 1409286144 0.0 0.0 0.0 0.0\r\n"
#define GPS_CMD_MASK_GGA "$CFG MASK GGA\r\n"
#define GPS_CMD_MASK_RMC "$CFG MASK RMC\r\n"
#define GPS_CMD_MASK_GSV "$CFG MASK GSV\r\n"
#define GPS_CMD_MASK_GSA "$CFG MASK GSA\r\n"
#define GPS_CMD_MASK_VTG "$CFG MASK VTG\r\n"
#define GPS_CMD_EN_IDLE "$CFG EN IDLE\r\n"
#define GPS_CMD_MASK_IDLE "$CFG MASK IDLE\r\n"

/**
 * @brief 当前世界时间时间戳结构体
 *
 */
typedef struct
{
    unsigned int time_s;  /**< 秒 */
    unsigned int time_us; /**< 微秒 */
} gps_time_t;

/**
 * @brief 三维坐标位置信息结构体
 *
 */
typedef struct gps_coor_xyz
{
    int pos_x;
    int pos_y;
    int pos_z;
} gps_coor_xyz_t;

/**
 * @brief 经纬度位置信息结构体
 *
 */
typedef struct gps_coor_lla
{
    float longitude; /**< 经度 */
    float latitude;  /**< 纬度 */
    float altitude;  /**< 海拔 */
} gps_coor_lla_t;

static rt_sem_t g_pos_sem = RT_NULL;
static gps_time_t g_cur_time = {0};
static gps_coor_xyz_t g_coor_xyz = {0};
static gps_coor_lla_t g_coor_lla = {0};

/**
 * @brief 发送GPS命令
 *
 * @param data           命令数据
 * @param len            命令长度
 * @return unsigned char 0：成功；1：失败
 */
unsigned char gps_send_cmd(unsigned char *data, unsigned short len)
{
    unsigned char npara, pLen[MAX_AT_PNUM];
    unsigned short pHead[MAX_AT_PNUM];
    unsigned char cmd_data[GNSS_AT_LENGTH] = {0};

    rt_memcpy(cmd_data, data, len);

    if (len < 2 || (len > 2 && cmd_data[len - 1] != '\n'))
    {
        cmd_data[len++] = '\r';
        cmd_data[len++] = '\n';
    }

    if (cmd_data[0] == '$' && len > 5)
    {
        split_at_cmd(cmd_data, len - 2, &npara, pLen, pHead);
        parse_at_cmd_sys(cmd_data, npara, len, pLen, pHead);
        return 0;
    }
    else
        return 1;
}

/**
 * @brief 获取两个GPS DFE计数的差值
 *
 * @param new_dfe       新的DFE计数
 * @param old_dfe       旧的DFE计数
 * @return unsigned int DFE计数差值
 */
static unsigned int get_diff_gps_dfe(unsigned int new_dfe, unsigned int old_dfe)
{
    unsigned int diff_dfe = 0;

    if (new_dfe >= old_dfe)
    {
        diff_dfe = new_dfe - old_dfe;
    }
    else
    {
        diff_dfe = 1638400 - old_dfe + new_dfe;
    }

    return diff_dfe;
}

/**
 * @brief 将单精度浮点转为字符串输出
 *
 * @param fnum      浮点数
 * @param str       字符串输出
 * @param str_len   字符串长度
 * @param precision 精度
 */
void float_to_string(float fnum, char *str, int str_len, int precision)
{
    snprintf(str, str_len, "%.*f", precision, fnum);
}

/**
 * @brief 打印经纬度信息
 *
 * @param longitude 经度
 * @param latitude  纬度
 * @param altitude  海拔
 */
void print_lla_info(float longitude, float latitude, float altitude)
{
    char longitude_str[20] = {0};
    char latitude_str[20] = {0};
    char altitude_str[20] = {0};

    float_to_string(longitude, longitude_str, sizeof(longitude_str), 4);
    float_to_string(latitude, latitude_str, sizeof(latitude_str), 4);
    float_to_string(altitude, altitude_str, sizeof(altitude_str), 4);

    rt_kprintf("cur_pos: longitude %s, latitude %s , altitude %s\n", longitude_str, latitude_str, altitude_str);
}

/**
 * @brief GPS授时回调函数
 *
 * @param time_ast GNSS授时信息
 */
static void gps_pvt_callback(IN const time_ast_t time_ast)
{
    // 当前世界时间
    unsigned int time_sec = 0;
    unsigned int time_usec = 0;
    int fdat = (int)(time_ast.decMs.fDat * 1000);  // us, maybe lost 1us
    int fsec = (int)(time_ast.rtc.fSec * 1000000); // s -> us
    unsigned int gps_delay_us = get_diff_gps_dfe(time_ast.dfeIrcnt, time_ast.dfecnt) * 1000 / 16384;

    time_t time_stamp = 0;
    struct tm tm = {0};

    tm.tm_year = time_ast.rtc.sYear - 1900;
    tm.tm_mon = time_ast.rtc.ucMon - 1;
    tm.tm_mday = time_ast.rtc.ucDay;
    tm.tm_hour = time_ast.rtc.ucHour;
    tm.tm_min = time_ast.rtc.ucMin;
    tm.tm_sec = (int)time_ast.rtc.fSec;
    time_stamp = mktime(&tm);

    // 转化为时间戳
    time_sec = time_stamp;
    time_usec = (fsec - (int)time_ast.rtc.fSec * 1000000) + fdat + gps_delay_us;
    g_cur_time.time_s = time_sec;
    g_cur_time.time_us = time_usec;

    // 当前位置，三维坐标
    g_coor_xyz.pos_x = time_ast.pxyz.tX.fDat + time_ast.pxyz.tX.fErr;
    g_coor_xyz.pos_y = time_ast.pxyz.tY.fDat + time_ast.pxyz.tY.fErr;
    g_coor_xyz.pos_z = time_ast.pxyz.tZ.fDat + time_ast.pxyz.tZ.fErr;

    // 当前位置，经纬度海拔
    g_coor_lla.longitude = time_ast.plla.tLon.fDat + time_ast.plla.tLon.fErr;
    g_coor_lla.latitude = time_ast.plla.tLat.fDat + time_ast.plla.tLat.fErr;
    g_coor_lla.altitude = time_ast.plla.fAlt;

    if (g_pos_sem)
    {
        rt_sem_release(g_pos_sem);
    }
}

/**
 * @brief GPS 二次开发示例，调用该接口的线程的线程栈至少要2K
 *        如果要对GPS进行二次开发，就不能在调用WIoTa提供的同步授时相关的接口，否则会产生冲突
 *
 */
void gps_demo(void)
{
    // 使能 lna
    gpio_set_pin_mux(UC_GPIO_CFG, GNSS_LNA_CONFIG, GPIO_FUNC_0);
    gpio_set_pin_direction(GNSS_LNA_CONFIG, GPIO_DIR_OUT);
    gpio_set_pin_value(GNSS_LNA_CONFIG, GPIO_VALUE_HIGH);

    // 开启GPS，注册授时回调
    GnssStart(gps_pvt_callback, GNSS_LNA_CONFIG, FALSE, RT_NULL);
    rt_thread_mdelay(3000);

    gps_send_cmd((unsigned char *)GPS_CMD_RST_TASSIST, rt_strlen(GPS_CMD_RST_TASSIST));
    rt_thread_mdelay(2000);

    gps_send_cmd((unsigned char *)GPS_CMD_TIMEING, rt_strlen(GPS_CMD_TIMEING));
    rt_thread_mdelay(100);

    gps_send_cmd((unsigned char *)GPS_CMD_FIX, rt_strlen(GPS_CMD_FIX));
    rt_thread_mdelay(100);

    gps_send_cmd((unsigned char *)GPS_CMD_INTV, rt_strlen(GPS_CMD_INTV));
    rt_thread_mdelay(100);

    gps_send_cmd((unsigned char *)GPS_CMD_TIMING_PRECTHER, rt_strlen(GPS_CMD_TIMING_PRECTHER));
    rt_thread_mdelay(100);

    gps_send_cmd((unsigned char *)GPS_CMD_QUERY_EN_FIX, rt_strlen(GPS_CMD_QUERY_EN_FIX));
    rt_thread_mdelay(100);
    gps_send_cmd((unsigned char *)GPS_CMD_MASK_RMC, rt_strlen(GPS_CMD_MASK_RMC));
    rt_thread_mdelay(100);
    gps_send_cmd((unsigned char *)GPS_CMD_MASK_VTG, rt_strlen(GPS_CMD_MASK_VTG));
    rt_thread_mdelay(100);

    // 创建信号量，等待授时和定位结果
    g_pos_sem = rt_sem_create("pos_sem", 0, RT_IPC_FLAG_PRIO);
    RT_ASSERT(g_pos_sem);

    // 2mins应该能等到，保证GPS天线连接正常，并能正常接收信号
    if (RT_EOK != rt_sem_take(g_pos_sem, 2 * 60 * 1000))
    {
        rt_kprintf("wait time&pos timeout\n");
    }
    else
    {
        rt_kprintf("wait time&pos suc\n");
        rt_kprintf("cur_utc: sec %u, usec %u\n", g_cur_time.time_s, g_cur_time.time_us);
        rt_kprintf("cur_pos: x %d, y %d , z %d\n", g_coor_xyz.pos_x, g_coor_xyz.pos_y, g_coor_xyz.pos_z);
        // 将单精度浮点转为字符串输出
        print_lla_info(g_coor_lla.longitude, g_coor_lla.latitude, g_coor_lla.altitude);
    }

    rt_sem_delete(g_pos_sem);
    g_pos_sem = RT_NULL;

    // 关闭GPS
    GnssStop();
}