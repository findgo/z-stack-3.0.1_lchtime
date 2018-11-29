#ifndef __BX_NWK_H__
#define __BX_NWK_H__

#include "prefix.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "memalloc.h"
#include "AF.h"
#include "log.h"
#include "ltlApp.h"
#include "ZComDef.h"

#define NWK_FC_DATA     0x00
#define NWK_FC_CMD      0x01
#define NWK_FC_PROTOCOL 0x02

// 定义网络层底层
#define bxNWK_LowInit() //ZbInit()

uint8_t bxNwkHdrLen(void);
uint8_t *bxNwkBuildHdr(uint8_t *pDat,uint8_t fc, uint16_t dstaddr, uint8_t seq);
int bxNwk_LowDataRequest(uint16_t dstaddr,uint8_t *dat, uint16_t len);
int bxNwkDatareq(uint16_t dstaddr, uint8_t *dat, uint16_t len);

void bxNwkInit(void);
void bxNwk_ProcessMessageMSG( afIncomingMSGPacket_t *pInMsg );

#endif

