

#include "ltl.h"
#include "memalloc.h"
#include "prefix.h"

/*********************************************************************
 * MACROS
 */
/*** Frame Control ***/
#define ltl_FCType( a )                 ( (a) & LTL_FRAMECTL_TYPE_MASK )
#define ltl_FCDisableDefaultRsp( a )    ( (a) & LTL_FRAMECTL_DISALBE_DEFAULT_RSP_MASK )
#define ltl_FCDirection(a)              ( (a) & LTL_FRAMECTL_DIRECTION_MASK ) 

/*** Attribute Access Control ***/
#define ltl_AccessCtrlRead( a )       ( (a) & ACCESS_CONTROL_READ )
#define ltl_AccessCtrlWrite( a )      ( (a) & ACCESS_CONTROL_WRITE )

// Commands that have corresponding responses
#define LTL_PROFILE_CMD_HAS_RSP( cmd )  ( (cmd) == LTL_CMD_READ_ATTRIBUTES            || \
                                        (cmd) == LTL_CMD_READ_ATTRIBUTES_RSP        || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES           || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES_RSP       || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES_NORSP || \
                                        (cmd) == LTL_CMD_CONFIGURE_REPORTING   || \
                                        (cmd) == LTL_CMD_CONFIGURE_REPORTING_RSP || \
                                        (cmd) == LTL_CMD_READ_CONFIGURE_REPORTING || \
                                        (cmd) == LTL_CMD_READ_CONFIGURE_REPORTING_RSP || \
                                        (cmd) == LTL_CMD_DEFAULT_RSP ) // exception

/* local typedef */
typedef void *(*ltlParseInProfileCmd_t)( uint8_t *pdata,uint16_t datalen );
typedef uint8_t (*ltlProcessInProfileCmd_t)( ltlApduMsg_t *ApduMsg );

typedef struct
{
    ltlParseInProfileCmd_t   pfnParseInProfile;
    ltlProcessInProfileCmd_t pfnProcessInProfile;
} ltlCmdItems_t;

typedef struct ltlLibPlugin_s
{
    uint16_t    starttrunkID;         //  start trunk ID
    uint16_t    endtrunkID;         //  end trunk ID
    ltlSpecificTrunckHdCB_t pfnSpecificTrunkHdlr;    // function to handle incoming message
    void *next;
} ltlLibPlugin_t;

// Attribute record list item
typedef struct
{
    uint16_t            trunkID;      // Used to link it into the trunk descriptor
    uint8_t             nodeNO; // Number of the following records
    uint8_t             numAttributes;
    const ltlAttrRec_t  *attrs;        // attribute record
    ltlReadWriteCB_t    pfnReadWriteCB;// Read or Write attribute value callback function
    void *next;
} ltlAttrRecsList_t;

// local function
static uint8_t *ltlSerializeData( uint8_t dataType, void *attrData, uint8_t *buf );
static uint8_t *ltlParseHdr(ltlFrameHdr_t *hdr,uint8_t *pDat);
static void ltlEncodeHdr( ltlFrameHdr_t *hdr,uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, uint8_t specific, uint8_t dir, uint8_t disableDefaultRsp,uint8_t cmd);
static uint8_t *ltlBuildHdr( ltlFrameHdr_t *hdr, uint8_t *pDat );
static uint8_t ltlHdrSize(void);
static ltlLibPlugin_t *ltlFindPlugin( uint16_t trunkID );
static ltlAttrRecsList_t *ltlFindAttrRecsList(uint16_t trunkID, uint8_t nodeNO);

static ltlReadWriteCB_t ltlGetReadWriteCB(uint16_t trunkID, uint8_t nodeNO);
static LStatus_t ltlWriteAttrData(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr, ltlWriteRec_t *pWriteRec );
static LStatus_t ltlWriteAttrDataUsingCB( uint16_t trunkID,uint8_t nodeNO, ltlAttrRec_t *pAttr, uint8_t *pAttrData );

// for parse
static void *ltlParseInReadCmd( uint8_t *pdata,uint16_t datalen );
static void *ltlParseInReadRspCmd( uint8_t *pdata, uint16_t datalen );
static void *ltlParseInWriteCmd(uint8_t *pdata,uint16_t datalen);
static void *ltlParseInWriteRspCmd( uint8_t *pdata, uint16_t datalen );
static void *ltlParseInConfigReportCmd(uint8_t *pdata ,uint16_t datalen);
static void *ltlParseInConfigReportRspCmd(uint8_t *pdata ,uint16_t datalen);
static void *ltlParseInReadReportCfgCmd(uint8_t *pdata ,uint16_t datalen);
static void *ltlParseInReadReportCfgRspCmd(uint8_t *pdata ,uint16_t datalen);
static void *ltlParseInReportCmd( uint8_t *pdata, uint16_t datalen );
static void *ltlParseInDefaultRspCmd(uint8_t *pdata ,uint16_t datalen);

// for process 
static uint8_t ltlProcessInReadCmd(ltlApduMsg_t *ApduMsg);
static uint8_t ltlProcessInWriteCmd(ltlApduMsg_t *ApduMsg);
static uint8_t ltlProcessInWriteUndividedCmd(ltlApduMsg_t *ApduMsg);

// local variable
static ltlLibPlugin_t *libplugins = NULL;
static ltlAttrRecsList_t *attrList = NULL;

static const ltlCmdItems_t ltlCmdTable[] = 
{
    {ltlParseInReadCmd, ltlProcessInReadCmd},           // LTL_CMD_READ_ATTRIBUTES
    {NULL/*ltlParseInReadRspCmd*/, NULL},               //LTL_CMD_READ_ATTRIBUTES_RSP
    {ltlParseInWriteCmd, ltlProcessInWriteCmd},         //LTL_CMD_WRITE_ATTRIBUTES
    {ltlParseInWriteCmd, ltlProcessInWriteUndividedCmd},//LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED
    {NULL/*ltlParseInWriteRspCmd*/, NULL},              //LTL_CMD_WRITE_ATTRIBUTES_RSP
    {ltlParseInWriteCmd, ltlProcessInWriteCmd},         //LTL_CMD_WRITE_ATTRIBUTES_NORSP
    {ltlParseInConfigReportCmd, NULL},           //LTL_CMD_CONFIGURE_REPORTING
    {NULL/*ltlParseInConfigReportRspCmd*/, NULL},       //LTL_CMD_CONFIGURE_REPORTING_RSP
    {ltlParseInReadReportCfgCmd, NULL},         //LTL_CMD_READ_CONFIGURE_REPORTING
    {NULL/*ltlParseInReadReportCfgRspCmd*/, NULL},      //LTL_CMD_READ_CONFIGURE_REPORTING_RSP
    {NULL/*ltlParseInReportCmd*/, NULL},                //LTL_CMD_REPORT_ATTRIBUTES
    {ltlParseInDefaultRspCmd, NULL},            //LTL_CMD_DEFAULT_RSP                          
};

/*********************************************************************
 * @brief       Add a trunk Library handler 注册特定集下命令解析回调,用于解析特定集下命令
 *
 * @param       starttrunkID -  trunk ID
 * @param       endtrunkID -  trunk ID
 * @param       pfnSpecificTrunkHdCB - function pointer to apdu message handler
 *
 * @return      0 if OK
 */
// ok
LStatus_t ltl_registerPlugin(uint16_t starttrunkID, uint16_t endtrunkID, ltlSpecificTrunckHdCB_t pfnSpecificTrunkHdCB)
{
    ltlLibPlugin_t *pNewItem;
    ltlLibPlugin_t *pLoop;

    // Fill in the new profile list
    pNewItem = mo_malloc( sizeof( ltlLibPlugin_t ) );
    if ( !pNewItem ){
        return LTL_MEMERROR;
    }

    // Fill in the plugin record.
    pNewItem->starttrunkID = starttrunkID;
    pNewItem->endtrunkID = endtrunkID;
    pNewItem->pfnSpecificTrunkHdlr = pfnSpecificTrunkHdCB;
    pNewItem->next = (ltlLibPlugin_t *)NULL;
    
    // Find spot in list
    if ( libplugins == NULL ){
        libplugins = pNewItem;
    }
    else{
        // Look for end of list
        pLoop = libplugins;
        while ( pLoop->next != NULL )
        {
            pLoop = pLoop->next;
        }

        // Put new item at end of list
        pLoop->next = pNewItem;
    }

    return LTL_SUCCESS;
}

/*********************************************************************
 * @fn         
 *              注册集下 指定节点的属性列表
 * @brief      
 *
 * @param       trunkID -  trunk ID
 * @param       nodeNO -  node number
 * @param       numAttr -  attribute number 
 * @param       newAttrList[] - list of attrubute
 *
 * @return      0 if OK
 */
// ok
LStatus_t ltl_registerAttrList(uint16_t trunkID, uint8_t nodeNO, uint8_t numAttr, const ltlAttrRec_t newAttrList[] )
{
    ltlAttrRecsList_t *pNewItem;
    ltlAttrRecsList_t *pLoop;

    // Fill in the new profile list
    pNewItem = mo_malloc( sizeof( ltlAttrRecsList_t ) );
    if ( !pNewItem ){
        return LTL_MEMERROR;
    }

    pNewItem->trunkID = trunkID;
    pNewItem->nodeNO = nodeNO;
    pNewItem->numAttributes = numAttr;
    pNewItem->attrs = newAttrList;
    pNewItem->pfnReadWriteCB = NULL;
    pNewItem->next = (void *)NULL;
    
    // Find spot in list
    if ( attrList == NULL ) {
        attrList = pNewItem;
    }
    else{
        // Look for end of list
        pLoop = attrList;
        while ( pLoop->next != NULL )
        {
            pLoop = pLoop->next;
        }

        // Put new item at end of list
        pLoop->next = pNewItem;
    }

    return LTL_SUCCESS;
}

