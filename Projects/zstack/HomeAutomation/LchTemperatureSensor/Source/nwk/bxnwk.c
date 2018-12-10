
#include "bxnwk.h"

/* 网络层帧格式 发送模式
 * | byte|      2  |    2   |  1    |   1   | variable |
 * |     | dstaddr | dstaddr| fcctl |   seq | payload  |
*/
/* 网络层帧格式 发送模式
 
 * | byte|    2    |    2   |  1    |   1   | variable |
 * |     | srcaddr | dstaddr| fcctl |   seq | payload  |
 */
typedef struct {
    uint8_t fc;   // nwk 控制帧
    uint8_t seq;  // 序列号
    uint16_t dstaddr; // 目的地址
    uint16_t srcaddr; // 源地址
}nwkHdr_t;

extern void ltl_ProcessInApdu(MoIncomingMsgPkt_t *pkt);

static uint8_t *bxNwkParseHdr(nwkHdr_t *hdr, afIncomingMSGPacket_t *pInMsg);
static void bxNwk_ProcessInNpdu(nwkHdr_t *phdr, MoIncomingMsgPkt_t *pkt);


void bxNwkInit(void)
{
    bxNWK_LowInit();
}


uint8_t bxNwkHdrLen(void)
{
    return ( 2 + 1 + 1 );// dst addr + fc + seq
}
// get past hdr pointer
uint8_t *bxNwkBuildHdr(uint8_t *pDat,uint8_t fc, uint16_t dstaddr, uint8_t seq)
{
    // real data to user
    *pDat++ = LO_UINT16(dstaddr); // NWK层需求
    *pDat++ = HI_UINT16(dstaddr);
    *pDat++ = fc;
    *pDat++ = seq;
    
    return pDat;
}

// get past hdr pointer
static uint8_t *bxNwkParseHdr(nwkHdr_t *hdr, afIncomingMSGPacket_t *pInMsg)
{
    uint8_t *pDat = pInMsg->cmd.Data;

    hdr->srcaddr = pInMsg->srcAddr.addr.shortAddr; // 直接获取,不用解析
    
    hdr->dstaddr = BUILD_UINT16(*pDat, *(pDat + 1));
    pDat += 2;
    hdr->fc = *pDat++;
    hdr->seq = *pDat++;
    
    return pDat;
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


// 网络层数据请求
int bxNwkDatareq(uint16_t dstaddr, uint8_t *dat, uint16_t len)
{
    uint8_t *pbuf;
    uint8_t *ptemp;
    int status;
    uint8_t size;

    size =  bxNwkHdrLen() + len;
    pbuf = mo_malloc(size);
    if(pbuf == NULL)
        return FALSE;

    ptemp = bxNwkBuildHdr(pbuf, NWK_FC_DATA, dstaddr, 0x00);

    memcpy(ptemp, dat, len);
    status = bxNwk_LowDataRequest(dstaddr, pbuf, size);
    mo_free(pbuf);

    return status;
}

// 网络层命令请求
static int bxNwkCmdreq(uint16_t dstaddr,uint8_t seq, uint8_t *dat, uint16_t len)
{
    uint8_t *pbuf;
    uint8_t *ptemp;
    int status;
    uint8_t size;

    size =  bxNwkHdrLen() + len;
    pbuf = mo_malloc(size);
    if(pbuf == NULL)
        return FALSE;

    ptemp = bxNwkBuildHdr(pbuf, NWK_FC_CMD, dstaddr, seq);

    memcpy(ptemp, dat, len);
    status = bxNwk_LowDataRequest(dstaddr, pbuf, size);
    mo_free(pbuf);

    return status;
}


void bxNwk_ProcessMessageMSG( afIncomingMSGPacket_t *pInMsg )
{
    MoIncomingMsgPkt_t pkt;
    nwkHdr_t hdr;


    if(pInMsg->cmd.DataLength < 4)
        return;

    log_debugln("nwk msg process!");
    // process message
    pkt.apduData = bxNwkParseHdr(&hdr, pInMsg);
    pkt.apduLength = pInMsg->cmd.DataLength - bxNwkHdrLen();
    pkt.srcaddr = hdr.srcaddr;
    pkt.isbroadcast = pInMsg->wasBroadcast;
    
    if(hdr.fc == NWK_FC_DATA){  // 数据应用层
        ltl_ProcessInApdu(&pkt);
    }
    else if(hdr.fc == NWK_FC_CMD){ // 网络层
        bxNwk_ProcessInNpdu(&hdr, &pkt);
    }
    else if(hdr.fc == NWK_FC_PROTOCOL){ // 协议栈层
        // process protocol
    }
    else { // 保留,暂定
        
    // drop it
    }
}

static void bxNwk_ProcessInNpdu(nwkHdr_t *phdr, MoIncomingMsgPkt_t *pkt)
{



}

