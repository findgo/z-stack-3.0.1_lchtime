#ifndef __LTL_H_
#define __LTL_H_

#include "hal_defs.h"
#include "hal_types.h"
#include "memalloc.h"
#include "ltl_trunk.h"
#include "prefix.h"

// define in frame control field
//bit mask
#define LTL_FRAMECTL_TYPE_MASK             0x03
#define LTL_FRAMECTL_DISALBE_DEFAULT_RSP_MASK  0x04
//subfield type
#define LTL_FRAMECTL_TYPE_PROFILE          0x00
#define LTL_FRAMECTL_TYPE_TRUNK_SPECIFIC   0x01
//subfield disable default response
#define LTL_FRAMECTL_DIS_DEFAULT_RSP_OFF 0
#define LTL_FRAMECTL_DIS_DEFAULT_RSP_ON 1

// General command IDs on profile 
#define LTL_CMD_READ_ATTRIBUTES                 0x00
#define LTL_CMD_READ_ATTRIBUTES_RSP             0x01
#define LTL_CMD_WRITE_ATTRIBUTES                0x02
#define LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED      0x03
#define LTL_CMD_WRITE_ATTRIBUTES_RSP            0x04
#define LTL_CMD_WRITE_ATTRIBUTES_NORSP          0x05
#define LTL_CMD_CONFIGURE_REPORTING             0x06 
#define LTL_CMD_CONFIGURE_REPORTING_RSP         0x07
#define LTL_CMD_READ_CONFIGURE_REPORTING        0x08
#define LTL_CMD_READ_CONFIGURE_REPORTING_RSP    0x09
#define LTL_CMD_REPORT_ATTRIBUTES               0x0a
#define LTL_CMD_DEFAULT_RSP                     0x0b
#define LTL_CMD_PROFILE_MAX                     LTL_CMD_DEFAULT_RSP

/*** Data Types (32) ***/
#define LTL_DATATYPE_NO_DATA                            0x00
#define LTL_DATATYPE_BOOLEAN                            0x10
#define LTL_DATATYPE_BITMAP8                            0x15
#define LTL_DATATYPE_BITMAP16                           0x16
#define LTL_DATATYPE_BITMAP32                           0x17
#define LTL_DATATYPE_BITMAP64                           0x18
#define LTL_DATATYPE_UINT8                              0x25
#define LTL_DATATYPE_UINT16                             0x26
#define LTL_DATATYPE_UINT32                             0x27
#define LTL_DATATYPE_UINT64                             0x28
#define LTL_DATATYPE_INT8                               0x35
#define LTL_DATATYPE_INT16                              0x36
#define LTL_DATATYPE_INT32                              0x37
#define LTL_DATATYPE_INT64                              0x38
#define LTL_DATATYPE_ENUM8                              0x3a
#define LTL_DATATYPE_ENUM16                             0x3b
#define LTL_DATATYPE_SINGLE_PREC                        0x3c
#define LTL_DATATYPE_DOUBLE_PREC                        0x3d
#define LTL_DATATYPE_CHAR_STR                           0x41
#define LTL_DATATYPE_OCTET_ARRAY                        0x42
#define LTL_DATATYPE_DWORD_ARRAY                        0x43
#define LTL_DATATYPE_SN_ADDR                            0x51
#define LTL_DATATYPE_128_BIT_SEC_KEY                    0x52
#define LTL_DATATYPE_UNKNOWN                            0xff

/*** Error Status Codes ***/
#define LTL_STATUS_SUCCESS                              0x00 //操作成功
#define LTL_STATUS_FAILURE                              0x01 //操作失败
// 0x02-0x7E are reserved.
#define LTL_STATUS_MALFORMED_COMMAND                    0x80
#define LTL_STATUS_UNSUP_TRUNK_COMMAND                  0x81  //不支持集下命令
#define LTL_STATUS_UNSUP_GENERAL_COMMAND                0x82   //不支持profile下的通用标准命令
#define LTL_STATUS_INVALID_FIELD                        0x83  // 域无效,一般表现为发送的值域对设备无影响
#define LTL_STATUS_UNSUPPORTED_ATTRIBUTE                0x84  //不支持的属性
#define LTL_STATUS_INVALID_VALUE                        0x85  // 无效数值
#define LTL_STATUS_READ_ONLY                            0x86   // 只读
#define LTL_STATUS_NOT_FOUND                            0x87  //请求的信息没有找到
#define LTL_STATUS_UNREPORTABLE_ATTRIBUTE               0x88  // 这个属性不能定期报告
#define LTL_STATUS_INVALID_DATA_TYPE                    0x89    //无效数据类型
#define LTL_STATUS_WRITE_ONLY                           0x8a   //只写
#define LTL_STATUS_DEFINED_OUT_OF_BAND                  0x8b   // 写的数据超过范围
#define LTL_STATUS_INCONSISTENT                         0x8c
#define LTL_STATUS_ACTION_DENIED                        0x8d  // 拒绝此命令动作
#define LTL_STATUS_TIMEOUT                              0x8e  //超时
#define LTL_STATUS_ABORT                                0x8f  //停止
#define LTL_STATUS_HARDWARE_FAILURE                     0x90  // 硬件问题错误
#define LTL_STATUS_SOFTWARE_FAILURE                     0x91  // 软件错误
// 0xa0-0xff are reserved.
#define LTL_STATUS_CMD_HAS_RSP                          0xff

