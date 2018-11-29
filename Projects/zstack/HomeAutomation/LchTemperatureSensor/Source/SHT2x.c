/** 
 * @file     SHT2x.c
 * @brief    湿温度传感器
 * @details  
 * @author   xzl
 * @email    
 * @date     
 * @version  
 * @par Copyright (c):  
 *           
 * @par History:          
 *   version: author, date, desc\n 
 *             mo 20181109 抽像调试宏,便于裁减和移植,缩减代码尺寸,优化API接口
 */

#include "SHT2x.h"

// 定义使能调试信息
// 模块调试
#if  ( 1 ) && defined(GLOBAL_DEBUG)
#define SHT2Xlog(format,args...) log_debugln(format,##args) 
#else
#define SHT2Xlog(format,args...)
#endif

// 表明测量的类型 LSB bit1(0:TEMP 1: HUMI)
#define SHT2X_MEASURE_TYPE_MASK  0x0002 
#define SHT2X_MEASURE_TYPE_HUMI  0x0002
#define SHT2X_MEASURE_TYPE_TEMP  0x0000

typedef enum {
    SHT2x_RES_12_14BIT         = 0x00, //RH=12bit, T=14bit
    SHT2x_RES_8_12BIT          = 0x01, //RH= 8bit, T=12bit
    SHT2x_RES_10_13BIT         = 0x80, //RH=10bit, T=13bit
    SHT2x_RES_11_11BIT         = 0x81, //RH=11bit, T=11bit
    SHT2x_RES_MASK             = 0x81  //Mask for res. bits (7,0) in user reg.
} SHT2xResolution;

typedef enum {
    SHT2x_EOB_ON               = 0x40, //end of battery
    SHT2x_EOB_MASK             = 0x40  //Mask for EOB bit(6) in user reg.
} SHT2xEob;

typedef enum {
    SHT2x_HEATER_ON            = 0x04, //heater on
    SHT2x_HEATER_OFF           = 0x00, //heater off
    SHT2x_HEATER_MASK          = 0x04  //Mask for Heater bit(2) in user reg.
} SHT2xHeater;


#define SHT2x_ReadUserReg()        _SHT2x_iicDevReadByte( SHT2x_SLAVE_ADDRESS , USER_REG_R )
#define SHT2x_WriteUserReg(reg)    _SHT2x_iicDevWriteByte( SHT2x_SLAVE_ADDRESS , USER_REG_W, reg )

// 单位ms
static SHT2x_info_t sht2x_info = {.measure_humi_time = 29, .measure_temp_time = 85};

void SHT2x_Init(void)
{
	_SHT2x_iic_init();   
}

static uint8_t SHT2x_calCrc8(uint8_t data[],uint8_t nbrOfBytes)
{
    //P(x) = x^8+ x^5 + x^4 + x^0
	uint8_t crc = 0;
	uint8_t byteCtr;
    
	for(byteCtr = 0;byteCtr <nbrOfBytes; byteCtr++ )
	{
		crc ^=(data[byteCtr]);
		for(uint8_t bit = 0;bit < 8; bit++)
		{
			if(crc & 0x80) 
                crc = (crc << 1) ^ 0x31;
			else 
                crc = ( crc << 1);      
		}
	}
    
	return crc;
}
//正常
void SHT2x_GetSerialNumber(u8 *buf)
{
	u8 temp[8], i = 0;
    
	_SHT2x_iicDevReadReg( SHT2x_SLAVE_ADDRESS ,0xFA0F,temp,4);	
	_SHT2x_iicDevReadReg( SHT2x_SLAVE_ADDRESS ,0xFCC9,temp+4,4);
	
	buf[5] = temp[i++];
	buf[4] = temp[i++];
	buf[3] = temp[i++];
	buf[2] = temp[i++];
	
	buf[1] = temp[i++];
	buf[0] = temp[i++];
	buf[7] = temp[i++];
	buf[6] = temp[i];
}

SHT2x_info_t *SHT2x_GetInfo(void)
{
    return &sht2x_info;
}
//正常
uint8_t SHT2x_CheckDevice(void)
{
    return _SHT2x_iic_CheckDevice(SHT2x_SLAVE_ADDRESS);
}
void SHT2x_SoftReset(void)   
{
    _SHT2x_iicDevWriteCmd(SHT2x_SLAVE_ADDRESS,SOFT_RESET);
}
void SHT2x_SetFeature(uint8_t Resolution, uint8_t isHeatterOn)
{
    uint8_t tmp,reg;

    if(Resolution == SHT2x_Resolution_11_11) {//RH=11bit, T=11bit
        tmp = SHT2x_RES_11_11BIT;
        sht2x_info.measure_humi_time = 15;
        sht2x_info.measure_temp_time = 11;
    }
    else if (Resolution == SHT2x_Resolution_8_12){      //RH= 8bit, T=12bit
        tmp = SHT2x_RES_8_12BIT;
        sht2x_info.measure_humi_time = 4;
        sht2x_info.measure_temp_time = 22;
    }
    else if (Resolution == SHT2x_Resolution_10_13){     //RH=10bit, T=13bit
        tmp = SHT2x_RES_10_13BIT;
        sht2x_info.measure_humi_time = 9; 
        sht2x_info.measure_temp_time = 43;
    }
    else { //RH=12bit, T=14bit 其它采用默认
        tmp = SHT2x_RES_12_14BIT;
        sht2x_info.measure_humi_time = 29; 
        sht2x_info.measure_temp_time = 85;
    }
    sht2x_info.measure_humi_time += 5; // 追加5ms偏差
    sht2x_info.measure_temp_time += 5;
    sht2x_info.isHeatterOn = isHeatterOn;
    sht2x_info.resolution = Resolution;
    
	reg = SHT2x_ReadUserReg();
	//set the RES_MASK status	
	reg = (reg & (~SHT2x_RES_MASK) ) | tmp;
	//set the RES_MASK status	
	reg = reg & (~SHT2x_HEATER_MASK);
    if(isHeatterOn)
        reg |= SHT2x_HEATER_ON;

	SHT2x_WriteUserReg(reg);
}