/*********************************************************************
 *              注册用户回调函数,处理属性和属性的授权  
 * @brief       Register the application's callback function to read/write
 *              attribute data, and authorize read/write operation.
 *
 *              Note: The pfnReadWriteCB callback function is only required
 *                    when the attribute data format is unknown to LTL. The
 *                    callback function gets called when the pointer 'dataPtr'
 *                    to the attribute value is NULL in the attribute database
 *                    registered with the LTL.  
 *              dataptr为NULL时,将调用此回调,用户处理此数据
 *
 * @param       pfnReadWriteCB - function pointer to read/write routine
 * @param       pfnAuthorizeCB - function pointer to authorize read/write operation
 *
 * @return      LTL_SUCCESS if successful. LTL_FAILURE, otherwise.
 */
LStatus_t ltl_registerReadWriteCB(uint16_t trunkID, uint8_t nodeNO, ltlReadWriteCB_t pfnReadWriteCB )
{
    ltlAttrRecsList_t *pRec = ltlFindAttrRecsList(trunkID, nodeNO);

    if ( !pRec ){
        return ( LTL_FAILURE );
    }
    
    pRec->pfnReadWriteCB = pfnReadWriteCB;
    return ( LTL_SUCCESS );
}

/*********************************************************************
 * @brief   Used to send Profile and trunk Specific Command messages.
 *
 *          NOTE: The calling application is responsible for incrementing
 *                the Sequence Number.
 * 
 * @param   pAddr - address
 * @param   trunkID - trunk ID
 * @param   nodeNO -  node number
 * @param   seqNumber - identification number for the transaction
 * @param   specific - whether the command is trunk Specific
 * @param   direction - client/server direction of the command
 * @param   manuCode - manufacturer code for proprietary extensions to a profile
 * @param   disableDefaultRsp - disable Default Response command 
 * @param   cmd - command ID
 * @param   cmdFormat - in ---- command to be sent
 * @param   cmdFormatLen - length of the command to be sent
 *
 * @return  LTL_STATUS_SUCCESS if OK
 */
LStatus_t ltl_SendCommand(uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, 
                                uint8_t specific,uint8_t dir, uint8_t disableDefaultRsp,
                                uint8_t cmd, uint8_t *cmdFormat,uint16_t cmdFormatLen)
{
    ltlFrameHdr_t hdr;
    uint8_t *msgbuf;
    uint8_t *pbuf;
    uint16_t msglen;
    uint16_t prefixlen;
    LStatus_t status;
    
    memset((uint8_t *)&hdr,0,sizeof(ltlFrameHdr_t));  
    ltlEncodeHdr(&hdr, trunkID, nodeNO, seqNum, specific, dir, disableDefaultRsp, cmd);

    msglen = ltlHdrSize();
    //获得前置预留长度
    prefixlen = ltlprefixHdrsize(dstAddr);
    msglen += prefixlen + cmdFormatLen;

    msgbuf = (uint8_t *)mo_malloc(msglen);
    if(!msgbuf){
        return LTL_MEMERROR;
    }
    
    pbuf = msgbuf;
    //填充前置预留空间
    pbuf = ltlPrefixBuildHdr(dstAddr, pbuf);
    pbuf = ltlBuildHdr(&hdr, pbuf);
    memcpy(pbuf, cmdFormat, cmdFormatLen);

    status = ltlPrefixrequest(dstAddr, msgbuf, msglen);
    mo_free(msgbuf);
    
    return status;
}

/*********************************************************************
 * @param   hdr -  out ----place to put the frame head  information
 * @param   pDat - in  ---- incoming apdu buffer to parse
 * @ 解析帧头,获得帧头后的指针
 * @return  pointer past the header 
 */
static uint8_t *ltlParseHdr(ltlFrameHdr_t *hdr, uint8_t *pDat)
{
    memset((uint8_t *)hdr,0,sizeof(ltlFrameHdr_t));

    hdr->trunkID = BUILD_UINT16(*pDat, *(pDat +1));
    pDat += 2;  
    
    hdr->nodeNo = *pDat++;
    hdr->transSeqNum = *pDat++;

    hdr->fc.type = ltl_FCType(*pDat);
    hdr->fc.disableDefaultRsp = ltl_FCDisableDefaultRsp(*pDat) ? LTL_FRAMECTL_DIS_DEFAULT_RSP_ON : LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF;
    hdr->fc.dir = ltl_FCDirection(*pDat) ? LTL_FRAMECTL_CLIENT_SERVER_DIR : LTL_FRAMECTL_SERVER_CLIENT_DIR;
    pDat++;

    hdr->commandID = *pDat++;
    
    return (pDat);
}
/*********************************************************************
 * @brief   encode to frame header
 *
 * @param   hdr - out ---- header information
 * @param   trunkID - 
 * @param   nodeNO -
 * @param   seqNum -
 * @param   specific -
 * @param   direction -
 * @param   disableDefaultRsp -
 * @param   cmd -
 *
 * @return  no
 */
static void ltlEncodeHdr( ltlFrameHdr_t *hdr,uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, 
                                uint8_t specific,uint8_t dir,uint8_t disableDefaultRsp,uint8_t cmd)
{
    // trunkID
    hdr->trunkID = trunkID;  
    // node number
    hdr->nodeNo = nodeNO;    
    //Transaction Sequence Number
    hdr->transSeqNum = seqNum;
    // Add the Trunk's command ID
    hdr->commandID = cmd;
    // Build the Frame Control byte
    hdr->fc.type = specific ? LTL_FRAMECTL_TYPE_TRUNK_SPECIFIC : LTL_FRAMECTL_TYPE_PROFILE;
    hdr->fc.disableDefaultRsp =  disableDefaultRsp ? LTL_FRAMECTL_DIS_DEFAULT_RSP_ON : LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF;
    hdr->fc.dir = dir ? LTL_FRAMECTL_CLIENT_SERVER_DIR : LTL_FRAMECTL_SERVER_CLIENT_DIR;
}

/*********************************************************************
 * @brief   Build header of the ltL format
 *
 * @param   hdr - in ---- header information
 * @param   pDat -out ---- outgoing header space
 *
 * @return  pointer past the header
 */
static uint8_t *ltlBuildHdr( ltlFrameHdr_t *hdr, uint8_t *pDat )
{
    *pDat++ = LO_UINT16(hdr->trunkID);
    *pDat++ = HI_UINT16(hdr->trunkID);
    // node number
    *pDat++ = hdr->nodeNo;
    //Transaction Sequence Number
    *pDat++ = hdr->transSeqNum;
    
    // Build the Frame Control byte
    *pDat = hdr->fc.type;
    *pDat |= hdr->fc.disableDefaultRsp << 2;
    *pDat |= hdr->fc.dir << 3;
    pDat++;  // move past the frame control field
    
    // Add the Trunk's command ID
    *pDat++ = hdr->commandID;

    // Should point to the frame payload
    return ( pDat );
}

/*********************************************************************
 * @brief       get frame head length
 *
 * @param       hdr -  pointer to the head frame struct
 *
 * @return      size of head
 */
static uint8_t ltlHdrSize(void)
{
    return (2 + 1 + 1 + 1 + 1); //trunkID + seqnum + nodeNO + frame control + cmdID;
}
/*********************************************************************
 * @brief       find a trunk Library pointer
 *
 * @param       trunkID -  trunk ID
 *
 * @return      pointer to trunk lib,if NULL not find
 */
static ltlLibPlugin_t *ltlFindPlugin( uint16_t trunkID )
{
    ltlLibPlugin_t *pLoop = libplugins;

    while ( pLoop != NULL )
    {
        if ( (trunkID >= pLoop->starttrunkID) &&  (trunkID <= pLoop->endtrunkID ) ){
            return ( pLoop );
        }

        pLoop = pLoop->next;
    }

    return ( (ltlLibPlugin_t *)NULL );
}

/*********************************************************************
 * @brief   Find the right attribute record list for an trunkID and nodeNumber
 *
 * @param   trunkID  -  look for
 * @param   nodeNO   -  look for
 *
 * @return  pointer to record list, NULL if not found
 */

static ltlAttrRecsList_t *ltlFindAttrRecsList(uint16_t trunkID, uint8_t nodeNO)
{
    ltlAttrRecsList_t *pLoop = attrList;

    while ( pLoop != NULL )
    {
        if ( pLoop->nodeNO == nodeNO && pLoop->trunkID == trunkID){
            return ( pLoop );
        }

        pLoop = (ltlAttrRecsList_t *)pLoop->next;
    }

    return ( NULL );
}
/*********************************************************************
 * @brief   Find the attribute record that matchs the parameters
 *
 * @param   trunkID - trunk ID
 * @param   nodeNO - node number
 * @param   attrId - attribute looking for
 * @param   pAttr - out---- attribute record to be returned
 *
 * @return  TRUE if record found. FALSE, otherwise.
 */
uint8_t ltlFindAttrRec( uint16_t trunkID,  uint8_t nodeNO, uint16_t attrId, ltlAttrRec_t *pAttr )
{
    uint8_t i;
    ltlAttrRecsList_t *pRec = ltlFindAttrRecsList( trunkID, nodeNO);

    if ( pRec == NULL ){
        return ( FALSE );
    }
    
    for ( i = 0; i < pRec->numAttributes; i++ ){
        if (pRec->attrs[i].attrId == attrId ){
            *pAttr = pRec->attrs[i]; // TODO check?
    
            return ( TRUE );
        }
    }
    
    return FALSE;
}

