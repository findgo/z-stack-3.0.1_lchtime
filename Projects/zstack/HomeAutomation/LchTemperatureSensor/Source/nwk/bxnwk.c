
#include "bxnwk.h"



extern void ltl_ProcessInApdu(MoIncomingMsgPkt_t *pkt);

void bxNwkInit(void)
{
    bxNWK_LowInit();
}

int bxNwk_LowDataRequest(uint16_t dstaddr,uint8_t *dat, uint16_t len)
{
    afAddrType_t afdstddr;
    endPointDesc_t *epDesc;
    uint8_t options = 0;

    afdstddr.addrMode = (afAddrMode_t)Addr16Bit;
    afdstddr.endPoint = LCHTIMEAPP_ENDPOINT;
    afdstddr.addr.shortAddr = dstaddr;
    epDesc = afFindEndPointDesc( LCHTIMEAPP_ENDPOINT );
    if ( epDesc == NULL )
    {
        return ( ZInvalidParameter ); // EMBEDDED RETURN
    }

//    if(dstaddr == 0x0000)
//        options |= AF_EN_SECURITY;

    return AF_DataRequest( &afdstddr, epDesc, LCHTIMEAPP_CLUSTERID, len, dat, 0, options, AF_DEFAULT_RADIUS );
}

void bxNwk_ProcessMessageMSG( afIncomingMSGPacket_t *pInMsg )
{
    MoIncomingMsgPkt_t pkt;

    if(pInMsg->cmd.DataLength < 6)
        return;

    log_debugln("nwk msg process!");
    // process message
    pkt.apduData = pInMsg->cmd.Data;
    pkt.apduLength = pInMsg->cmd.DataLength;
    pkt.srcaddr = pInMsg->srcAddr.addr.shortAddr;
    pkt.isbroadcast = pInMsg->wasBroadcast;
    
    ltl_ProcessInApdu(&pkt);
}



