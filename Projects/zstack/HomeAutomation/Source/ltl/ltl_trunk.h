

#ifndef __LTL_TRUNK_H__
#define __LTL_TRUNK_H__

#define LTL_DEVICE_COMMON_NODENO    0x00

// basic trunks 基本集
#define LTL_TRUNK_ID_GENERAL_BASIC          0x0000
#define LTL_TRUNK_ID_GENERAL_POWER          0x0001
#define LTL_TRUNK_ID_GENERAL_ONOFF          0x0002
#define LTL_TRUNK_ID_GENERAL_LEVEL_CONTROL  0x0003

// Measurement and Sensing trunks  测量与传感器集
#define LTL_TRUNK_ID_MS_ILLUMINANCE_MEASUREMENT            0x0200   // 光照测量
#define LTL_TRUNK_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG   0x0201   // 光照水平感知配置
#define LTL_TRUNK_ID_MS_TEMPERATURE_MEASUREMENT            0x0202   // 温度测量
#define LTL_TRUNK_ID_MS_PRESSURE_MEASUREMENT               0x0203   // 压力测量
#define LTL_TRUNK_ID_MS_FLOW_MEASUREMENT                   0x0204   // 流量测量
#define LTL_TRUNK_ID_MS_RELATIVE_HUMIDITY                  0x0205   // 相对湿度测量
#define LTL_TRUNK_ID_MS_OCCUPANCY_SENSING                  0x0206   // 占有率测量

#endif