/*
 * @brief   Verifies endianness in system.
 *
 * @param   none
 *
 * @return  MSB-00 or LSB-01 depending on endianness in the system
 */
static int ltlIsLittleEndianMachine(void)
{
  uint16_t test = 0x0001;

  return (*((uint8_t *)(&test)));
}

/*********************************************************************
 * @brief   Build an analog arribute out of sequential bytes.
 *
 * @param   dataType - type of data
 * @param   pData - pointer to data
 * @param   pBuf - where to put the data
 *
 * @return  none
 */
static void ltl_BuildAnalogData( uint8_t dataType, uint8_t *pData, uint8_t *pBuf )
{
    int current_byte_index;
    int bytes;
    int step;

    bytes = ltlGetBaseDataTypeLength( dataType);
  
    // decide if move forward or backwards to copy data
    if ( ltlIsLittleEndianMachine() ) {
        step = 1;
        current_byte_index = 0;
    }
    else {
        step = -1;
        current_byte_index = bytes - 1;
    }
  
    while ( bytes-- ) {
        pData[current_byte_index] = *(pBuf++);
        current_byte_index += step;
    }
}


uint8_t ltlIsValidDataType (uint8_t dataType )
{
    if (dataType < LTL_DATATYPE_UNKNOWN){
        return TRUE;
    }

    return FALSE;
}

uint8_t ltlIsBaseDataType( uint8_t dataType)
{
    if (dataType <= LTL_DATATYPE_DOUBLE_PREC){
        return TRUE;
    }

    return FALSE;
}

uint8_t ltlIsComplexDataType(uint8_t dataType)
{
    if ((dataType >= LTL_DATATYPE_CHAR_STR) && (dataType < LTL_DATATYPE_UNKNOWN)){
        return TRUE;
    }

    return FALSE;
}

uint8_t ltlIsAnalogDataType( uint8_t dataType )
{
    if (ltlIsBaseDataType( dataType) && dataType != LTL_DATATYPE_BOOLEAN){
        return TRUE;
    }

    return FALSE;
}

/*********************************************************************
 * @brief   Find the base data type length that matchs the dataType 获取基本数据类型的数据长度
 *
 * @param   dataType - data type
 *
 * @return  length of data type
 */
uint8_t ltlGetBaseDataTypeLength( uint8_t dataType )
{
    uint8_t len = 0;

    switch ( dataType ){
        case LTL_DATATYPE_BOOLEAN:
        case LTL_DATATYPE_UINT8:
        case LTL_DATATYPE_INT8:
            len = 1;
            break;

        case LTL_DATATYPE_UINT16:
        case LTL_DATATYPE_INT16:
            len = 2;
            break;

        case LTL_DATATYPE_UINT32:
        case LTL_DATATYPE_INT32:
        case LTL_DATATYPE_SINGLE_PREC:
            len = 4;
            break;
        case LTL_DATATYPE_UINT64:
        case LTL_DATATYPE_INT64:
        case LTL_DATATYPE_DOUBLE_PREC:
            len = 8;
            break;

        default: //LTL_DATATYPE_UNKNOWN
            len = 0;
        break;
    }

    return ( len );
}

/*********************************************************************
 * @brief   Return the length of the attribute.  获得属性的数据长度
 *
 * @param   dataType - data type
 * @param   pData - in ---- pointer to data 
 *
 * @return  returns atrribute length
 */
uint16_t ltlGetAttrDataLength( uint8_t dataType, uint8_t *pData )
{
    if (ltlIsComplexDataType( dataType)){
        return ( *pData + OCTET_CHAR_HEADROOM_LEN); // 1 for length field
    }
    
    return ltlGetBaseDataTypeLength( dataType );
}

/*********************************************************************
 * @brief   Builds a buffer from the attribute data
 *
 * @param   dataType - data types defined in LTL.h
 * @param   attrData - in --- pointer to the attribute data
 * @param   buf - out -- where to put the serialized data
 *
 * @return  pointer to end of destination buffer
 */
static uint8_t *ltlSerializeData( uint8_t dataType, void *attrData, uint8_t *buf )
{
    uint8_t *pStr;
    uint16_t len;

    switch ( dataType ){
        case LTL_DATATYPE_BOOLEAN:
        case LTL_DATATYPE_INT8:
        case LTL_DATATYPE_UINT8:
            *buf++ = *((uint8_t *)attrData);
            break;

        case LTL_DATATYPE_UINT16:
        case LTL_DATATYPE_INT16:
            *buf++ = LO_UINT16( *((uint16_t *)attrData) );
            *buf++ = HI_UINT16( *((uint16_t *)attrData) );
            break;

        case LTL_DATATYPE_UINT32:
        case LTL_DATATYPE_INT32:
        case LTL_DATATYPE_SINGLE_PREC:
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 0 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 1 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 2 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 3 );
            break;

        case LTL_DATATYPE_UINT64:
        case LTL_DATATYPE_INT64:
        case LTL_DATATYPE_DOUBLE_PREC:
            pStr = (uint8_t *)attrData;
            buf = memcpy( buf, pStr, 8 );
            break;

        case LTL_DATATYPE_CHAR_STR:
        case LTL_DATATYPE_UINT8_ARRAY:
        case LTL_DATATYPE_INT8_ARRAY:
            pStr = (uint8_t *)attrData;
            len = *pStr;
            buf = memcpy( buf, pStr, len + 1 ); // Including length field
            break;
            
        case LTL_DATATYPE_UINT16_ARRAY:
        case LTL_DATATYPE_INT16_ARRAY:
            pStr = (uint8_t *)attrData;
            len = *pStr * 2;
            buf = memcpy( buf, pStr, len + 1 ); // Including length field
            break;
        case LTL_DATATYPE_UINT32_ARRAY:
        case LTL_DATATYPE_INT32_ARRAY:
            pStr = (uint8_t *)attrData;
            len = *pStr * 4;
            buf = memcpy( buf, pStr, len + 1 ); // Including length field
            break;         
        case LTL_DATATYPE_UINT64_ARRAY:
        case LTL_DATATYPE_INT64_ARRAY:
            pStr = (uint8_t *)attrData;
            len = *pStr * 8;
            buf = memcpy( buf, pStr, len + 1 ); // Including length field
            break;
        case LTL_DATATYPE_UNKNOWN:
        // Fall through

        default:
            break;
    }

    return ( buf );
}
/*********************************************************************
 * @brief   Get the Read/Write callback function pointer
 *
 * @param   trunkID -
 * @param   nodeNO - 
 *
 * @return  Read/Write CB, NULL if not found
 */
static ltlReadWriteCB_t ltlGetReadWriteCB(uint16_t trunkID, uint8_t nodeNO)
{
    ltlAttrRecsList_t *pRec = ltlFindAttrRecsList( trunkID, nodeNO );

    if ( pRec != NULL ) {
        return ( pRec->pfnReadWriteCB );
    }

    return ( NULL );
}

/*********************************************************************
 * @brief   Get the Read/Write readwrite length 
 *          here may be user`s database
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   attrId - 
 *
 * @return  length
 */
static uint16_t ltlGetAttrDataLengthUsingCB( uint16_t trunkID, uint8_t nodeNO,  uint16_t attrId )
{
    uint16_t dataLen = 0;
    ltlReadWriteCB_t pfnReadWriteCB = ltlGetReadWriteCB( trunkID, nodeNO );

    if ( pfnReadWriteCB ){ // Only get the attribute length
        (*pfnReadWriteCB)( trunkID, nodeNO, attrId, LTL_OPER_LEN, NULL, &dataLen );
    }

    return ( dataLen );
}
 /*********************************************************************
 * @brief   the Read/Write readwrite callback function process using read
 *          here may be user`s database
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   attrId - 
 * @param   pAttrData - out --- attribute pointer 
 * @param   pDataLen - out  -- attribute length pointer
 *
 * @return  LTL_STATUS_SUCCESS
 */
static LStatus_t ltlReadAttrDataUsingCB( uint16_t trunkID,uint8_t nodeNO,  uint16_t attrId ,
                                         uint8_t *pAttrData, uint16_t *pDataLen )
{
    ltlReadWriteCB_t pfnReadWriteCB = ltlGetReadWriteCB( trunkID, nodeNO );

    if ( pDataLen ){
        *pDataLen = 0; // Always initialize it to 0
    }

    if ( pfnReadWriteCB ){
        // Read the attribute value and its length
        return ( (*pfnReadWriteCB)( trunkID, nodeNO, attrId, LTL_OPER_READ, pAttrData, pDataLen ) );
    }

    return ( LTL_STATUS_SOFTWARE_FAILURE );
}

/*********************************************************************
 * @brief   Write the attribute's current value into pAttrData.
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   pAttr - in --- pointer to attribute,  where to write data to 
 * @param   pWriteRec - in -- pointer to attribute ,data to be written
 *
 * @return success or read only
 */
static LStatus_t ltlWriteAttrData(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr, ltlWriteRec_t *pWriteRec )
{
    uint16_t len;

    if ( ltl_AccessCtrlWrite( pAttr->accessControl ) ){
        len = ltlGetAttrDataLength(pAttr->dataType, pWriteRec->attrData);
        memcpy( pAttr->dataPtr, pWriteRec->attrData, len );
    
        return LTL_STATUS_SUCCESS;
    }

    return LTL_STATUS_READ_ONLY;
}
/*********************************************************************
 * @brief   Write the attribute's current value into pAttrData.
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   pAttr - in --- pointer to attribute, where to write data to 
 * @param   pWriteRec - in -- pointer to attribute ,data to be written
 *
 * @return 
 */
