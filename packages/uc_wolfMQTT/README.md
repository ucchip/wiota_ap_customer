# uc_wolfMQTT



## 1、介绍

uc_wolfMQTT 是应用于嵌入式系统、使用C语言编写的MQTT客户端，依赖POSIX规范的BSD socket接口和pthread接口，支持v5.0版本的mqtt协议，支持SSL/TLS功能(需要wolfSSL库)。

### 1.1 特性

- 接口简单易用

### 1.2 目录结构

| 名称     | 说明            |
| -------- | ------------- |
| examples | 例子目录        |
| wolfmqtt | 头文件目录      |
| port     | 网络接口代码目录 |
| src      | 源代码目录      |

### 1.3 依赖

- RT-Thread 4.0+

- 网络 组件，需要开启 socket abstraction layer功能

    ```
    RT-Thread Components  --->
        Network  --->
            Socket abstraction layer  --->
            [*]   Enable socket abstraction layer
            [*]   Enable BSD socket operated by file system API
    ```



## 2、使用 uc_wolfMQTT 软件包

### 2.1 配置选项

目前有如下几个配置项：

| 配置项                                           | 数据类型 | 说明                                       |
| ----------------------------------------------- | ------- | ----------------------------------------- |
| Enable TLS Support                              | bool    | 是否使用TLS功能(依赖wolfSSL组件)             |
| Enable MQTT-SN(Sensor Network) Support          | bool    | 是否使用传感器网络(Sensor Network)           |
| Enable MQTT v5.0 support                        | bool    | 是否使用v5.0版本MQTT协议                     |
| Enable property callback support                | bool    | 是否使用特性回调(仅支持v5.0版本)              |
| Enable Non-blocking support                     | bool    | 是否使用非阻塞数据收发                        |
| Disable socket timeout code                     | bool    | 是否禁用socket超时处理代码                   |
| Enable Disconnect callback support              | bool    | 是否使用连接断开事件回调                      |
| Enable Multi-threading                          | bool    | 是否使用多线程任务                           |
| Enable Debugging                                | bool    | 是否使用调试信息输出                         |
| Enable Debugging client                         | bool    | 是否使用客户端模块调试信息输出                 |
| Enable Debugging socket                         | bool    | 是否使用socket模块调试信息输出                |
| Enable Debugging thread                         | bool    | 是否使用thread模块调试信息输出                |
| Disable error strings                           | bool    | 是否禁用错误信息字符串                        |



## 3、API 说明

### 3.1 网络层接口

```c
int MqttClientNet_Init(MqttNet* net, MQTTCtx* mqttCtx);         /* 网络层接口初始化 */
int MqttClientNet_DeInit(MqttNet* net);                         /* 网络层接口去初始化 */
```

### 3.2 mqtt协议client接口

```c
int MqttClient_Init(
    MqttClient *client,
    MqttNet *net,
    MqttMsgCb msg_cb,
    byte *tx_buf, int tx_buf_len,
    byte *rx_buf, int rx_buf_len,
    int cmd_timeout_ms);                                         /* MqttClient结构初始化 */
void MqttClient_DeInit(MqttClient *client);                      /* MqttClient结构去初始化 */
int MqttClient_SetDisconnectCallback(
    MqttClient *client,
    MqttDisconnectCb discb,
    void* ctx);                                                  /* 设置连接断开事件回调函数 */
int MqttClient_SetPropertyCallback(
    MqttClient *client,
    MqttPropertyCb propCb,
    void* ctx);                                                  /* 设置特性信息回调函数 */
int MqttClient_Connect(
    MqttClient *client,
    MqttConnect *connect);                                       /* 连接MQTT服务器(协议层) */
int MqttClient_Publish(
    MqttClient *client,
    MqttPublish *publish);                                       /* 发布主题消息 */
int MqttClient_Publish_ex(
    MqttClient *client,
    MqttPublish *publish,
    MqttPublishCb pubCb);                                        /* 发布主题消息(带回调接口) */
int MqttClient_Subscribe(
    MqttClient *client,
    MqttSubscribe *subscribe);                                   /* 订阅主题消息 */
int MqttClient_Unsubscribe(
    MqttClient *client,
    MqttUnsubscribe *unsubscribe);                               /* 取消主题消息订阅 */
int MqttClient_Ping(
    MqttClient *client);                                         /* 发送ping消息 */
int MqttClient_Ping_ex(MqttClient *client, MqttPing* ping);      /* 发送ping消息(带状态) */
int MqttClient_Auth(
    MqttClient *client,
	MqttAuth *auth);                                             /* 发送认证请求 */
MqttProp* MqttClient_PropsAdd(
    MqttProp **head);                                            /* 添加特性 */
int MqttClient_PropsFree(
    MqttProp *head);                                             /* 删除特性 */
int MqttClient_Disconnect(
    MqttClient *client);                                         /* 断开与MQTT服务器连接(协议层) */
int MqttClient_Disconnect_ex(
    MqttClient *client,
    MqttDisconnect *disconnect);                                 /* 断开与MQTT服务器连接(协议层)(带状态、特性) */
int MqttClient_WaitMessage(
    MqttClient *client,
    int timeout_ms);                                             /* 等待消息 */
int MqttClient_WaitMessage_ex(
    MqttClient *client,
    MqttObject* msg,
    int timeout_ms);                                             /* 等待消息(获取消息内容) */
int MqttClient_NetConnect(
    MqttClient *client,
    const char *host,
    word16 port,
    int timeout_ms,
    int use_tls,
    MqttTlsCb cb);                                               /* 设置iote断开连接通知回调函数 */
int MqttClient_NetConnect(
    MqttClient *client,
    const char *host,
    word16 port,
    int timeout_ms,
    int use_tls,
    MqttTlsCb cb);                                               /* 连接MQTT服务器(网络层) */
int MqttClient_NetDisconnect(
    MqttClient *client);                                         /* 断开与MQTT服务器连接(网络层) */
int MqttClient_GetProtocolVersion(MqttClient *client);           /* 获取协议版本(数值) */
const char* MqttClient_GetProtocolVersionString(MqttClient *client);      /* 获取协议版本(字符串) */
const char* MqttClient_ReturnCodeToString(
    int return_code);                                            /* 返回值转换为字符串 */
```


使用方法请参考示例 examples/wolfmqtt_example.c，更多接口说明请查看[wolfMQTT使用手册](https://www.wolfssl.com/docs/wolfmqtt-manual/)。



## 4、联系方式

- 维护：shengda.ma@ucchip.com

