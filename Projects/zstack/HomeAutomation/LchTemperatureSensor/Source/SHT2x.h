#ifndef __SHT2x_H__
#define __SHT2x_H__
 
 
#ifdef __cplusplus
  extern "C" {
#endif

 //fun location
#include "xiic.h"
#include "log.h"

#define _SHT2x_iic_init 						xIIC_init
#define _SHT2x_iic_CheckDevice 					xIICheckDevice
#define _SHT2x_iicDevWriteByte 					xIICWriteByte
#define _SHT2x_iicDevWriteMultiByte 			xIICWriteMultiBytes
#define _SHT2x_iicDevReadByte 					xIICReadByte
#define _SHT2x_iicDevReadMultiByte 				xIICReadMultiBytes
#define _SHT2x_iicDevReadMeasure                SHT_DevReadMeasure
#define _SHT2x_iicDevReadbyPoll 		        SHT_DevReadbyPoll
#define _SHT2x_iicDevReadReg 					SHT_DevReadMultiReg
#define _SHT2x_iicDevWriteCmd 					xIICWriteReg

 
#define SHT2x_SLAVE_ADDRESS    0x80		/* I2C从机地址 */
 
 typedef enum {
     TRIG_TEMP_MEASUREMENT_HM   = 0xE3, //command trig. temp meas. hold master
     TRIG_HUMI_MEASUREMENT_HM   = 0xE5, //command trig. humidity meas. hold master
     TRIG_TEMP_MEASUREMENT_POLL = 0xF3, //command trig. temp meas. no hold master
     TRIG_HUMI_MEASUREMENT_POLL = 0xF5, //command trig. humidity meas. no hold master
     USER_REG_W                 = 0xE6, //command writing user register
     USER_REG_R                 = 0xE7, //command reading user register
     SOFT_RESET                 = 0xFE  //command soft reset
 } SHT2xCommand;

// 用于feature设置
enum {
    SHT2x_Resolution_12_14 = 0, //RH=12bit, T=14bit
    SHT2x_Resolution_8_12,      //RH= 8bit, T=12bit
    SHT2x_Resolution_10_13,     //RH=10bit, T=13bit
    SHT2x_Resolution_11_11,     //RH=11bit, T=11bit
};
 
typedef struct {
    uint8_t isHeatterOn; // 是否开启加热
    uint8_t resolution;  // 精度
    uint8_t measure_humi_time; // 特定精度下humi测量花费时间 单位ms,已加上可能偏差
    uint8_t measure_temp_time;  // 特定精度下temp测量花费时间 单位ms,已加上可能偏差
    float TEMP; // 存储读到的温度
    float HUMI; // 存储读到的湿度
}SHT2x_info_t;

 
void SHT2x_Init(void); 
uint8_t SHT2x_CheckDevice(void);
SHT2x_info_t *SHT2x_GetInfo(void);
void SHT2x_GetSerialNumber(u8 *buf);
void SHT2x_SoftReset(void);
/*Resolution : RH,TMP的精度, 查看上面枚举 , isHeatterOn: TRUE,启动加热*/
void SHT2x_SetFeature(uint8_t Resolution, uint8_t isHeatterOn);
/* 检测功能是否是设置的,一样:Success, Failed */
uint8_t SHT2x_CheckFeature(uint8_t Resolution, uint8_t isHeatterOn);

/* 非主机同步测量,这个会耗费等待时间,根据不同的精度和测量类型决定 */
/*成功: Success, 失败: failed*/
uint8_t SHT2x_Measure(uint8_t cmd);
/* 非主机序列测量, 测量耗费最大时间可以查看info里的time,需要用户自己处理 */
// 启动一个测量序列
void SHT2x_MeasureStart(uint8_t cmd);
/* 处理前面启动测量序列 */
/*成功: Success, 失败: failed*/
uint8_t SHT2x_MeasureDeal(void);
 
#ifdef __cplusplus
 }
#endif
 
#endif