static LStatus_t ltlWriteAttrDataUsingCB( uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr, uint8_t *pAttrData )
{
    uint8_t status;
    ltlReadWriteCB_t pfnReadWriteCB;

    if ( ltl_AccessCtrlWrite( pAttr->accessControl ) ){
        pfnReadWriteCB = ltlGetReadWriteCB( trunkID, nodeNO);
        if ( pfnReadWriteCB ){
            // Write the attribute value
            status = (*pfnReadWriteCB)( trunkID, nodeNO, pAttr->attrId, LTL_OPER_WRITE, pAttrData, NULL );
        }
        else{
            status = LTL_STATUS_SOFTWARE_FAILURE;
        }
    }
    else{
        status = LTL_STATUS_READ_ONLY;
    }

    return ( status );
}

// 读属性命令
LStatus_t ltl_SendReadReq(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO, uint8_t seqNum, ltlReadCmd_t *readCmd )
{
    uint8_t i;
    uint16_t dataLen;
    uint8_t *buf;
    uint8_t *pBuf;
    LStatus_t status;

    dataLen = readCmd->numAttr * 2; // Attribute ID

    buf = mo_malloc( dataLen );
    if (!buf){
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for (i = 0; i < readCmd->numAttr; i++) {
        *pBuf++ = LO_UINT16( readCmd->attrID[i] );
        *pBuf++ = HI_UINT16( readCmd->attrID[i] );
    }

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_CLIENT_SERVER_DIR, TRUE, LTL_CMD_READ_ATTRIBUTES, buf,  dataLen);
    mo_free( buf );

    return ( status );
}

// 读属性应答命令
LStatus_t ltl_SendReadRsp(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO, uint8_t seqNum, ltlReadRspCmd_t *readRspCmd )
{
    uint8_t i;
    uint16_t len = 0;
    uint16_t dataLen;
    LStatus_t status;
    uint8_t *buf;
    uint8_t *pBuf;
    ltlReadRspStatus_t *statusRec;

    // calculate the size of the command
    for ( i = 0; i < readRspCmd->numAttr; i++ ) {
        statusRec = &(readRspCmd->attrList[i]); // get every Read Attribute Response Status record

        len += 2 + 1; // Attribute ID + Status

        //success contain attribute data type and attribute value
        if ( statusRec->status == LTL_STATUS_SUCCESS ){
            len += 1; // Attribute Data Type length

            // Attribute Data length
            if ( statusRec->data != NULL ){
                len += ltlGetAttrDataLength( statusRec->dataType, statusRec->data );
            }
            else{
                len += ltlGetAttrDataLengthUsingCB( trunkID, nodeNO, statusRec->attrID );
            }
        }
    }

    buf = mo_malloc( len );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < readRspCmd->numAttr; i++ ){
        statusRec = &(readRspCmd->attrList[i]);

        *pBuf++ = LO_UINT16( statusRec->attrID );
        *pBuf++ = HI_UINT16( statusRec->attrID );
        *pBuf++ = statusRec->status;

        if ( statusRec->status == LTL_STATUS_SUCCESS ){
            *pBuf++ = statusRec->dataType;

            if ( statusRec->data != NULL ){
                // Copy attribute data to the buffer to be sent out
                pBuf = ltlSerializeData( statusRec->dataType, (void *)statusRec->data, pBuf );
            }
            else{
                dataLen = 0;
                // Read attribute data directly into the buffer to be sent out
                ltlReadAttrDataUsingCB( trunkID, nodeNO, statusRec->attrID, pBuf, &dataLen );
                pBuf += dataLen;// move to next pointer buff
            }
        }
    } // for loop

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE, LTL_CMD_READ_ATTRIBUTES_RSP, buf, len);
    mo_free( buf );

    return ( status );
}
LStatus_t ltl_SendWriteRequest(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO,
                                uint8_t seqNum, uint8_t cmd, ltlWriteCmd_t *writeCmd )
{
    uint8_t *buf;
    uint8_t *pBuf;
    uint8_t i;
    uint16_t dataLen = 0;
    LStatus_t status;
    ltlWriteRec_t *statusRec;

    for ( i = 0; i < writeCmd->numAttr; i++ ){
        statusRec = &(writeCmd->attrList[i]);

        dataLen += 2 + 1; // Attribute ID + Attribute Type

        // Attribute Data
        dataLen += ltlGetAttrDataLength( statusRec->dataType, statusRec->attrData );
    }

    buf = mo_malloc( dataLen );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < writeCmd->numAttr; i++ ){
        statusRec = &(writeCmd->attrList[i]);

        *pBuf++ = LO_UINT16( statusRec->attrID );
        *pBuf++ = HI_UINT16( statusRec->attrID );
        *pBuf++ = statusRec->dataType;

        pBuf = ltlSerializeData( statusRec->dataType, statusRec->attrData, pBuf );
    }
    
    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_CLIENT_SERVER_DIR, TRUE, cmd, buf, dataLen);
    mo_free( buf );

    return ( status);
}

LStatus_t ltl_SendWriteRsp(uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum, ltlWriteRspCmd_t *writeRspCmd)
{
    uint8_t i;
    uint16_t len = 0;
    uint8_t *pbuf;
    uint8_t *buf;
    LStatus_t status;
    ltlWriteRspStatus_t *statusRec;

    // calculate the size of the command
    len = writeRspCmd->numAttr * (1 + 2); //status + attribute id
    
    buf = mo_malloc( len );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
    pbuf = buf;
    for(i = 0;i < writeRspCmd->numAttr; i++){
        statusRec = &(writeRspCmd->attrList[i]);
        *pbuf++ = statusRec->status;
        *pbuf++ = LO_UINT16( statusRec->attrID );
        *pbuf++ = HI_UINT16( statusRec->attrID );
    }

    if(writeRspCmd->numAttr == 1 && writeRspCmd->attrList[0].status == LTL_STATUS_SUCCESS){
        len = 1;
    }
    
    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE,LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE, LTL_CMD_WRITE_ATTRIBUTES_RSP, buf, len);              
    mo_free( buf );

    return status;
}
                                 
LStatus_t ltl_SendReportCmd( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum,
                                    uint8_t disableDefaultRsp, ltlReportCmd_t *reportCmd)
{
    uint8_t i;
    uint16_t dataLen;
    uint8_t *buf;
    uint8_t *pBuf;
    LStatus_t status;
    ltlReport_t *reportRec;

    // calculate the size of the command
    dataLen = 0;
    for ( i = 0; i < reportCmd->numAttr; i++ ){
        reportRec = &(reportCmd->attrList[i]);

        dataLen += 2 + 1; // Attribute ID + data type

        // + Attribute Data
        dataLen += ltlGetAttrDataLength( reportRec->dataType, reportRec->attrData );
    }

    buf = mo_malloc( dataLen );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < reportCmd->numAttr; i++ ){
        reportRec = &(reportCmd->attrList[i]);

        *pBuf++ = LO_UINT16( reportRec->attrID );
        *pBuf++ = HI_UINT16( reportRec->attrID );
        *pBuf++ = reportRec->dataType;

        pBuf = ltlSerializeData( reportRec->dataType, reportRec->attrData, pBuf );
    }

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_CLIENT_SERVER_DIR, disableDefaultRsp, LTL_CMD_REPORT_ATTRIBUTES, buf, dataLen);
    mo_free( buf );

    return ( status );
}
                                 
LStatus_t ltl_SendConfigReportReq( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                            uint8_t seqNum , ltlCfgReportCmd_t *cfgReportCmd)
{
    uint16_t dataLen = 0;
    LStatus_t status;    
    uint8_t *buf;
    uint8_t *pBuf;
    uint8_t i;
    ltlCfgReportRec_t *reportRec;

    // Find out the data length
    for ( i = 0; i < cfgReportCmd->numAttr; i++ ) {
        ltlCfgReportRec_t *reportRec = &(cfgReportCmd->attrList[i]);

        dataLen += 2 + 1 + 2; // Attribute ID + Data Type + Min
        // Find out the size of the Reportable Change field (for Analog data types)
        if ( ltlIsAnalogDataType( reportRec->dataType ) ) {
            dataLen += ltlGetBaseDataTypeLength( reportRec->dataType );
        }

    }

    buf = mo_malloc( dataLen );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
     // Load the buffer - serially
    pBuf = buf;

    for ( i = 0; i < cfgReportCmd->numAttr; i++ ) {
        reportRec = &(cfgReportCmd->attrList[i]);
  
        *pBuf++ = LO_UINT16( reportRec->attrID );
        *pBuf++ = HI_UINT16( reportRec->attrID ); 
        *pBuf++ = reportRec->dataType;
        *pBuf++ = LO_UINT16( reportRec->minReportInt );
        *pBuf++ = HI_UINT16( reportRec->minReportInt );
  
        if ( ltlIsAnalogDataType( reportRec->dataType ) ){
            pBuf = ltlSerializeData( reportRec->dataType, reportRec->reportableChange, pBuf );
        }

    } // for loop

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_CLIENT_SERVER_DIR, TRUE, LTL_CMD_CONFIGURE_REPORTING, buf, dataLen);
    mo_free( buf );

    return ( status );
}
                          
LStatus_t ltl_SendConfigReportRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                 uint8_t seqNum, ltlCfgReportRspCmd_t *cfgReportRspCmd)
{
    uint16_t dataLen;
    uint8_t *buf;
    uint8_t *pBuf;
    uint8_t i;
    LStatus_t status;

    // Atrribute list (Status and Attribute ID)
    dataLen = cfgReportRspCmd->numAttr * ( 1 + 2 );

    buf = mo_malloc( dataLen );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < cfgReportRspCmd->numAttr; i++ ) {
        *pBuf++ = cfgReportRspCmd->attrList[i].status;
        *pBuf++ = LO_UINT16( cfgReportRspCmd->attrList[i].attrID );
        *pBuf++ = HI_UINT16( cfgReportRspCmd->attrList[i].attrID );
    }

    // If there's only a single status record and its status field is set to
    // SUCCESS then omit the attribute ID field.
    if ( cfgReportRspCmd->numAttr == 1 && cfgReportRspCmd->attrList[0].status == LTL_STATUS_SUCCESS ) {
        dataLen = 1;
    }

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                             LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE, LTL_CMD_CONFIGURE_REPORTING_RSP, buf, dataLen);
    mo_free( buf );

  return ( status );
}

