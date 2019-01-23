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

// 定义网络层底层
#define bxNWK_LowInit() //ZbInit()

int bxNwk_LowDataRequest(uint16_t dstaddr,uint8_t *dat, uint16_t len);

void bxNwkInit(void);
void bxNwk_ProcessMessageMSG( afIncomingMSGPacket_t *pInMsg );

#endif