/*** Attribute Access Control - bit masks ***/
#define ACCESS_CONTROL_READ                             0x01
#define ACCESS_CONTROL_WRITE                            0x02

// Used by ltlReadWriteCB_t callback function
#define LTL_OPER_LEN                                    0x00 // Get length of attribute value to be read
#define LTL_OPER_READ                                   0x01 // Read attribute value
#define LTL_OPER_WRITE                                  0x02 // Write new attribute value

#define LTL_SUCCESS  0x00
#define LTL_FAILURE  0x01  
#define LTL_MEMERROR 0x02



// 对于应用层的数据类型的一些 字节串 字符串 数组进行定义预留长度
#define OCTET_CHAR_HEADROOM_LEN         (1) // length : 1

/*********************************************************************
 * MACROS
 */
#define ltl_IsProfileCmd( a )           ( (a) == LTL_FRAMECTL_TYPE_PROFILE )
#define ltl_IsTrunkCmd( a )             ( (a) == LTL_FRAMECTL_TYPE_TRUNK_SPECIFIC )

typedef uint8_t LStatus_t;

// LTL header - frame control field
typedef struct
{
    uint8_t type;
    uint8_t disableDefaultRsp;
    uint8_t reserved;
} ltlFrameHdrctl_t;

// LTL header
typedef struct
{
    uint16_t    trunkID;
    uint8_t     nodeNo;
    uint8_t     transSeqNum;
    uint8_t     commandID;
    ltlFrameHdrctl_t fc;
} ltlFrameHdr_t;

//解析完包头的APDU包
typedef struct 
{
    MoIncomingMsgPkt_t *pkt;
    ltlFrameHdr_t hdr;  /* adpu head frame  */
    uint16_t datalen; // length of remaining data
    uint8_t *pdata;  // point to data after header
    void *attrCmd; // point to the parsed attribute or command
}ltlApduMsg_t;

// Parse received command
typedef struct
{
    uint16_t trunkID;
    uint8_t  nodeNO;
    uint16_t dataLen;
    uint8_t  *pData;
} ltlParseCmd_t;

/*------------------------for application-----------------------------------------------*/

// Attribute record
typedef struct
{
  uint16_t  attrId;         // Attribute ID
  uint8_t   dataType;       // Data Type 
  uint8_t   accessControl;  // Read/write acess - bit field
  void    *dataPtr;       // Pointer to data field
} ltlAttrRec_t;



// for read
// Read Attribute Command format
// be allocated with the appropriate number of attributes.
typedef struct
{
  uint8_t  numAttr;            // number of attributes in the list
  uint16_t attrID[];           // supported attributes list - this structure should
} ltlReadCmd_t;

// Read Attribute Response Status record format
typedef struct
{
  uint16_t attrID;            // attribute ID
  uint8_t  status;            // should be ZCL_STATUS_SUCCESS or error
  uint8_t  dataType;          // attribute data type
  uint8_t  *data;             // this structure is allocated, so the data is HERE
                            // - the size depends on the attribute data type
} ltlReadRspStatus_t;

// Read Attribute Response Command format
typedef struct
{
  uint8_t            numAttr;     // number of attributes in the list
  ltlReadRspStatus_t attrList[];  // attribute status list
} ltlReadRspCmd_t;

// for write
// Write Attribute record
typedef struct
{
  uint16_t attrID;             // attribute ID
  uint8_t  dataType;           // attribute data type
  uint8_t  *attrData;          // this structure is allocated, so the data is HERE
                             //  - the size depends on the attribute data type
} ltlWriteRec_t;