LStatus_t ltl_SendReadReportCfgReq( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                 uint8_t seqNum , ltlReadReportCfgCmd_t *readReportCfgCmd)
{
    uint16_t dataLen;
    uint8_t *buf;
    uint8_t *pBuf;
    uint8_t i;
    LStatus_t status;

    dataLen = readReportCfgCmd->numAttr * 2; // Atrribute ID

    buf = mo_malloc( dataLen );
    if ( !buf ) {
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < readReportCfgCmd->numAttr; i++ ){
        *pBuf++ = LO_UINT16( readReportCfgCmd->attrID[i] );
        *pBuf++ = HI_UINT16( readReportCfgCmd->attrID[i] );
    }

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE,LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE, LTL_CMD_READ_CONFIGURE_REPORTING, buf, dataLen);
    mo_free( buf );

  return ( status );
}

LStatus_t ltl_SendReadReportCfgRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                 uint8_t seqNum , ltlReadReportCfgRspCmd_t *readReportCfgRspCmd)
{
    uint16_t dataLen = 0;
    LStatus_t status;
    uint8_t i;
    uint8_t *buf;
    uint8_t *pBuf;
    ltlReportCfgRspRec_t *reportRspRec;

    // Find out the data length
    for ( i = 0; i < readReportCfgRspCmd->numAttr; i++ ) {
        reportRspRec = &(readReportCfgRspCmd->attrList[i]);
        dataLen += 1 + 2 ; // Status and Atrribute ID
        if ( reportRspRec->status == LTL_STATUS_SUCCESS ) {
            dataLen += 1 + 2; // Data Type + Min
            // Find out the size of the Reportable Change field (for Analog data types)
            if ( ltlIsAnalogDataType( reportRspRec->dataType ) ) {
                dataLen += ltlGetBaseDataTypeLength( reportRspRec->dataType );
            }
        }
    }

    buf = mo_malloc( dataLen );
    if ( !buf ){
        return LTL_MEMERROR;
    }
    
    // Load the buffer - serially
    pBuf = buf;
    for ( i = 0; i < readReportCfgRspCmd->numAttr; i++ ){
        reportRspRec = &(readReportCfgRspCmd->attrList[i]);

        *pBuf++ = reportRspRec->status;
        *pBuf++ = LO_UINT16( reportRspRec->attrID );
        *pBuf++ = HI_UINT16( reportRspRec->attrID );

        if ( reportRspRec->status == LTL_STATUS_SUCCESS ) {
            *pBuf++ = reportRspRec->dataType;
            *pBuf++ = LO_UINT16( reportRspRec->minReportInt );
            *pBuf++ = HI_UINT16( reportRspRec->minReportInt );

            if ( ltlIsAnalogDataType( reportRspRec->dataType ) ) {
                pBuf = ltlSerializeData( reportRspRec->dataType,
                reportRspRec->reportableChange, pBuf );
            }
        }
    }

    status = ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, 
                            LTL_FRAMECTL_TYPE_PROFILE, LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE, LTL_CMD_READ_CONFIGURE_REPORTING_RSP, buf, dataLen);
    mo_free( buf );

    return ( status );
}
                                 
//ok
LStatus_t ltl_SendDefaultRsp( uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO,
                                uint8_t seqNum, ltlDefaultRspCmd_t *defaultRspCmd)
{
  uint8_t buf[2]; // Command ID and Status;

  // Load the buffer - serially
  buf[0] = defaultRspCmd->commandID;
  buf[1] = defaultRspCmd->statusCode;

  return (ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, LTL_FRAMECTL_TYPE_PROFILE,LTL_FRAMECTL_SERVER_CLIENT_DIR, TRUE,
                          LTL_CMD_DEFAULT_RSP, buf, 2));
}

/*********************************************************************
 * @brief   Parse the "Profile" Read Commands
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
 //ok
static void *ltlParseInReadCmd( uint8_t *pdata, uint16_t datalen )
{
    ltlReadCmd_t *readCmd;
    uint8_t i;    
    
    readCmd = (ltlReadCmd_t *)mo_malloc( sizeof ( ltlReadCmd_t ) + datalen );
    if ( !readCmd ){
        return (void *)NULL;
   }
    
    readCmd->numAttr = datalen / 2; // Atrribute ID number
    for ( i = 0; i < readCmd->numAttr; i++ ){
        readCmd->attrID[i] = BUILD_UINT16( pdata[0], pdata[1] );
        pdata += 2;
    }

    return ( (void *)readCmd );
}

/*********************************************************************
 * @brief   Parse the "Profile" Read Response Commands
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pCmd - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInReadRspCmd( uint8_t *pdata, uint16_t datalen )
{
    ltlReadRspCmd_t *readRspCmd;
    uint8_t *pBuf = pdata;
    uint8_t *dataPtr;
    uint8_t numAttr = 0;
    uint8_t hdrLen;
    uint16_t dataLength = 0;
    uint16_t attrDataLen;
    uint8_t status;
    uint8_t dataType;
    uint8_t i;

    // find out the number of attributes and the length of attribute data
    while ( pBuf < ( pdata + dataLength ) )
    {
    
        numAttr++;
        pBuf += 2; // move pass attribute id
    
        status = *pBuf++;
        if ( status == LTL_STATUS_SUCCESS ) {
           dataType = *pBuf++;
     
           attrDataLen = ltlGetAttrDataLength( dataType, pBuf );
           pBuf += attrDataLen; // move pass attribute data
     
           // add padding if needed
           if ( attrDataLen % 2 ) {
                attrDataLen++;
           }
     
           dataLength += attrDataLen;
        }
    }
  
    // calculate the length of the response header
    hdrLen = sizeof( ltlReadRspCmd_t ) + ( numAttr * sizeof( ltlReadRspStatus_t ) );
  
    readRspCmd = (ltlReadRspCmd_t *)mo_malloc( hdrLen + dataLength );
    if ( !readRspCmd ){
        return (void *) NULL;
    }

    pBuf = pdata;
    dataPtr = (uint8_t *)( (uint8_t *)readRspCmd + hdrLen );

    readRspCmd->numAttr = numAttr;
    for ( i = 0; i < numAttr; i++ ){
        ltlReadRspStatus_t *statusRec = &(readRspCmd->attrList[i]);

        statusRec->attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
  
        statusRec->status = *pBuf++;
        if ( statusRec->status == LTL_STATUS_SUCCESS ) {
            statusRec->dataType = *pBuf++;
    
            attrDataLen = ltlGetAttrDataLength( statusRec->dataType, pBuf );
            memcpy( dataPtr, pBuf, attrDataLen);
            statusRec->data = dataPtr;
    
            pBuf += attrDataLen; // move pass attribute data
    
            // advance attribute data pointer
            if ( attrDataLen % 2){
                attrDataLen++;
            }
    
            dataPtr += attrDataLen;
        }
    }
  
    return ( (void *)readRspCmd );
}


/*********************************************************************
 * @brief   Parse the "Profile" Write Commands 
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 * format:  (ltlWriteCmd_t) |numAttr | (ltlWriteRec_t) attrID dataType attrData ....  | -- real data here
 */
 //OK
static void *ltlParseInWriteCmd(uint8_t *pdata ,uint16_t datalen)
{
    uint8_t i;
    uint16_t attrDataLen;
    uint8_t *dataPtr;
    uint8_t numAttr;
    uint8_t hdrLen;
    uint8_t dataType;
    uint16_t tempLen;
    uint8_t *tempBuf;
    ltlWriteCmd_t *writeCmd;
    ltlWriteRec_t *statusRec;
        
    tempBuf = pdata;
    tempLen = 0;
    numAttr = 0;
    // find out the number of attributes and the length of attribute data
    while ( tempBuf < ( pdata + datalen ) )
    {
        numAttr++;
        tempBuf += 2; // move pass attribute id

        dataType = *tempBuf++;// get data type and then move pass data type 

        attrDataLen = ltlGetAttrDataLength( dataType, tempBuf ); // get corresponding data length by data type 
        tempBuf += attrDataLen; // move pass attribute data

        // Padding needed if buffer has odd number of octects in length
        if ( attrDataLen % 2 ){
            attrDataLen++;
        }

        tempLen += attrDataLen;
    }

    // calculate the length of the response header
    hdrLen = sizeof( ltlWriteCmd_t ) + ( numAttr * sizeof( ltlWriteRec_t ) );
    
    writeCmd = (ltlWriteCmd_t *)mo_malloc( hdrLen + tempLen );
    if ( !writeCmd ){
        return (void *)NULL;
    }
    
    tempBuf = pdata;
    dataPtr = (uint8_t *)( (uint8_t *)writeCmd + hdrLen ); // data store after the hdr tail
    
    writeCmd->numAttr = numAttr;
    for ( i = 0; i < numAttr; i++ ){ 
        statusRec = &(writeCmd->attrList[i]);

        statusRec->attrID = BUILD_UINT16( tempBuf[0], tempBuf[1] ); // get attribute id
        tempBuf += 2;// move pass attribute id
        statusRec->dataType = *tempBuf++; // get data type and move pass data type 

        attrDataLen = ltlGetAttrDataLength( statusRec->dataType, tempBuf );// get corresponding data length by data type 
        memcpy( dataPtr, tempBuf, attrDataLen); // copy data to dst
        statusRec->attrData = dataPtr;

        tempBuf += attrDataLen; // move pass wirte attribute record and pointer next

        // advance attribute data pointer
        if ( attrDataLen % 2){
            attrDataLen++;
        }

        dataPtr += attrDataLen; // data store move nest datptr
    }
    
    return ( (void *)writeCmd );
}

