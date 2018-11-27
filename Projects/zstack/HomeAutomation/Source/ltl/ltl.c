

#include "ltl.h"
#include "memalloc.h"
#include "prefix.h"



/*********************************************************************
 * MACROS
 */
/*** Frame Control ***/
#define ltl_FCType( a )               ( (a) & LTL_FRAMECTL_TYPE_MASK )
#define ltl_FCDirection( a )          ( (a) & LTL_FRAMECTL_DIR_MASK)
#define ltl_FCDisableDefaultRsp( a )  ( (a) & LTL_FRAMECTL_DISALBE_DEFAULT_RSP_MASK )

/*** Attribute Access Control ***/
#define ltl_AccessCtrlRead( a )       ( (a) & ACCESS_CONTROL_READ )
#define ltl_AccessCtrlWrite( a )      ( (a) & ACCESS_CONTROL_WRITE )
#define ltl_AccessCtrlCmd( a )        ( (a) & ACCESS_CONTROL_COMMAND )
#define ltl_AccessCtrlAuthRead( a )   ( (a) & ACCESS_CONTROL_AUTH_READ )
#define ltl_AccessCtrlAuthWrite( a )  ( (a) & ACCESS_CONTROL_AUTH_WRITE )

// Commands that have corresponding responses
#define LTL_PROFILE_CMD_HAS_RSP( cmd )  ( (cmd) == LTL_CMD_READ_ATTRIBUTES            || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES           || \
                                        (cmd) == LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED || \
                                        (cmd) == LTL_CMD_CONFIGURE_REPORTING   || \
                                        (cmd) == LTL_CMD_READ_CONFIGURE_REPORTING || \
                                        (cmd) == LTL_CMD_DEFAULT_RSP ) // exception

/* local typedef */

typedef void *(*ltlParseInProfileCmd_t)( uint8_t *pdata,uint16_t datalen );
typedef uint8_t (*ltlProcessInProfileCmd_t)( ltlApduMsg_t *ApduMsg );

//typedef uint16_t (*ltlprefixsizefn_t)(uint8_t *pAddr);
//typedef uint8_t *(*ltlprefixBuildHdrfn_t)(uint8_t *pAddr,uint8_t *pbuf);

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
typedef struct ltlAttrRecsList_s
{
    uint16_t            trunkID;      // Used to link it into the trunk descriptor
    uint8_t             nodeNO; // Number of the following records
    uint8_t             numAttributes;
    const ltlAttrRec_t  *attrs;        // attribute record
    ltlReadWriteCB_t    pfnReadWriteCB;// Read or Write attribute value callback function
    ltlAuthorizeCB_t    pfnAuthorizeCB;// Authorize Read or Write operation   
    void *next;
} ltlAttrRecsList_t;


// local function
static uint8_t *ltlSerializeData( uint8_t dataType, void *attrData, uint8_t *buf );
static uint8_t *ltlParseHdr(ltlFrameHdr_t *hdr,uint8_t *pDat);

static void ltlEncodeHdr( ltlFrameHdr_t *hdr,uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, 
                                uint8_t specific, uint8_t direction, uint8_t disableDefaultRsp,uint8_t cmd);
static uint8_t *ltlBuildHdr( ltlFrameHdr_t *hdr, uint8_t *pDat );
static uint8_t ltlHdrSize(ltlFrameHdr_t *hdr);
static ltlLibPlugin_t *ltlFindPlugin( uint16_t trunkID );
static ltlAttrRecsList_t *ltlFindAttrRecsList(uint16_t trunkID, uint8_t nodeNO);

static ltlReadWriteCB_t ltlGetReadWriteCB(uint16_t trunkID, uint8_t nodeNO);
static LStatus_t ltlAuthorizeReadUsingCB(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr );
static LStatus_t ltlWriteAttrData(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr, ltlWriteRec_t *pWriteRec );
static LStatus_t ltlWriteAttrDataUsingCB( uint16_t trunkID,uint8_t nodeNO, ltlAttrRec_t *pAttr, uint8_t *pAttrData );
static LStatus_t ltlAuthorizeWriteUsingCB(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr );


static void *ltlParseInReadCmd( uint8_t *pdata,uint16_t datalen );
static void *ltlParseInWriteCmd(uint8_t *pdata,uint16_t datalen);

