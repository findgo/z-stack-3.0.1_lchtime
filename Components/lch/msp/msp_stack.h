/*
 * The general format of an MSP message is:
 * <preamble1><preamble2>,<data length><command>,<data>,<CRC/FCS/SUM>
 * 
 * pdu = <command>,<data>
 *
 * Where:
 * preamble     = the ASCII characters '$M'
 * data length  = number of data bytes, binary. Can be zero as in the case of a data request to the master
 * command      = the command you want to send
 * data         = as per the table below. UINT8
 * CRC/FOC/SUM  = FOC/SUM : 1 byte, XOR/SUM of  <command> and each data byte into sum 
                       CRC:
*/


#ifndef __MSP_STACK_H__
#define __MSP_STACK_H__

// 0 : crc 1: fcs 　2: sum
#define configMSP_USE_CHECK    0


#include <stdint.h>

//frame head offset
#define MSP_FRAME_HEAD_PREAMBLE1_OFFSET  0  
#define MSP_FRAME_HEAD_PREAMBLE2_OFFSET  1
// frame data length offset
#define MSP_FRAME_DATALEN_OFFSET     2
// command offset in pdu
#define MSP_PDU_COMMAND_OFFSET      0
// data offset in pdu
#define MSP_PDU_DATA_OFFSET         1

#define MSP_FRAME_HEAD_LEN               2  // frame head len
#define MSP_FRAME_DATALEN_LEN            2  // frame data length len
// PDU
#define MSP_PDU_COMMAND_LEN              1   // pdu command

#define MSP_FRAME_CRC_LEN                2  // CRC
#define MSP_FRAME_FCS_LEN                1  // FCS
#define MSP_FRAME_SUM_LEN                1  // SUM
                                  //ascii
#define MSP_PREAMBLE1       '$'  // 0x24
#define MSP_PREAMBLE2       'M'  // 0x4d


typedef struct {
    uint8_t *data;
    uint16_t data_len;
    uint8_t buf[];
}mspbuf_t;


#endif