/*********************************************************************
 * @brief   Parse the "Profile" Write Response Commands
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pCmd - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInWriteRspCmd( uint8_t *pdata, uint16_t datalen )
{
    ltlWriteRspCmd_t *writeRspCmd;
    uint8_t *pBuf = pdata;
    uint8_t i = 0;

    writeRspCmd = (ltlWriteRspCmd_t *)mo_malloc( sizeof ( ltlWriteRspCmd_t ) + datalen );
    if ( !writeRspCmd ){
        return (void *)NULL;
    }
    
    if ( datalen == 1 ) {
        // special case when all writes were successfull
        writeRspCmd->attrList[i++].status = *pBuf;
    }
    else{
        while ( pBuf < ( pdata + datalen ) )
        {
            writeRspCmd->attrList[i].status = *pBuf++;
            writeRspCmd->attrList[i++].attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
            pBuf += 2;
        }
    }
    writeRspCmd->numAttr = i;

    return ( (void *)writeRspCmd );
}
/*********************************************************************
 * @brief   Parse the "Profile" Configure Reporting Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInConfigReportCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlCfgReportCmd_t *cfgReportCmd;
    uint8_t *pBuf = pdata;
    uint8_t *dataPtr;
    uint8_t numAttr = 0;
    uint8_t dataType;
    uint8_t hdrLen;
    uint16_t dataLength = 0;
    uint8_t reportChangeLen; // length of Reportable Change field
    uint8_t i;
    ltlCfgReportRec_t *reportRec;
        
  // Calculate the length of the Request command
    while ( pBuf < ( pdata + datalen ) )
    {
        numAttr++;
        pBuf += 2; // move pass the attribute ID
        dataType = *pBuf++;
        pBuf += 2; // move pass the Min Reporting Intervals
  
        // For attributes of 'discrete' data types this field is omitted
        if ( ltlIsAnalogDataType( dataType ) ){
            reportChangeLen = ltlGetBaseDataTypeLength( dataType );
            pBuf += reportChangeLen;
    
            // add padding if needed
            if ( reportChangeLen % 2 ){
                reportChangeLen++;
            }
    
            dataLength += reportChangeLen;
        }
        else {
            pBuf++; // move past reportable change field
        }
    } // while loop

    hdrLen = sizeof( ltlCfgReportCmd_t ) + ( numAttr * sizeof( ltlCfgReportRec_t ) );

    cfgReportCmd = (ltlCfgReportCmd_t *)mo_malloc( hdrLen + dataLength );
    if ( !cfgReportCmd ){
        return (void *)NULL;
    }
    
    pBuf = pdata;
    dataPtr = (uint8_t *)( (uint8_t *)cfgReportCmd + hdrLen );

    cfgReportCmd->numAttr = numAttr;
    for ( i = 0; i < numAttr; i++ ) {
        reportRec = &(cfgReportCmd->attrList[i]);
        memset( reportRec, 0, sizeof( ltlCfgReportRec_t ) );
    
        reportRec->attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
        // Attribute to be reported
        reportRec->dataType = *pBuf++;
        reportRec->minReportInt = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
  
        // For attributes of 'discrete' data types this field is omitted
        if ( ltlIsAnalogDataType( reportRec->dataType ) ) {
            ltl_BuildAnalogData( reportRec->dataType, dataPtr, pBuf);
            reportRec->reportableChange = dataPtr;
    
            reportChangeLen = ltlGetBaseDataTypeLength( reportRec->dataType );
            pBuf += reportChangeLen;
    
            // advance attribute data pointer
            if ( reportChangeLen % 2 ){
                reportChangeLen++;
            }
    
            dataPtr += reportChangeLen;
        }
    } // while loop

  return ( (void *)cfgReportCmd );
}
/*********************************************************************
 * @brief   Parse the "Profile" Configure Reporting Response Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInConfigReportRspCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlCfgReportRspCmd_t *cfgReportRspCmd;
    uint8_t *pBuf = pdata;
    uint8_t numAttr;
    uint8_t i;
    
    numAttr = datalen / ( 1 + 2 ); // Status  + Attribute ID

    cfgReportRspCmd = (ltlCfgReportRspCmd_t *)mo_malloc( sizeof( ltlCfgReportRspCmd_t ) + ( numAttr * sizeof( ltlCfgReportStatus_t ) ) );
    if ( !cfgReportRspCmd ) {
        return (void *)NULL;
    }
    
    cfgReportRspCmd->numAttr = numAttr;
    for ( i = 0; i < cfgReportRspCmd->numAttr; i++ ) {
        cfgReportRspCmd->attrList[i].status = *pBuf++;
        cfgReportRspCmd->attrList[i].attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
    }
  
  return ( (void *)cfgReportRspCmd );
}
/*********************************************************************
 * @brief   Parse the "Profile" Read Reporting Configuration Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInReadReportCfgCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlReadReportCfgCmd_t *readReportCfgCmd;
    uint8_t *pBuf = pdata;
    uint8_t numAttr;
    uint8_t i;
    
    numAttr = datalen / 2; //  Attribute ID
  
    readReportCfgCmd = (ltlReadReportCfgCmd_t *)mo_malloc( sizeof( ltlReadReportCfgCmd_t ) + ( numAttr * sizeof( uint16_t ) ) );
    if ( !readReportCfgCmd ) {
        return (void *)NULL;
    }
    readReportCfgCmd->numAttr = numAttr;
    for ( i = 0; i < readReportCfgCmd->numAttr; i++) {
        readReportCfgCmd->attrID[i] = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
    }

  return ( (void *)readReportCfgCmd );
}
/*********************************************************************
 * @brief   Parse the "Profile" Read Reporting Configuration Response Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInReadReportCfgRspCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlReadReportCfgRspCmd_t *readReportCfgRspCmd;
    uint8_t reportChangeLen;
    uint8_t *pBuf = pdata;
    uint8_t *dataPtr;
    uint8_t numAttr = 0;
    uint8_t hdrLen;
    uint16_t dataLength = 0;
    uint8_t status;
    uint8_t dataType;
    uint8_t i;
    ltlReportCfgRspRec_t *reportRspRec;
    
    // Calculate the length of the response command
    while ( pBuf < ( pdata + datalen ) )
    {
        numAttr++;
        status = *pBuf++;
        pBuf += 2; // move pass the attribute ID
    
        if ( status == LTL_STATUS_SUCCESS ) {
            dataType = *pBuf++;
            pBuf += 2; // move pass the Min Reporting Intervals
    
            // For attributes of 'discrete' data types this field is omitted
            if ( ltlIsAnalogDataType( dataType ) ) {
                reportChangeLen = ltlGetBaseDataTypeLength( dataType );
                pBuf += reportChangeLen;
      
                // add padding if needed
                if ( reportChangeLen % 2 ) {
                    reportChangeLen++;
                }
      
                dataLength += reportChangeLen;
            }
        }
    } // while loop

    hdrLen = sizeof( ltlReadReportCfgRspCmd_t ) + ( numAttr * sizeof( ltlReportCfgRspRec_t ) );

    readReportCfgRspCmd = (ltlReadReportCfgRspCmd_t *)mo_malloc( hdrLen + dataLength );
    if ( !readReportCfgRspCmd ){
        return (void *)NULL;
    }
    
    pBuf = pdata;
    dataPtr = (uint8_t *)( (uint8_t *)readReportCfgRspCmd + hdrLen );

    readReportCfgRspCmd->numAttr = numAttr;
    for ( i = 0; i < numAttr; i++ ) {
        reportRspRec = &(readReportCfgRspCmd->attrList[i]);
  
        reportRspRec->status = *pBuf++;
        reportRspRec->attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
  
        if ( reportRspRec->status == LTL_STATUS_SUCCESS ){
             reportRspRec->dataType = *pBuf++;
             reportRspRec->minReportInt = BUILD_UINT16( pBuf[0], pBuf[1] );
             pBuf += 2;
             
             if ( ltlIsAnalogDataType( reportRspRec->dataType ) ) {
                 ltl_BuildAnalogData( reportRspRec->dataType, dataPtr, pBuf);
                 reportRspRec->reportableChange = dataPtr;
     
                 reportChangeLen = ltlGetBaseDataTypeLength( reportRspRec->dataType );
                 pBuf += reportChangeLen;
     
                 // advance attribute data pointer
                 if ( reportChangeLen % 2 ){
                    reportChangeLen++;
                 }
     
                 dataPtr += reportChangeLen;
             }
          }
    }

  return ( (void *)readReportCfgRspCmd );
}

/*********************************************************************
 * @brief   Parse the "Profile" Report Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInReportCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlReportCmd_t *reportCmd;
    uint8_t *pBuf = pdata;
    uint16_t attrDataLen;
    uint8_t *dataPtr;
    uint8_t numAttr = 0;
    uint8_t hdrLen;
    uint16_t dataLen = 0;
    uint8_t i;
    
    // find out the number of attributes and the length of attribute data
    while ( pBuf < ( pdata + datalen ) )
    {
        uint8_t dataType;

        numAttr++;
        pBuf += 2; // move pass attribute id

        dataType = *pBuf++;

        attrDataLen = ltlGetAttrDataLength( dataType, pBuf );
        pBuf += attrDataLen; // move pass attribute data

        // add padding if needed
        if ( attrDataLen % 2 ) {
            attrDataLen++;
        }

        dataLen += attrDataLen;
    }

    hdrLen = sizeof( ltlReportCmd_t ) + ( numAttr * sizeof( ltlReport_t ) );

    reportCmd = (ltlReportCmd_t *)mo_malloc( hdrLen + dataLen );
    if (!reportCmd ){
        return (void *)NULL;
    }

    pBuf = pdata;
    dataPtr = (uint8_t *)( (uint8_t *)reportCmd + hdrLen );

    reportCmd->numAttr = numAttr;
    for ( i = 0; i < numAttr; i++ )
    {
        ltlReport_t *reportRec = &(reportCmd->attrList[i]);

        reportRec->attrID = BUILD_UINT16( pBuf[0], pBuf[1] );
        pBuf += 2;
        reportRec->dataType = *pBuf++;

        attrDataLen = ltlGetAttrDataLength( reportRec->dataType, pBuf );
        memcpy( dataPtr, pBuf, attrDataLen );
        reportRec->attrData = dataPtr;

        pBuf += attrDataLen; // move pass attribute data

        // advance attribute data pointer
        if ( attrDataLen % 2 ){
            attrDataLen++;
        }

        dataPtr += attrDataLen;
    }

  return ( (void *)reportCmd );
}

/*********************************************************************
 * @brief   Parse the "Profile" Default Response Command
 *
 *      NOTE: THIS FUNCTION ALLOCATES THE RETURN BUFFER, SO THE CALLING
 *            FUNCTION IS RESPONSIBLE TO FREE THE MEMORY.
 *
 * @param   pdata - pointer to incoming data to parse
 * @param   datalen - pointer to incoming data to parse
 *
 * @return  pointer to the parsed command structure
 */