// Write Attribute Command format
typedef struct
{
  uint8_t         numAttr;     // number of attribute records in the list
  ltlWriteRec_t attrList[];  // attribute records
} ltlWriteCmd_t;
// Write Attribute Status record
typedef struct
{
  uint8_t  status;             // should be LTL_STATUS_SUCCESS or error
  uint16_t attrID;             // attribute ID
} ltlWriteRspStatus_t;

// Write Attribute Response Command format
typedef struct
{
  uint8_t               numAttr;     // number of attribute status in the list
  ltlWriteRspStatus_t attrList[];  // attribute status records
} ltlWriteRspCmd_t;
// Configure Reporting Command format
typedef struct
{
  uint16_t attrID;             // attribute ID
  uint8_t  dataType;           // attribute data type
  uint16_t minReportInt;       // minimum reporting interval
  uint8_t  *reportableChange;  // reportable change (only applicable to analog data type)
                             // - the size depends on the attribute data type
} ltlCfgReportRec_t;

typedef struct
{
  uint8_t             numAttr;    // number of attribute IDs in the list
  ltlCfgReportRec_t attrList[]; // attribute ID list
} ltlCfgReportCmd_t;

// Attribute Status record
typedef struct
{
  uint8_t  status;             // should be ZCL_STATUS_SUCCESS or error
  uint16_t attrID;             // attribute ID
} ltlCfgReportStatus_t;

// Configure Reporting Response Command format
typedef struct
{
  uint8_t                numAttr;    // number of attribute status in the list
  ltlCfgReportStatus_t attrList[]; // attribute status records
} ltlCfgReportRspCmd_t;

typedef struct
{
  uint8_t  numAttr;    // number of attributes in the list
  uint16_t attrID[];                 // attribute ID table
} ltlReadReportCfgCmd_t;

// Attribute Reporting Configuration record
typedef struct
{
  uint8_t  status;             // status field
  uint16_t attrID;             // attribute ID
  uint8_t  dataType;           // attribute data type
  uint16_t minReportInt;       // minimum reporting interval
  uint8_t  *reportableChange;  // reportable change (only applicable to analog data type)
                             // - the size depends on the attribute data type
} ltlReportCfgRspRec_t;
                             
// Read Reporting Configuration Response Command format
typedef struct
{
  uint8_t              numAttr;    // number of records in the list
  ltlReportCfgRspRec_t attrList[]; // attribute reporting configuration list
} ltlReadReportCfgRspCmd_t;

// Attribute Report
typedef struct
{
  uint16_t attrID;             // atrribute ID
  uint8_t  dataType;           // attribute data type
  uint8_t  *attrData;          // this structure is allocated, so the data is HERE
                             // - the size depends on the data type of attrID
} ltlReport_t;

// Report Attributes Command format
typedef struct
{
  uint8_t       numAttr;       // number of reports in the list
  ltlReport_t attrList[];    // attribute report list
} ltlReportCmd_t;

// Default Response Command format
typedef struct
{
  uint8_t  commandID;
  uint8_t  statusCode;
} ltlDefaultRspCmd_t;

/* for callback */
// Function pointer type to handle incoming messages.
// The return value of the plugin function will be
//  LTL_STATUS_SUCCESS - Supported and need default response
//  LTL_STATUS_FAILURE - Unsupported
//  LTL_STATUS_CMD_HAS_RSP - Supported and do not need default rsp
//  LTL_STATUS_INVALID_FIELD - Supported, but the incoming msg is wrong formatted
//  LTL_STATUS_INVALID_VALUE - Supported, but the request not achievable by the h/w
//  LTL_STATUS_MEMERROR - Supported but memory allocation fails
typedef LStatus_t (*ltlSpecificTrunckHdCB_t)( ltlApduMsg_t *ApduMsg );



/* 回调函数定义当   ltlAttrRec_t 属性记录中dataPtr为NULL是由用户提供数据
回调实现三个oper, LTL_OPER_LEN,LTL_OPER_READ,LTL_OPER_WRITE
由用户决定数据的存储,比如数据库,flash等
return LTL_STATUS_SUCCESS 成功
*/
typedef LStatus_t (*ltlReadWriteCB_t)( uint16_t trunkID, uint8_t nodeNO, uint16_t attrId, uint8_t oper,
                                       uint8_t *pValue, uint16_t *pLen );

/*********************************************************************
 *              注册特定集下命令解析回调,用于解析集下命令
 * @brief       Add a trunk Library handler
 *
 * @param       starttrunkID -  trunk ID start
 * @param       endtrunkID -  trunk ID end
 * @param       pfnSpecificTrunkHdCB - function pointer to specific callback handler
 
 * @return      0 if OK
 */