uint8_t SHT2x_CheckFeature(uint8_t Resolution, uint8_t isHeatterOn)
{
    uint8_t resol,reg,temp;

    if(Resolution == SHT2x_Resolution_11_11) {//RH=11bit, T=11bit
        resol = SHT2x_RES_11_11BIT;
    }
    else if (Resolution == SHT2x_Resolution_8_12){      //RH= 8bit, T=12bit
        resol = SHT2x_RES_8_12BIT;
    }
    else if (Resolution == SHT2x_Resolution_10_13){     //RH=10bit, T=13bit
        resol = SHT2x_RES_10_13BIT;
    }
    else { //RH=12bit, T=14bit 其它采用默认
        resol = SHT2x_RES_12_14BIT;
    }

    reg = SHT2x_ReadUserReg();
    
    SHT2Xlog("Get user reg: 0x%x", reg);
    
	//check the RES_MASK status
	temp = reg & SHT2x_RES_MASK;
	if( temp != resol ) // is you want?
	{
		SHT2Xlog("SHT2x mode is incorrect");
		SHT2x_SetFeature(Resolution, isHeatterOn);
	}
	else
	{
		return Success;
	}

	if( temp == SHT2x_RES_12_14BIT )
	{
		SHT2Xlog("SHT2x_RES_12_14BIT");
	}
	else 	if( temp == SHT2x_RES_8_12BIT )
	{
		SHT2Xlog("SHT2x_RES_8_12BIT");	
	}
	else if( temp == SHT2x_RES_10_13BIT )
	{
		SHT2Xlog("SHT2x_RES_10_13BIT");	
	}
	else 	if( temp == SHT2x_RES_11_11BIT )
	{
		SHT2Xlog("SHT2x_RES_11_11BIT");	
	}
	
	//check the battery status
	if(( reg & SHT2x_EOB_MASK ) == SHT2x_EOB_ON )
	{
		SHT2Xlog("Battery <2.25V");
	}else
	{
		SHT2Xlog("Battery >2.25V");
	}
	
	//check the HEATER status
	if(( reg & SHT2x_HEATER_MASK ) == SHT2x_HEATER_ON )
	{
		SHT2Xlog("SHT2x_HEATER_ON");
	}else
	{
        SHT2Xlog("SHT2x_HEATER_OFF");
	}
    
	return Failed;
}

void SHT2x_MeasureStart(uint8_t cmd)
{
    _SHT2x_iicDevWriteCmd(SHT2x_SLAVE_ADDRESS, cmd); 
    SHT2Xlog("SHT2x measure start!");
}
// 解析前一个序列的值
uint8_t SHT2x_MeasureDeal(void)
{
	u8  tmp[3];
	u16 tmpval;	
	uint8_t crc;
    
    SHT2Xlog("SHT2x measure deal!");
    if((_SHT2x_iicDevReadMeasure(SHT2x_SLAVE_ADDRESS, sizeof(tmp), tmp) == Failed)
        || ( (crc = SHT2x_calCrc8(tmp,2)) != tmp[2])){
        SHT2Xlog("read failed crc rcv: %x - cal:%x", tmp[2], crc);
    
        return Failed;
    }

	tmpval = (tmp[0] << 8) | tmp[1];
    if((tmpval & SHT2X_MEASURE_TYPE_MASK) == SHT2X_MEASURE_TYPE_HUMI){
        // 湿度
        tmpval &= ~0x0003;
        #if 1
        sht2x_info.HUMI = -6.0 + 125.0/65536 * (float)tmpval;
        #else
        sht2x_info.HUMI_HM = ((float)tmpval * 0.00190735) - 6;
        #endif
    }
    else{
     //温度    
        tmpval &= ~0x0003;
        #if 1
        sht2x_info.TEMP = -46.85 + 175.72/65536 *(float)tmpval;
        #else
        sht2x_info.TEMP_HM = ((float)value * 0.00268127) - 46.85;
        #endif
    }
    SHT2Xlog("read success!");
    
    return SUCCESS;
}


uint8_t SHT2x_Measure(uint8_t cmd)
{
	u8  tmp[3];
	u16 tmpval;	
	uint8_t crc;
    
    SHT2Xlog("measure sync start!");
    if((_SHT2x_iicDevReadbyPoll(SHT2x_SLAVE_ADDRESS,cmd,sizeof(tmp),tmp) == Failed)
        || ( (crc = SHT2x_calCrc8(tmp,2)) != tmp[2])){
        SHT2Xlog("read failed crc rcv: %x - cal:%x", tmp[2], crc);
    
        return Failed;
    }
        
	tmpval = (tmp[0] << 8) | tmp[1];
    if((tmpval & SHT2X_MEASURE_TYPE_MASK) == SHT2X_MEASURE_TYPE_HUMI){
    // 湿度
        tmpval &= ~0x0003;
        #if 1
        sht2x_info.HUMI = -6.0 + 125.0/65536 * (float)tmpval;
        #else
        sht2x_info.HUMI = ((float)tmpval * 0.00190735) - 6;
        #endif
    }
    else{
     //温度         
        tmpval &= ~0x0003;
        #if 1
        sht2x_info.TEMP = -46.85 + 175.72/65536 *(float)tmpval;
        #else
        sht2x_info.TEMP = ((float)value * 0.00268127) - 46.85;
        #endif
    }
    SHT2Xlog("read sync success!");
    
    return Success;
}