static void *ltlParseInDefaultRspCmd(uint8_t *pdata ,uint16_t datalen)
{
    ltlDefaultRspCmd_t *defaultRspCmd;
    uint8_t *pBuf = pdata;

    defaultRspCmd = (ltlDefaultRspCmd_t *)mo_malloc( sizeof ( ltlDefaultRspCmd_t ) );
    if ( defaultRspCmd != NULL ){
        defaultRspCmd->commandID = *pBuf++;
        defaultRspCmd->statusCode = *pBuf;
    }

    return ( (void *)defaultRspCmd );
}

/*********************************************************************
 * @brief   Process the "Profile" Read Command
 *
 * @param   ApduMsg - apdu message to process
 *
 * @return  TRUE if command processed. FALSE, otherwise.
 */
 static uint8_t ltlProcessInReadCmd(ltlApduMsg_t *ApduMsg)
{
    uint16_t len;
    uint8_t i;
    ltlAttrRec_t attrRec;
    ltlReadCmd_t *readCmd;
    ltlReadRspStatus_t *statusRec;
    ltlReadRspCmd_t *readRspCmd;

    readCmd = (ltlReadCmd_t *)ApduMsg->attrCmd;

    // calculate the length of the response status record
    len = sizeof( ltlReadRspCmd_t ) + (readCmd->numAttr * sizeof( ltlReadRspStatus_t ));

    readRspCmd = ( ltlReadRspCmd_t *)mo_malloc( len );
    if ( !readRspCmd ){
        return FALSE; // EMBEDDED RETURN
    }

    // 对每一个属性进行读处理,并准备好read response 
    readRspCmd->numAttr = readCmd->numAttr;
    for ( i = 0; i < readCmd->numAttr; i++ ){
        statusRec = &(readRspCmd->attrList[i]);

        statusRec->attrID = readCmd->attrID[i];

        // find this device attribute record
        if ( ltlFindAttrRec( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, readCmd->attrID[i], &attrRec ) ){ 
            if ( ltl_AccessCtrlRead( attrRec.accessControl ) ) {
               statusRec->status = LTL_STATUS_SUCCESS;
               statusRec->dataType = attrRec.dataType; // get the attribute data type
               statusRec->data = attrRec.dataPtr; // get the attribute pointer
            }
            else{
                statusRec->status = LTL_STATUS_WRITE_ONLY;
            }
        }
        else{
            statusRec->status = LTL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
    }

    // Build and send Read Response command
    ltl_SendReadRsp(ApduMsg->pkt->srcaddr,ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo,
                    ApduMsg->hdr.transSeqNum, readRspCmd);
    mo_free( readRspCmd );

    return TRUE;
}
 
/*********************************************************************
 * @brief   Process the "Profile" Write and Write No Response Commands
 *
 * @param   ApduMsg - apdu message to process
 *
 * @return  TRUE if command processed. FALSE, otherwise.
 */
static uint8_t ltlProcessInWriteCmd(ltlApduMsg_t *ApduMsg)
{
    uint8_t sendRsp = FALSE;
    uint8_t i,j;
    uint8_t status;
    ltlAttrRec_t attrRec;
    ltlWriteRec_t *statusRec;    
    ltlWriteCmd_t *writeCmd;
    ltlWriteRspCmd_t *writeRspCmd;
    
    writeCmd = (ltlWriteCmd_t *)ApduMsg->attrCmd;
    if ( ApduMsg->hdr.commandID == LTL_CMD_WRITE_ATTRIBUTES ) {
        // We need to send a response back - allocate space for it
        writeRspCmd = (ltlWriteRspCmd_t *)mo_malloc( sizeof( ltlWriteRspCmd_t ) + sizeof( ltlWriteRspStatus_t ) * writeCmd->numAttr );
        if ( !writeRspCmd ){
            return FALSE; // EMBEDDED RETURN
        }

        sendRsp = TRUE;
    }

    // 对每个属性进行处理,并做好应答准备
    for ( i = 0, j = 0; i < writeCmd->numAttr; i++ ){
        statusRec = &(writeCmd->attrList[i]);

        if ( ltlFindAttrRec(ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, statusRec->attrID, &attrRec )) {
            if ( statusRec->dataType == attrRec.dataType ) {
                // Write the new attribute value
                if ( attrRec.dataPtr != NULL ){
                    status = ltlWriteAttrData( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec, statusRec );
                }
                else {// Use CB
                    status = ltlWriteAttrDataUsingCB( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec, statusRec->attrData );
                }

                // If successful, a write attribute status record shall NOT be generated
                if ( sendRsp && status != LTL_STATUS_SUCCESS ){
                    // Attribute is read only - move on to the next write attribute record
                    writeRspCmd->attrList[j].status = status;
                    writeRspCmd->attrList[j].attrID = statusRec->attrID;
                    j++;
                }
            }
            else {
                // Attribute data type is incorrect 
                // move on to the next write attribute record
                if ( sendRsp ) {
                    writeRspCmd->attrList[j].status = LTL_STATUS_INVALID_DATA_TYPE;
                    writeRspCmd->attrList[j].attrID = statusRec->attrID;
                    j++;
                }
            }
        }
        else{
            // Attribute is not supported - move on to the next write attribute record
            if ( sendRsp ){
                writeRspCmd->attrList[j].status = LTL_STATUS_UNSUPPORTED_ATTRIBUTE;
                writeRspCmd->attrList[j].attrID = statusRec->attrID;
                j++;
            }
        }
    } // for loop

    if ( sendRsp ){
        writeRspCmd->numAttr = j;
        if ( writeRspCmd->numAttr == 0 ){
            // Since all records were written successful, include a single status record
            // in the resonse command with the status field set to SUCCESS and the
            // attribute ID field omitted.
            writeRspCmd->attrList[0].status = LTL_STATUS_SUCCESS;
            writeRspCmd->numAttr = 1;
        }

        ltl_SendWriteRsp(ApduMsg->pkt->srcaddr, ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo,ApduMsg->hdr.transSeqNum, writeRspCmd);
        mo_free( writeRspCmd );
    }

    return TRUE;
}
/*********************************************************************
 * @brief   Revert the "Profile" Write Undevided Command
 *
 * @param   ApduMsg - apdu message to process
 * @param   curWriteRec - old data
 * @param   numAttr - number of attributes to be reverted
 *
 * @return  none
 */
static void ltlRevertWriteUndividedCmd( ltlApduMsg_t *ApduMsg, ltlWriteRec_t *curWriteRec, uint16_t numAttr )
{
    uint8_t i;
    uint16_t dataLen;
    ltlAttrRec_t attrRec;
    ltlWriteRec_t *statusRec;

    for ( i = 0; i < numAttr; i++ ){
        statusRec = &(curWriteRec[i]);

        if ( !ltlFindAttrRec(ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, statusRec->attrID, &attrRec) ){
            break; // should never happen
        }

        if ( attrRec.dataPtr != NULL ){
            // Just copy the old data back - no need to validate the data
            dataLen = ltlGetAttrDataLength( attrRec.dataType, statusRec->attrData );
            memcpy( attrRec.dataPtr, statusRec->attrData, dataLen );
        }
        else { // Use CB
            // Write the old data back
            ltlWriteAttrDataUsingCB(ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec, (uint8_t *)statusRec->attrData);
        }
    } // for loop
}
/*********************************************************************
 * @brief   Process the "Profile" Write undivided Commands
 *
 * @param   ApduMsg - apdu message to process
 *
 * @return  TRUE if command processed. FALSE, otherwise.
 */
static uint8_t ltlProcessInWriteUndividedCmd(ltlApduMsg_t *ApduMsg)
{
    uint16_t dataLen;
    uint16_t curLen = 0;
    uint8_t i,j = 0;    
    ltlAttrRec_t attrRec;
    ltlWriteRec_t *statusRec; 
    ltlWriteCmd_t *writeCmd;
    ltlWriteRspCmd_t *writeRspCmd;
    
    uint8_t status;
    uint8_t hdrLen;
    uint8_t *curDataPtr;
    ltlWriteRec_t *curWriteRec;
    ltlWriteRec_t *curStatusRec;
    
    writeCmd = (ltlWriteCmd_t *)ApduMsg->attrCmd;

    // Allocate space for Write Response Command
    writeRspCmd = (ltlWriteRspCmd_t *)mo_malloc( sizeof( ltlWriteRspCmd_t ) + sizeof( ltlWriteRspStatus_t ) * writeCmd->numAttr );
    if ( !writeRspCmd ){
        return FALSE; // EMBEDDED RETURN
    }

    // If any attribute cannot be written, no attribute values are changed. Hence,
    // make sure all the attributes are supported and writable
    for ( i = 0; i < writeCmd->numAttr; i++ ) {
        statusRec = &(writeCmd->attrList[i]);

        if ( !ltlFindAttrRec(ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, statusRec->attrID, &attrRec ) ){
            // Attribute is not supported - stop here
            writeRspCmd->attrList[j].status = LTL_STATUS_UNSUPPORTED_ATTRIBUTE;
            writeRspCmd->attrList[j++].attrID = statusRec->attrID;
            break;
        }

        if ( statusRec->dataType != attrRec.dataType ){
            // Attribute data type is incorrect - stope here
            writeRspCmd->attrList[j].status =LTL_STATUS_INVALID_DATA_TYPE;
            writeRspCmd->attrList[j++].attrID = statusRec->attrID;
            break;
        }

        if ( !ltl_AccessCtrlWrite( attrRec.accessControl ) ){
            // Attribute is not writable - stop here
            writeRspCmd->attrList[j].status = LTL_STATUS_READ_ONLY;
            writeRspCmd->attrList[j++].attrID = statusRec->attrID;
            break;
        }

        // Attribute Data length
        if ( attrRec.dataPtr != NULL ){
            dataLen = ltlGetAttrDataLength(attrRec.dataType, attrRec.dataPtr);
        }
        else { // Use CB
            dataLen = ltlGetAttrDataLengthUsingCB( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, attrRec.attrId);
        }

        // add padding if needed
        if ( dataLen % 2 ){
            dataLen++;
        }

        curLen += dataLen;
    } // for loop

    writeRspCmd->numAttr = j;
    if ( writeRspCmd->numAttr == 0 ) { // All attributes can be written

        // calculate the length of the current data header
        hdrLen = writeCmd->numAttr * sizeof( ltlWriteRec_t );

        // Allocate space to keep a copy of the current data
        curWriteRec = (ltlWriteRec_t *) mo_malloc( hdrLen + curLen );
        if ( !curWriteRec ){
            mo_free( writeRspCmd );
            return FALSE; // EMBEDDED RETURN
        }

        curDataPtr = (uint8_t *)((uint8_t *)curWriteRec + hdrLen);// data store after the hdr tail

        // Write the new data over
        for ( i = 0; i < writeCmd->numAttr; i++ ){
            statusRec = &(writeCmd->attrList[i]);
            curStatusRec = &(curWriteRec[i]);

            if ( !ltlFindAttrRec(ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, statusRec->attrID, &attrRec )){
                break; // should never happen
            }

            // Keep a copy of the current data before before writing the new data over
            curStatusRec->attrID = statusRec->attrID;
            curStatusRec->attrData = curDataPtr;

            if ( attrRec.dataPtr != NULL ){
                // Read the current value and keep a copy
                dataLen = ltlGetAttrDataLength( attrRec.dataType, (uint8_t *)(attrRec.dataPtr) );
                memcpy( curDataPtr, attrRec.dataPtr, dataLen );

                // Write the new attribute value
                status = ltlWriteAttrData( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec, statusRec );
            }
            else { // Use CBs
                // Read the current value and keep a copy
                ltlReadAttrDataUsingCB( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, statusRec->attrID, curDataPtr, &dataLen );
                // Write the new attribute value
                status = ltlWriteAttrDataUsingCB( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec, statusRec->attrData );
            }

            // If successful, a write attribute status record shall NOT be generated
            if ( status != LTL_STATUS_SUCCESS ){
                writeRspCmd->attrList[j].status = status;
                writeRspCmd->attrList[j++].attrID = statusRec->attrID;

                // Since this write failed, we need to revert all the pervious writes
                ltlRevertWriteUndividedCmd( ApduMsg, curWriteRec, i);
                break;
            }

            // add padding if needed
            if ( dataLen % 2){
                dataLen++;
            }

            curDataPtr += dataLen; // move next
        } // for loop

        writeRspCmd->numAttr = j;
        if ( writeRspCmd->numAttr  == 0 ){
            // Since all records were written successful, include a single status record
            // in the resonse command with the status field set to SUCCESS and the
            // attribute ID field omitted.
            writeRspCmd->attrList[0].status = LTL_STATUS_SUCCESS;
            writeRspCmd->numAttr = 1;
        }

        mo_free( curWriteRec );
    }

    ltl_SendWriteRsp(ApduMsg->pkt->srcaddr, ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo,ApduMsg->hdr.transSeqNum,  writeRspCmd);
    mo_free( writeRspCmd );

    return TRUE;
}


//处理进来的apdu帧,本函数只处理APDU
void ltl_ProcessInApdu(MoIncomingMsgPkt_t *pkt)
{
    LStatus_t status = LTL_STATUS_SUCCESS;
    ltlApduMsg_t ApduMsg;
    ltlLibPlugin_t *pInPlugin;
    ltlDefaultRspCmd_t defaultRspCmd;

    //error no apdu payload
    if(pkt->apduLength < ltlHdrSize()){
        return;
    }

    //parse frame head
    ApduMsg.pkt = pkt;
    ApduMsg.pdata = ltlParseHdr(&(ApduMsg.hdr),pkt->apduData);
    ApduMsg.datalen = pkt->apduLength;
    ApduMsg.datalen -= (uint16_t)(ApduMsg.pdata - pkt->apduData); // 通过指针偏移计算apdu帧头数据长度
    
    // process frame head    
    //foundation type message in profile 标准命令
    if(ltl_IsProfileCmd(ApduMsg.hdr.fc.type)){
        if((ApduMsg.hdr.commandID < LTL_CMD_PROFILE_MAX) && (ltlCmdTable[ApduMsg.hdr.commandID].pfnParseInProfile != NULL)){
            // parse foundation corresponding command 
            ApduMsg.attrCmd = ltlCmdTable[ApduMsg.hdr.commandID].pfnParseInProfile(ApduMsg.pdata,ApduMsg.datalen);
            
            if((ApduMsg.attrCmd != NULL) && (ltlCmdTable[ApduMsg.hdr.commandID].pfnProcessInProfile != NULL)){
                //and then process corresponding appliction data domain
                if(ltlCmdTable[ApduMsg.hdr.commandID].pfnProcessInProfile(&ApduMsg) == FALSE){
                    // can not find attribute in the table
                }
            }

            if( ApduMsg.attrCmd ) //because parseInProfile may be alloc memory
                mo_free(ApduMsg.attrCmd);

            if(LTL_PROFILE_CMD_HAS_RSP(ApduMsg.hdr.commandID))
                return; // we are done in ltlProcessCmd()
        }
        else {
            status = LTL_STATUS_UNSUP_GENERAL_COMMAND;
        }
    }
    // Not a foundation type message, is a specific trunk command // 特殊集下的命令
    else{ //ltl_IsTrunkCmd(ApduMsg.hdr.fc.type) 
        // it is specific to the trunk id command
        //find trunk id 
        pInPlugin = ltlFindPlugin(ApduMsg.hdr.trunkID);
        if(pInPlugin && pInPlugin->pfnSpecificTrunkHdlr){
                  // The return value of the plugin function will be
            //  LTL_STATUS_SUCCESS - Supported and need default response
            //  LTL_STATUS_FAILURE - Unsupported
            //  LTL_STATUS_CMD_HAS_RSP - Supported and do not need default rsp
            //  LTL_STATUS_INVALID_FIELD - Supported, but the incoming msg is wrong formatted
            //  LTL_STATUS_INVALID_VALUE - Supported, but the request not achievable by the h/w
            //  LTL_STATUS_MEMERROR - Supported but memory allocation fails
            status = pInPlugin->pfnSpecificTrunkHdlr(&ApduMsg);
            if(status == LTL_STATUS_CMD_HAS_RSP)
                return; // we are done in pfnSpecificTrunkHdlr
        }

    }

    if(!ApduMsg.pkt->isbroadcast && 
        (!ApduMsg.hdr.fc.disableDefaultRsp) &&
        ltl_IsFromClient(ApduMsg.hdr.fc.dir) &&
        ApduMsg.hdr.transSeqNum){
        defaultRspCmd.commandID = ApduMsg.hdr.commandID;
        defaultRspCmd.statusCode = status;
        // send default response
        ltl_SendDefaultRsp(pkt->srcaddr, ApduMsg.hdr.trunkID, ApduMsg.hdr.nodeNo, ApduMsg.hdr.transSeqNum,  &defaultRspCmd);
    }
}

/*********************************************************************
 * @brief   string to app string (app string format: len | string)
 *
 * @param   pRawStr - in -- pointer to the ascii Raw string buffer
 * @param   pAppStr - out -- pointer to the ascii app string buffer
 * @param   Applen  -- App buffer length
 *
 * @return  
 */
void ltl_StrToAppString(char *pRawStr, char *pAppStr, uint8_t Applen )
{
    uint8_t rawlen;
    
    rawlen = strlen(pRawStr);
    rawlen = MIN(rawlen, (Applen - OCTET_CHAR_HEADROOM_LEN));

    *pAppStr++ = rawlen; // for length    
    memcpy(pAppStr, pRawStr, rawlen);
}