LStatus_t ltl_registerPlugin(uint16_t starttrunkID,uint16_t endtrunkID,ltlSpecificTrunckHdCB_t pfnSpecificTrunkHdCB);
/*********************************************************************
 *              注册集下 指定节点的属性列表
 * @brief      
 * @param       trunkID -  trunk ID
 * @param       nodeNO -  node number
 * @param       numAttr -  attribute number 
 * @param       newAttrList[] - list of attrubute
 *
 * @return      0 if OK
 */
LStatus_t ltl_registerAttrList(uint16_t trunkID, uint8_t nodeNO, uint8_t numAttr,const ltlAttrRec_t newAttrList[] );
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
 *
 * @return      LTL_SUCCESS if successful. LTL_FAILURE, otherwise.
 */
LStatus_t ltl_registerReadWriteCB(uint16_t trunkID, uint8_t nodeNO, ltlReadWriteCB_t pfnReadWriteCB );
uint8_t ltlIsAnalogDataType( uint8_t dataType );
uint8_t ltlGetDataTypeLength( uint8_t dataType );
uint16_t ltlGetAttrDataLength( uint8_t dataType, uint8_t *pData );

LStatus_t ltl_SendCommand(uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO,uint8_t seqNum, 
                                uint8_t specific , uint8_t disableDefaultRsp,
                                uint8_t cmd, uint8_t *cmdFormat,uint16_t cmdFormatLen);
LStatus_t ltl_SendReadReq(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO, uint8_t seqNum, ltlReadCmd_t *readCmd );
LStatus_t ltl_SendReadRsp(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO, uint8_t seqNum, ltlReadRspCmd_t *readRspCmd );
LStatus_t ltl_SendWriteRequest(uint16_t dstAddr, uint16_t trunkID, uint8_t nodeNO, uint8_t seqNum, uint8_t cmd, ltlWriteCmd_t *writeCmd );

#define ltl_SendWriteReq(dstAddr, trunkID, nodeNO, seqNum, writeCmd ) ltl_SendWriteRequest(dstAddr, trunkID, nodeNO, seqNum, LTL_CMD_WRITE_ATTRIBUTES, writeCmd )
#define ltl_SendWriteReqUndivided(dstAddr, trunkID, nodeNO, seqNum, writeCmd ) ltl_SendWriteRequest(dstAddr, trunkID, nodeNO, seqNum,  LTL_CMD_WRITE_ATTRIBUTES_UNDIVIDED, writeCmd )       
#define ltl_SendWriteReqNoRsp(dstAddr, trunkID, nodeNO, seqNum, direction, disableDefaultRsp, writeCmd ) ltl_SendWriteRequest(dstAddr, trunkID, nodeNO, seqNum, LTL_CMD_WRITE_ATTRIBUTES_NORSP, writeCmd )
                        
LStatus_t ltl_SendWriteRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum , ltlWriteRspCmd_t *writeRspCmd);

LStatus_t ltl_SendConfigReportReq( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum , ltlCfgReportCmd_t *cfgReportCmd);

LStatus_t ltl_SendConfigReportRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum , ltlCfgReportRspCmd_t *cfgReportRspCmd);

LStatus_t ltl_SendReadReportCfgReq( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum , ltlReadReportCfgCmd_t *readReportCfgCmd);

LStatus_t ltl_SendReadReportCfgRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum , ltlReadReportCfgRspCmd_t *readReportCfgRspCmd);

LStatus_t ltl_SendReportCmd( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum, uint8_t disableDefaultRsp, ltlReportCmd_t *reportCmd);

LStatus_t ltl_SendDefaultRsp( uint16_t dstAddr, uint16_t trunkID,uint8_t nodeNO, uint8_t seqNum, ltlDefaultRspCmd_t *defaultRspCmd);

#define ltl_SendSpecificCmd(dstAddr,trunkID,nodeNO,seqNum,disableDefaultRsp,cmd,cmdFormat,cmdFormatLen) ltl_SendCommand(dstAddr, trunkID,nodeNO,seqNum,TRUE, disableDefaultRsp,cmd,cmdFormat,cmdFormatLen)


void ltl_ProcessInApdu(MoIncomingMsgPkt_t *pkt);


uint8_t ltlFindAttrRec( uint16_t trunkID,  uint8_t nodeNO, uint16_t attrId, ltlAttrRec_t *pAttr );


void ltl_StrToAppString(char *pRawStr, char *pAppStr, uint8_t Applen );

#endif