static uint8_t ltlProcessInReadCmd(ltlApduMsg_t *ApduMsg);
static uint8_t ltlProcessInWriteCmd(ltlApduMsg_t *ApduMsg);
static uint8_t ltlProcessInWriteUndividedCmd(ltlApduMsg_t *ApduMsg);

// local variable
static ltlLibPlugin_t *libplugins = NULL;
static ltlAttrRecsList_t *attrList = NULL;

static ltlCmdItems_t ltlCmdTable[] = 
{
    {ltlParseInReadCmd, ltlProcessInReadCmd},           // LTL_CMD_READ_ATTRIBUTES
    {NULL, NULL},                                       //LTL_CMD_READ_ATTRIBUTES_RSP
    {ltlParseInWriteCmd, ltlProcessInWriteCmd},         //LTL_CMD_WRITE_ATTRIBUTES
    {ltlParseInWriteCmd, ltlProcessInWriteUndividedCmd},//LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED
    {NULL, NULL},                                       //LTL_CMD_WRITE_ATTRIBUTES_RSP
    {ltlParseInWriteCmd, ltlProcessInWriteCmd},         //LTL_CMD_WRITE_ATTRIBUTES_NORSP
    {NULL, NULL},                           //LTL_CMD_CONFIGURE_REPORTING
    {NULL, NULL},                           //LTL_CMD_CONFIGURE_REPORTING_RSP
    {NULL, NULL},                           //LTL_CMD_READ_CONFIGURE_REPORTING
    {NULL, NULL},                           //LTL_CMD_READ_CONFIGURE_REPORTING_RSP
    {NULL, NULL},                           //LTL_CMD_REPORT_ATTRIBUTES
    {NULL, NULL},                           //LTL_CMD_DEFAULT_RSP                          
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
LStatus_t ltl_registerPlugin(uint16_t starttrunkID,uint16_t endtrunkID,ltlSpecificTrunckHdCB_t pfnSpecificTrunkHdCB)
{
    ltlLibPlugin_t *pNewItem;
    ltlLibPlugin_t *pLoop;

    // Fill in the new profile list
    pNewItem = mo_malloc( sizeof( ltlLibPlugin_t ) );
    if ( pNewItem == NULL ){
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
    if ( pNewItem == NULL){
        return LTL_MEMERROR;
    }

    pNewItem->trunkID = trunkID;
    pNewItem->nodeNO = nodeNO;
    pNewItem->numAttributes = numAttr;
    pNewItem->attrs = newAttrList;
    pNewItem->pfnReadWriteCB = NULL;
    pNewItem->pfnAuthorizeCB = NULL;
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
 *              Note: The pfnAuthorizeCB callback function is only required
 *                    when the Read/Write operation on an attribute requires
 *                    authorization (i.e., attributes with ACCESS_CONTROL_AUTH_READ
 *                    or ACCESS_CONTROL_AUTH_WRITE access permissions).
 *              授权回调,对
 *
 * @param       pfnReadWriteCB - function pointer to read/write routine
 * @param       pfnAuthorizeCB - function pointer to authorize read/write operation
 *
 * @return      LTL_SUCCESS if successful. LTL_FAILURE, otherwise.
 */
LStatus_t ltl_registerReadWriteCB(uint16_t trunkID, uint8_t nodeNO, 
                                ltlReadWriteCB_t pfnReadWriteCB, ltlAuthorizeCB_t pfnAuthorizeCB )
{
    ltlAttrRecsList_t *pRec = ltlFindAttrRecsList(trunkID, nodeNO);

    if ( pRec != NULL ){
        pRec->pfnReadWriteCB = pfnReadWriteCB;
        pRec->pfnAuthorizeCB = pfnAuthorizeCB;

        return ( LTL_SUCCESS );
    }

    return ( LTL_FAILURE );
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
                                uint8_t specific, uint8_t direction, uint8_t disableDefaultRsp,
                                uint8_t cmd, uint8_t *cmdFormat,uint16_t cmdFormatLen)
{
    ltlFrameHdr_t hdr;
    uint8_t *msgbuf;
    uint8_t *pbuf;
    uint16_t msglen;
    uint16_t prefixlen;
    LStatus_t status;
    
    
    memset((uint8_t *)&hdr,0,sizeof(ltlFrameHdr_t));
    
    ltlEncodeHdr(&hdr, trunkID, nodeNO, seqNum, specific, direction, disableDefaultRsp, cmd);

    msglen = ltlHdrSize(&hdr);

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

    status = ltlPrefixrequest(dstAddr,msgbuf, msglen);
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
    hdr->fc.direction = ltl_FCDirection(*pDat) ? LTL_FRAMECTL_DIR_SERVER_CLIENT : LTL_FRAMECTL_DIR_CLIENT_SERVER;
    hdr->fc.disableDefaultRsp = ltl_FCDisableDefaultRsp(*pDat) ? LTL_FRAMECTL_DIS_DEFAULT_RSP_ON : LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF;
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
 * @param   manuCode -
 * @param   disableDefaultRsp -
 * @param   cmd -
 *
 * @return  no
 */
static void ltlEncodeHdr( ltlFrameHdr_t *hdr,uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, 
                                uint8_t specific, uint8_t direction, uint8_t disableDefaultRsp,uint8_t cmd)
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
    hdr->fc.direction = direction ? LTL_FRAMECTL_DIR_SERVER_CLIENT : LTL_FRAMECTL_DIR_CLIENT_SERVER;
    hdr->fc.disableDefaultRsp=  disableDefaultRsp ? LTL_FRAMECTL_DIS_DEFAULT_RSP_ON : LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF;
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
    *pDat |= hdr->fc.direction << 2;
    *pDat |= hdr->fc.disableDefaultRsp << 3;
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
static uint8_t ltlHdrSize(ltlFrameHdr_t *hdr)
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
/*********************************************************************
 * @brief   Find the base data type length that matchs the dataType
 *
 * @param   dataType - data type
 *
 * @return  length of data type
 */
uint8_t ltlGetDataTypeLength( uint8_t dataType )
{
    uint8_t len;

    switch ( dataType ){
        case LTL_DATATYPE_DATA8:
        case LTL_DATATYPE_BOOLEAN:
        case LTL_DATATYPE_BITMAP8:
        case LTL_DATATYPE_UINT8:
        case LTL_DATATYPE_INT8:
        case LTL_DATATYPE_ENUM8:
            len = 1;
            break;

        case LTL_DATATYPE_DATA16:
        case LTL_DATATYPE_BITMAP16:
        case LTL_DATATYPE_UINT16:
        case LTL_DATATYPE_INT16:
        case LTL_DATATYPE_ENUM16:
        case LTL_DATATYPE_TRUNK_ID:
        case LTL_DATATYPE_ATTR_ID:
            len = 2;
            break;

        case LTL_DATATYPE_DATA32:
        case LTL_DATATYPE_BITMAP32:
        case LTL_DATATYPE_UINT32:
        case LTL_DATATYPE_INT32:
        case LTL_DATATYPE_SINGLE_PREC:
            len = 4;
            break;
        case LTL_DATATYPE_DATA64:
        case LTL_DATATYPE_BITMAP64:           
        case LTL_DATATYPE_UINT64:
        case LTL_DATATYPE_INT64:
        case LTL_DATATYPE_DOUBLE_PREC:
            len = 8;
            break;
        
        case LTL_DATATYPE_SN_ADDR:
        case LTL_DATATYPE_128_BIT_SEC_KEY:
            len = 16;
            break;

        case LTL_DATATYPE_NO_DATA:
        case LTL_DATATYPE_UNKNOWN:
        // Fall through

        default:
            len = 0;
        break;
    }

    return ( len );
}

/*********************************************************************
 * @brief   Return the length of the attribute. 
 *
 * @param   dataType - data type
 * @param   pData - in ---- pointer to data 
 *
 * @return  returns atrribute length
 */
uint16_t ltlGetAttrDataLength( uint8_t dataType, uint8_t *pData )
{
    if ( dataType == LTL_DATATYPE_LONG_CHAR_STR || dataType == LTL_DATATYPE_LONG_OCTET_ARRAY ) {
        return ( BUILD_UINT16( pData[0], pData[1] ) + 2); // long string length + 2 for length field
    }
    else if ( dataType == LTL_DATATYPE_CHAR_STR || dataType == LTL_DATATYPE_OCTET_ARRAY ){
        return ( *pData + 1 ); // string length + 1 for length field
    }else if(dataType == LTL_DATATYPE_ARRAY){
        // TODO: do later
    }

    return ltlGetDataTypeLength( dataType );
}
/*********************************************************************
 * @brief   Read the attribute's current value into pAttrData.
 *
 * @param   pAttr - in --- pointer to attribute
 * @param   pAttrData - out -- where to put attribute data
 * @param   pDataLen - out -- where to put attribute data length
 *
 * @return Success
 */
uint8_t ltlReadAttrData(ltlAttrRec_t *pAttr, uint8_t *pAttrData, uint16_t *pDataLen )
{
    uint16_t dataLen;

    dataLen = ltlGetAttrDataLength( pAttr->dataType, (uint8_t *)(pAttr->dataPtr) );
    memcpy( pAttrData, pAttr->dataPtr, dataLen );

    if ( pDataLen != NULL ) {
        *pDataLen = dataLen;
    }

    return ( LTL_STATUS_SUCCESS );
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
        case LTL_DATATYPE_DATA8:
        case LTL_DATATYPE_BOOLEAN:
        case LTL_DATATYPE_BITMAP8:
        case LTL_DATATYPE_INT8:
        case LTL_DATATYPE_UINT8:
        case LTL_DATATYPE_ENUM8:
            *buf++ = *((uint8_t *)attrData);
            break;

        case LTL_DATATYPE_DATA16:
        case LTL_DATATYPE_BITMAP16:
        case LTL_DATATYPE_UINT16:
        case LTL_DATATYPE_INT16:
        case LTL_DATATYPE_ENUM16:
        case LTL_DATATYPE_TRUNK_ID:
        case LTL_DATATYPE_ATTR_ID:
            *buf++ = LO_UINT16( *((uint16_t *)attrData) );
            *buf++ = HI_UINT16( *((uint16_t *)attrData) );
            break;

        case LTL_DATATYPE_DATA32:
        case LTL_DATATYPE_BITMAP32:
        case LTL_DATATYPE_UINT32:
        case LTL_DATATYPE_INT32:
        case LTL_DATATYPE_SINGLE_PREC:
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 0 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 1 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 2 );
            *buf++ = BREAK_UINT32( *((uint32_t *)attrData), 3 );
            break;

        case LTL_DATATYPE_DATA64:
        case LTL_DATATYPE_BITMAP64:           
        case LTL_DATATYPE_UINT64:
        case LTL_DATATYPE_INT64:
        case LTL_DATATYPE_DOUBLE_PREC:
            pStr = (uint8_t *)attrData;
            buf = memcpy( buf, pStr, 8 );
        /*
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 0 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 1 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 2 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 3 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 4 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 5 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 6 );
            *buf++ = BREAK_UINT64( *((uint64_t *)attrData), 7 );
        */
            break;

        case LTL_DATATYPE_CHAR_STR:
        case LTL_DATATYPE_OCTET_ARRAY:
            pStr = (uint8_t *)attrData;
            len = *pStr;
            buf = memcpy( buf, pStr, len + 1 ); // Including length field
            break;

        case LTL_DATATYPE_LONG_CHAR_STR:
        case LTL_DATATYPE_LONG_OCTET_ARRAY:
            pStr = (uint8_t *)attrData;
            len = BUILD_UINT16( pStr[0], pStr[1] );
            buf = memcpy( buf, pStr, len + 2 ); // Including length field
            break;
            
        case LTL_DATATYPE_SN_ADDR:
        case LTL_DATATYPE_128_BIT_SEC_KEY:
            
            pStr = (uint8_t *)attrData;
            buf = memcpy( buf, pStr, 16 );
            break;

        case LTL_DATATYPE_NO_DATA:
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
 * @brief   Get the Read/Write Authorization callback function pointer
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 *
 * @return  Authorization CB, NULL if not found
 */
static ltlAuthorizeCB_t ltlGetAuthorizeCB(uint16_t trunkID, uint8_t nodeNO)
{
    ltlAttrRecsList_t *pRec = ltlFindAttrRecsList( trunkID, nodeNO );

    if ( pRec != NULL ){
        return ( pRec->pfnAuthorizeCB );
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

    if ( pfnReadWriteCB != NULL ){
        // Only get the attribute length
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

    if ( pDataLen != NULL ){
        *pDataLen = 0; // Always initialize it to 0
    }

    if ( pfnReadWriteCB != NULL ){
        // Read the attribute value and its length
        return ( (*pfnReadWriteCB)( trunkID, nodeNO, attrId, LTL_OPER_READ, pAttrData, pDataLen ) );
    }

    return ( LTL_STATUS_SOFTWARE_FAILURE );
}
/*********************************************************************
 * @brief   Use application's callback to authorize a Read operation
 *          on a given attribute.
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   pAttr - out -- pointer to attribute record
 *
 * @return  LTL_STATUS_SUCCESS: Operation authorized
 *          LTL_STATUS_NOT_AUTHORIZED: Operation not authorized
 */
static LStatus_t ltlAuthorizeReadUsingCB(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr )
{
    ltlAuthorizeCB_t pfnAuthorizeCB;

    if ( ltl_AccessCtrlAuthRead( pAttr->accessControl ) ){
        pfnAuthorizeCB = ltlGetAuthorizeCB( trunkID, nodeNO );

        if ( pfnAuthorizeCB != NULL ){
            return ( (*pfnAuthorizeCB)(pAttr, LTL_OPER_READ ) );
        }
    }

    return ( LTL_STATUS_SUCCESS );
}
/*********************************************************************
 * @brief   Write the attribute's current value into pAttrData.
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   pAttr - in --- pointer to attribute,  where to write data to 
 * @param   pWriteRec - in -- pointer to attribute ,data to be written
 *
 * @return 
 */
static LStatus_t ltlWriteAttrData(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr, ltlWriteRec_t *pWriteRec )
{
    uint8_t status;
    uint16_t len;

    if ( ltl_AccessCtrlWrite( pAttr->accessControl ) ){
        status = ltlAuthorizeWriteUsingCB( trunkID, nodeNO, pAttr );
        if ( status == LTL_STATUS_SUCCESS ){
//            if ( ( ltl_ValidateAttrDataCB == NULL ) || ltl_ValidateAttrDataCB( pAttr, pWriteRec ) ){
                // Write the attribute value
               len = ltlGetAttrDataLength(pAttr->dataType, pWriteRec->attrData);
               memcpy( pAttr->dataPtr, pWriteRec->attrData, len );

//                status = LTL_STATUS_SUCCESS;
//            }
//            else{
//                  status = LTL_STATUS_INVALID_VALUE;
//            }
        }
    }
    else{
        status = LTL_STATUS_READ_ONLY;
    }

    return ( status );
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
static LStatus_t ltlWriteAttrDataUsingCB( uint16_t trunkID,uint8_t nodeNO, ltlAttrRec_t *pAttr, uint8_t *pAttrData )
{
    uint8_t status;
    ltlReadWriteCB_t pfnReadWriteCB;

    if ( ltl_AccessCtrlWrite( pAttr->accessControl ) ){
        status = ltlAuthorizeWriteUsingCB( trunkID, nodeNO, pAttr );
        if ( status == LTL_STATUS_SUCCESS ){
          pfnReadWriteCB = ltlGetReadWriteCB( trunkID, nodeNO);
          if ( pfnReadWriteCB != NULL ){
                // Write the attribute value
                status = (*pfnReadWriteCB)( trunkID, nodeNO, pAttr->attrId, LTL_OPER_WRITE, pAttrData, NULL );
          }
          else{
            status = LTL_STATUS_SOFTWARE_FAILURE;
          }
        }
    }
    else{
        status = LTL_STATUS_READ_ONLY;
    }

    return ( status );
}

/*********************************************************************
 * @brief   Use application's callback to authorize a Write operation
 *          on a given attribute.
 *
 * @param   trunkID - 
 * @param   nodeNO - 
 * @param   pAttr - in -- pointer to attribute record
 *
 * @return  LTL_STATUS_SUCCESS: Operation authorized
 *          LTL_STATUS_NOT_AUTHORIZED: Operation not authorized
 */
static LStatus_t ltlAuthorizeWriteUsingCB(uint16_t trunkID, uint8_t nodeNO, ltlAttrRec_t *pAttr )
{
    ltlAuthorizeCB_t pfnAuthorizeCB;
    
    if ( ltl_AccessCtrlAuthWrite( pAttr->accessControl ) ) {
        pfnAuthorizeCB = ltlGetAuthorizeCB( trunkID, nodeNO);

        if ( pfnAuthorizeCB != NULL ){
            return ( (*pfnAuthorizeCB)(pAttr, LTL_OPER_WRITE ) );
        }
    }

    return ( LTL_STATUS_SUCCESS );
}

LStatus_t ltl_SendReadReq(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO,
                                uint8_t seqNum,uint8_t direction, 
                                uint8_t disableDefaultRsp, ltlReadCmd_t *readCmd )
{
    uint8_t i;
    uint16_t dataLen;
    uint8_t *buf;
    uint8_t *pBuf;
    LStatus_t status;

    dataLen = readCmd->numAttr * 2; // Attribute ID

    buf = mo_malloc( dataLen );
    if ( buf != NULL ){
        // Load the buffer - serially
        pBuf = buf;
        for (i = 0; i < readCmd->numAttr; i++) {
            *pBuf++ = LO_UINT16( readCmd->attrID[i] );
            *pBuf++ = HI_UINT16( readCmd->attrID[i] );
        }

        status = ltl_SendCommand(dstAddr, trunkID, nodeNO,seqNum, 
                                FALSE, direction, disableDefaultRsp, 
                                LTL_CMD_READ_ATTRIBUTES, buf,  dataLen);
        mo_free( buf );
    }
    else{
        status = LTL_MEMERROR;
    }

    return ( status );
}


LStatus_t ltl_SendReadRsp(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO,
                                uint8_t seqNum,uint8_t direction,
                                uint8_t disableDefaultRsp, ltlReadRspCmd_t *readRspCmd )
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
    if ( buf == NULL ){
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
                            FALSE, direction, disableDefaultRsp,
                            LTL_CMD_READ_ATTRIBUTES_RSP, buf, len);
              
    mo_free( buf );

    return ( status );
}
LStatus_t ltl_SendWriteRequest(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO,
                                uint8_t seqNum,uint8_t direction,
                                uint8_t disableDefaultRsp, uint8_t cmd, ltlWriteCmd_t *writeCmd )
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
    if ( buf != NULL ){
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
                                FALSE, direction, disableDefaultRsp,
                                cmd, buf, dataLen);
        mo_free( buf );
    }
    else{
        status = LTL_MEMERROR;
    }

    return ( status);
}
LStatus_t ltl_SendReportCmd( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                 uint8_t seqNum , uint8_t direction, 
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
    if ( buf == NULL ){
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
                            FALSE, direction, disableDefaultRsp,
                            LTL_CMD_REPORT_ATTRIBUTES, buf, dataLen);
    mo_free( buf );

    return ( status );
}
LStatus_t ltl_SendWriteRsp(uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                 uint8_t seqNum , uint8_t direction,
                                 uint8_t disableDefaultRsp, ltlWriteRspCmd_t *writeRspCmd)
{
    uint8_t i;
    uint16_t len = 0;
    uint8_t *pbuf;
    uint8_t *buf;
    LStatus_t status;
    ltlWriteRspStatus_t *statusRec;

    // calculate the size of the command
    len = writeRspCmd->numAttr * sizeof(1 + 2); //status + attribute id
    
    buf = mo_malloc( len );
    if ( buf == NULL ){
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
                            FALSE, direction, disableDefaultRsp,
                            LTL_CMD_WRITE_ATTRIBUTES_RSP, buf, len);              
    mo_free( buf );

    return status;
}

//ok
LStatus_t ltl_SendDefaultRspCmd( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,
                                uint8_t seqNum, uint8_t direction,
                                uint8_t disableDefaultRsp, ltlDefaultRspCmd_t *defaultRspCmd)
{
  uint8_t buf[2]; // Command ID and Status;

  // Load the buffer - serially
  buf[0] = defaultRspCmd->commandID;
  buf[1] = defaultRspCmd->statusCode;

  return (ltl_SendCommand(dstAddr, trunkID, nodeNO, seqNum, FALSE, direction, disableDefaultRsp,
                    LTL_CMD_DEFAULT_RSP, buf, 2));
}

/*********************************************************************
 * @fn      
 *
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
    if ( readCmd == NULL ){
        return NULL;
   }
    
    readCmd->numAttr = datalen / 2; // Atrribute ID number
    for ( i = 0; i < readCmd->numAttr; i++ ){
        readCmd->attrID[i] = BUILD_UINT16( pdata[0], pdata[1] );
        pdata += 2;
    }

    return ( (void *)readCmd );
}
/*********************************************************************
 * @fn      
 *
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
    if ( writeCmd == NULL ){
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
 * @fn      
 *
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
    if ( readRspCmd == NULL ){
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
                // check user authorize
                statusRec->status = ltlAuthorizeReadUsingCB( ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo, &attrRec );
                if ( statusRec->status == LTL_STATUS_SUCCESS ){
                    statusRec->data = attrRec.dataPtr; // get the attribute pointer
                    statusRec->dataType = attrRec.dataType; // get the attribute data type
                }
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
                    ApduMsg->hdr.transSeqNum, LTL_FRAMECTL_DIR_SERVER_CLIENT, true, readRspCmd);
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
        if ( writeRspCmd == NULL ){
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

        ltl_SendWriteRsp(ApduMsg->pkt->srcaddr, ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo,ApduMsg->hdr.transSeqNum, 
                    LTL_FRAMECTL_DIR_SERVER_CLIENT,TRUE, writeRspCmd);
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
    if ( writeRspCmd == NULL ){
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

/*        if ( ltl_AccessCtrlAuthWrite( attrRec.accessControl ) ){
            // Not authorized to write - stop here
            writeRspCmd->attrList[j].status = LTL_STATUS_NOT_AUTHORIZED;
            writeRspCmd->attrList[j++].attrID = statusRec->attrID;
            break;
        }
*/
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
        if ( curWriteRec == NULL ){
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
                ltlReadAttrData(&attrRec, curDataPtr,  &dataLen );

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

    ltl_SendWriteRsp(ApduMsg->pkt->srcaddr, ApduMsg->hdr.trunkID, ApduMsg->hdr.nodeNo,ApduMsg->hdr.transSeqNum, 
                    LTL_FRAMECTL_DIR_SERVER_CLIENT, TRUE, writeRspCmd);
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
    if(pkt->apduLength == 0){
        return;
    }

    //parse frame head
    ApduMsg.pkt = pkt;
    ApduMsg.pdata = ltlParseHdr(&(ApduMsg.hdr),pkt->apduData);
    ApduMsg.datalen = pkt->apduLength;
    ApduMsg.datalen -= (uint16_t)(ApduMsg.pdata - pkt->apduData); // 通过指针偏移计算apdu帧头数据长度
    ApduMsg.attrCmd = NULL;
    
    // process frame head    
    //foundation type message in profile 标准命令
    if(ltl_IsProfileCmd(ApduMsg.hdr.fc.type)){
        if((ApduMsg.hdr.commandID <= LTL_CMD_PROFILE_MAX) && (ltlCmdTable[ApduMsg.hdr.commandID].pfnParseInProfile != NULL)){
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

    if(ApduMsg.pkt->isbroadcast == FALSE && ApduMsg.hdr.fc.disableDefaultRsp == LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF ){
        defaultRspCmd.commandID = ApduMsg.hdr.commandID;
        defaultRspCmd.statusCode = status;
        // send default response
        ltl_SendDefaultRspCmd(pkt->srcaddr, ApduMsg.hdr.trunkID, ApduMsg.hdr.nodeNo,
                                ApduMsg.hdr.transSeqNum, LTL_FRAMECTL_DIR_SERVER_CLIENT, 
                                TRUE,  &defaultRspCmd);
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
/*********************************************************************
 * @brief  long string to app string (app string format: len | string)
 *
 * @param   pRawStr - in -- pointer to the ascii Raw string buffer
 * @param   pAppStr - out -- pointer to the ascii app string buffer
 * @param   Applen  -- App buffer length
 *
 * @return  
 */
void ltl_LongStrToAppString(char *pRawStr, char *pAppStr, uint16_t Applen )
{
    uint16_t rawlen;
    
    rawlen = strlen(pRawStr);
    rawlen = MIN(rawlen, (Applen - OCTET_CHAR_LONG_HEADROOM_LEN));

    *pAppStr++ = LO_UINT16(rawlen); // for length low
    *pAppStr++ = HI_UINT16(rawlen); // for length high
    memcpy(pAppStr, pRawStr, rawlen);
}

