/* 软件模拟IIC 模块 */
#ifndef __XIIC__H_
#define __XIIC__H_

#include "hal_types.h"
#include "hal_defs.h"
#include "hal_board_cfg.h"
#include "OnBoard.h"

#define xIIC_SDA_OUTPUT()       MCU_IO_DIR_OUTPUT(1, 7)//通用推挽输出50MZ 
#define xIIC_SDA_INPUT()        MCU_IO_DIR_INPUT(1, 7) //输入模式上拉下拉输入模式

#define xIIC_SDA_HIGH()     MCU_IO_SET_HIGH(1, 7)
#define xIIC_SDA_LOW()      MCU_IO_SET_LOW(1, 7)
#define xIIC_SCL_HIGH()     MCU_IO_SET_HIGH(1, 6)
#define xIIC_SCL_LOW()      MCU_IO_SET_LOW(1, 6)

#define xIIC_SDA_GET()  MCU_IO_GET(1, 7)
#define xIIC_SCL_GET()  MCU_IO_GET(1, 6)

// 定义scl sda电平保持时间
#define xIIC_DELAY_WIDE()   Onboard_wait(1000)

/* 用于读内部寄存器使用,比如命令寄存器*/
#define xIICDEV_NO_MEM_ADDR     0xff  // 0xff一般是个空指令

void    xIIC_init(void);
/* 设备是否在线,  */
uint8_t xIICheckDevice(uint8_t devddress);

uint8_t xIICWriteByte(u8 devaddr, u8 memAddress, u8 data);
uint8_t xIICWriteMultiBytes(u8 devaddr,u8 memAddress,u8 len,u8 *wbuf);

uint8_t xIICReadByte(u8 devaddr,u8 memAddress);
uint8_t xIICReadMultiBytes(u8 devaddr,u8 memAddress,u8 len,u8 *wbuf);

// 写命令寄存器
#define xIICWriteReg(devaddr, data) xIICWriteByte(devaddr, xIICDEV_NO_MEM_ADDR, data)
// 读命令寄存器
#define xIICReadReg(devaddr) xIICReadByte(devaddr, xIICDEV_NO_MEM_ADDR)

/******************************非标准IIC Only for SHT2x*****************************************************/
/* 非主机模式 读取成功返回success*/ 
uint8_t SHT_DevReadbyPoll(u8 devaddr,u8 addr,u8 len,u8 *rbuf);
uint8_t SHT_DevReadMeasure(u8 devaddr,u8 len,u8 *wbuf);
void SHT_DevReadMultiReg(u8 devaddr,uint16_t _usRegAddr, uint8_t *_pRegBuf, uint8_t _ucLen);
void SHT_DevWriteMultiReg(u8 devaddr,uint16_t _usRegAddr, uint8_t *_pRegBuf, uint8_t _ucLen);
#endif
