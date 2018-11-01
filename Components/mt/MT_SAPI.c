/**************************************************************************************************
  Filename:       MT_SAPI.c
  Revised:        $Date: 2015-02-05 17:15:13 -0800 (Thu, 05 Feb 2015) $
  Revision:       $Revision: 42371 $

  Description:    MonitorTest functions for the Simple API.

  Copyright 2007-2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.

**************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 ***************************************************************************************************/
#include "ZComDef.h"
#include "AddrMgr.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"
#include "MT.h"
#include "MT_SAPI.h"
#include "MT_UART.h"

#include "ZDProfile.h"
/***************************************************************************************************
 * GLOBAL VARIABLES
 ***************************************************************************************************/
#if defined ( MT_SAPI_CB_FUNC )
uint16 _sapiCallbackSub;
#endif

#if defined ( MT_SAPI_FUNC )
/***************************************************************************************************
 * LOCAL FUNCTIONS
 ***************************************************************************************************/
static void MT_SapiSystemReset(uint8 *pBuf);
static void MT_SapiResetFactory(uint8 *pBuf);
static void MT_SapiReadLogicalType(uint8 *pBuf);
static void MT_SapiWriteLogicalType(uint8 *pBuf);
static void MT_SapiStart(uint8* pBuf);
static void MT_SapiBindDevice(uint8 *pBuf);
static void MT_SapiAllowBind(uint8 *pBuf);
static void MT_SapiSendData(uint8 *pBuf);
static void MT_SapiReadCfg(uint8 *pBuf);
static void MT_SapiWriteCfg(uint8 *pBuf);
static void MT_SapiGetDevInfo(uint8 *pBuf);
static void MT_SapiGetDevAllInfo(uint8 *pBuf);
static void MT_SapiFindDev(uint8 *pBuf);
static void MT_SapiPermitJoin(uint8 *pBuf);
static void MT_SapiAppRegister(uint8 *pBuf);

static void MT_SapiBDBStartCommissioning(uint8* pBuf);
static void MT_SapiBDBZedAttemptRecoverNWK(uint8* pBuf);

extern void MT_SapiAfDataRequest(uint8 *pBuf);


/***************************************************************************************************
 * @fn      MT_sapiCommandProcessing
 *
 * @brief   Process all the SAPI commands that are issued by test tool
 *
 * @param   pBuf - pointer to received buffer
 *
 * @return  MT_RPC_SUCCESS if command processed, MT_RPC_ERR_COMMAND_ID if not.
 ***************************************************************************************************/
uint8 MT_SapiCommandProcessing(uint8 *pBuf)
{
  uint8 status = MT_RPC_SUCCESS;

  switch (pBuf[MT_RPC_POS_CMD1])
  {
    case MT_SAPI_START_REQ:
      MT_SapiStart(pBuf);
      break;

    case MT_SAPI_BIND_DEVICE_REQ:
      MT_SapiBindDevice(pBuf);
      break;

    case MT_SAPI_ALLOW_BIND_REQ:
      MT_SapiAllowBind(pBuf);
      break;

    case MT_SAPI_SEND_DATA_REQ:
      MT_SapiSendData(pBuf);
      break;

    case MT_SAPI_READ_CFG_REQ:
      MT_SapiReadCfg(pBuf);
      break;

    case MT_SAPI_WRITE_CFG_REQ:
      MT_SapiWriteCfg(pBuf);
      break;

    case MT_SAPI_GET_DEV_INFO_REQ:
      MT_SapiGetDevInfo(pBuf);
      break;

    case MT_SAPI_FIND_DEV_REQ:
      MT_SapiFindDev(pBuf);
      break;

    case MT_SAPI_PMT_JOIN_REQ:
      MT_SapiPermitJoin(pBuf);
      break;

    case MT_SAPI_SYS_RESET:
      MT_SapiSystemReset(pBuf);
      break;
    
    case MT_SAPI_APP_REGISTER_REQ:
      MT_SapiAppRegister(pBuf);
      break;
 //扩展命令   
 #if defined(MT_USER_BY_MO)
    case MT_SAPI_RESET_FACTORY:
      MT_SapiResetFactory(pBuf);
      break;
    
    case MT_SAPI_GET_DEV_ALL_INFO_REQ:
      MT_SapiGetDevAllInfo(pBuf);
      break;
    case MT_SAPI_READ_LOGICAL_TYPE:
      MT_SapiReadLogicalType(pBuf);
      break;
    case MT_SAPI_WRITE_LOGICAL_TYPE:
      MT_SapiWriteLogicalType(pBuf);
      break;
    case MT_SAPI_AF_DATA_REQUEST:
      MT_SapiAfDataRequest(pBuf);
      break;
    case MT_SAPI_BDB_START_COMMISSIONING:
      MT_SapiBDBStartCommissioning(pBuf);
      break;
    case MT_SAPI_BDB_ZED_ATTEMPT_RECOVER_NWK:
      MT_SapiBDBZedAttemptRecoverNWK(pBuf);
      break;
 #endif
    default:
      status = MT_RPC_ERR_COMMAND_ID;
      break;
  }

  return status;
}

/***************************************************************************************************
 * @fn          MT_SapiSystemReset
 *
 * @brief       Process SAPI System Reset
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiSystemReset(uint8 *pBuf)
{
  zb_SystemReset();
}
static void MT_SapiResetFactory(uint8 *pBuf)
{
    uint8 value = 0x01;
    uint8 retval;
    
    retval = osal_nv_write(ZCD_NV_STARTUP_OPTION, 0, 1, &value);
    
    MT_BuildAndSendZToolResponse( ((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_RESET_FACTORY, 1, &retval);
}

static void MT_SapiReadLogicalType(uint8 *pBuf)
{
    uint8 retStatus;
    uint8 buf[2];
    
    retStatus = zb_ReadConfiguration(ZCD_NV_LOGICAL_TYPE, 1, buf + 1);
    buf[0] = retStatus;
    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_READ_LOGICAL_TYPE, 2, buf);
}
static void MT_SapiWriteLogicalType(uint8 *pBuf)
{
    uint8 retValue = ZFailure;
    
    /* Parse header */
    pBuf += MT_RPC_FRAME_HDR_SZ;
    
    if((*pBuf <= ZG_DEVICETYPE_ENDDEVICE))
        retValue = zb_WriteConfiguration(ZCD_NV_LOGICAL_TYPE, 1, pBuf);

    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_WRITE_LOGICAL_TYPE, 1, &retValue);   
}
/***************************************************************************************************
 * @fn          MT_SapiStart
 *
 * @brief       Process SAPI Start
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiStart(uint8 *pBuf)
{
  zb_StartRequest();

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_START_REQ, 0, NULL);
}
/***************************************************************************************************
 * @fn          MT_SapiAppRegister
 *
 * @brief       Process SAPI App Register
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiAppRegister(uint8 *pBuf)
{
  uint8 ret = ZApsIllegalRequest;

  /* check if sapi is alredy registered with an endpoint */
  if ( (sapi_epDesc.endPoint == 0) && (*pBuf != 0) )
  {
    ret = MT_BuildEndpointDesc( pBuf+MT_RPC_FRAME_HDR_SZ, &sapi_epDesc );
    if ( ret == ZSuccess )
    {
      ret = afRegister( &sapi_epDesc );
      // Turn off match descriptor response by default
      afSetMatch(sapi_epDesc.simpleDesc->EndPoint, FALSE);
    }

    if ( ret != ZSuccess )
    {
      sapi_epDesc.endPoint = 0;
    }
  }

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI),
                                       MT_SAPI_APP_REGISTER_REQ, 1, &ret);
}
/***************************************************************************************************
 * @fn          MT_SapiBindDevice
 *
 * @brief       Process SAPI Bind Device Command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiBindDevice(uint8 *pBuf)
{
  uint8 cmdId;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  if (AddrMgrExtAddrValid(pBuf+3))
  {
    zb_BindDevice(pBuf[0], osal_build_uint16( &pBuf[1] ), &pBuf[3]);
  }
  else
  {
    zb_BindDevice(pBuf[0], osal_build_uint16( &pBuf[1] ), (uint8 *)NULL);
  }

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 0, NULL);
}

/***************************************************************************************************
 * @fn          MT_SapiAllowBind
 *
 * @brief       Process SAPI Allow Bind
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiAllowBind(uint8 *pBuf)
{
  uint8 cmdId;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  zb_AllowBind(pBuf[0]);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 0, NULL);
}

/***************************************************************************************************
 * @fn          MT_SapiSendData
 *
 * @brief       Process SAPI Send Data Command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiSendData(uint8 *pBuf)
{
  uint8 cmdId;
  uint16 destination, command;
  uint8 len, handle, txOption, radius;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination */
  destination = osal_build_uint16( &pBuf[0] );
  /* Command */
  command = osal_build_uint16( &pBuf[2] );
  /* Handle */
  handle = pBuf[4];
  /* txOption */
  txOption = pBuf[5];
  /* Radius */
  radius = pBuf[6];
  /* Length */
  len = pBuf[7];

  zb_SendDataRequest(destination, command, len, &pBuf[8], handle, txOption, radius);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 0, NULL);
}

/***************************************************************************************************
 * @fn          MT_SapiReadCfg
 *
 * @brief       Process SAPI Read Config Commands
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiReadCfg(uint8 *pBuf)
{
  uint8 len, retStatus;
  uint8 cfgId, cmdId;
  uint8 *pRetBuf;

  /* Parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  cfgId = pBuf[MT_RPC_POS_DAT0];

  /* Length of item in NV memory */
  len = (uint8)osal_nv_item_len(cfgId);

  pRetBuf = osal_mem_alloc(len+3);
  if (pRetBuf != NULL)
  {
    if (len && ((cfgId != ZCD_NV_NIB) && (cfgId != ZCD_NV_DEVICE_LIST) &&
                (cfgId != ZCD_NV_ADDRMGR) && (cfgId != ZCD_NV_NWKKEY)))
    {
      if ((zb_ReadConfiguration(cfgId, len, pRetBuf+3)) == ZSUCCESS)
      {
        retStatus = ZSuccess;
      }
      else
      {
        retStatus = ZFailure;
      }
    }
    else
    {
      retStatus = ZInvalidParameter;
    }

    if (retStatus != ZSuccess)
    {
       /* Don't return garbage with error */
       len = 0;
    }

    /* Status */
    pRetBuf[0] = retStatus;
    /* Config ID */
    pRetBuf[1] = cfgId;
    /* NV item length */
    pRetBuf[2] = len;

    /* Build and send back the response */
    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, len+3, pRetBuf );

    osal_mem_free(pRetBuf);
  }
}

/***************************************************************************************************
 * @fn          MT_SpiWriteCfg
 *
 * @brief       Process Write Configuration Command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiWriteCfg(uint8 *pBuf)
{
  uint8 retValue, cmdId;

  /* Parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  if ((pBuf[0] != ZCD_NV_NIB) && (pBuf[0] != ZCD_NV_DEVICE_LIST) &&
      (pBuf[0] != ZCD_NV_ADDRMGR) && (pBuf[0] != ZCD_NV_NWKKEY))
  {
    if ((zb_WriteConfiguration(pBuf[0], pBuf[1], &pBuf[2])) == ZSUCCESS)
    {
      retValue = ZSuccess;
    }
    else
    {
      retValue = ZFailure;
    }
  }
  else
  {
    retValue = ZInvalidParameter;
  }

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 1, &retValue );
}

/***************************************************************************************************
 * @fn          MT_SapiGetDevInfo
 *
 * @brief       Process Get Device Info command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiGetDevInfo(uint8 *pBuf)
{
  uint8 *pRetBuf;
  uint8 cmdId;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  pRetBuf = osal_mem_alloc(Z_EXTADDR_LEN+1);
  if (pRetBuf)
  {
    zb_GetDeviceInfo(pBuf[0], pRetBuf+1);
    pRetBuf[0] = pBuf[0];

    /* Build and send back the response */
    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, Z_EXTADDR_LEN+1, pRetBuf );

    osal_mem_free(pRetBuf);
  }
}

static void MT_SapiGetDevAllInfo(uint8 *pBuf)
{
  uint8 *pRetBuf;
  uint8 *ptemp;
  uint8 cmdId;
  uint8 size;
  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  size = sizeof(uint8) * 2 + (Z_EXTADDR_LEN  + sizeof(uint16) ) * 3;
  pRetBuf = osal_mem_alloc(size);
  if (pRetBuf)
  {
    ptemp = pRetBuf;
    // device state
    zb_GetDeviceInfo(ZB_INFO_DEV_STATE, ptemp);
    ptemp += sizeof(uint8);
    //short address
    zb_GetDeviceInfo(ZB_INFO_SHORT_ADDR, ptemp);
    ptemp += sizeof(uint16);
    // ieee address
    zb_GetDeviceInfo(ZB_INFO_IEEE_ADDR, ptemp);
    ptemp += Z_EXTADDR_LEN;
    // parent short address
    zb_GetDeviceInfo(ZB_INFO_PARENT_SHORT_ADDR, ptemp);
    ptemp += sizeof(uint8);
    // parent ieee address
    zb_GetDeviceInfo(ZB_INFO_PARENT_IEEE_ADDR, ptemp);
    ptemp += Z_EXTADDR_LEN;
    // channell
    zb_GetDeviceInfo(ZB_INFO_CHANNEL, ptemp);
    ptemp += sizeof(uint8);
    // panid
    zb_GetDeviceInfo(ZB_INFO_PAN_ID, ptemp);
    ptemp += sizeof(uint16);
    // extend panid
    zb_GetDeviceInfo(ZB_INFO_EXT_PAN_ID, ptemp);
    ptemp += Z_EXTADDR_LEN;

    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, size, pRetBuf );

    osal_mem_free(pRetBuf);
  }
}

/***************************************************************************************************
 * @fn          MT_SapiFindDev
 *
 * @brief       Process Find Device Command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiFindDev(uint8 *pBuf)
{
  uint8 cmdId;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Currently only supports IEEE search */
  zb_FindDeviceRequest(ZB_IEEE_SEARCH, pBuf);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 0, NULL );
}

/***************************************************************************************************
 * @fn          MT_SapiPermitJoin
 *
 * @brief       Process Permit Join Command
 *
 * @param       pBuf - pointer to received buffer
 *
 * @return      none
 ***************************************************************************************************/
static void MT_SapiPermitJoin(uint8 *pBuf)
{
  uint8 retValue, cmdId;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  {// 需要认证中心认证
    zAddrType_t dstAddr;
    dstAddr.addr.shortAddr = osal_build_uint16( pBuf );
    dstAddr.addrMode = AddrBroadcast;
    // Trust Center significance is always true
    retValue = ZDP_MgmtPermitJoinReq( &dstAddr, pBuf[2], TRUE, FALSE );
  }
  //retValue = zb_PermitJoiningRequest(osal_build_uint16( pBuf ), pBuf[2]);
  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 1, &retValue );

}
#endif  /* MT_SAPI_FUNC */

#if defined ( MT_SAPI_CB_FUNC )
/***************************************************************************************************
 * @fn          zb_MTCallbackStartConfirm
 *
 * @brief       Process the callback subscription for zb_StartConfirm
 *
 * @param       Status - status
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackStartConfirm( uint8 status )
{
  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_START_CNF, 1, &status);
}

/***************************************************************************************************
 * @fn          zb_MTCallbackSendDataConfirm
 *
 * @brief       Process the callback subscription for zb_SendDataConfirm
 *
 * @param
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackSendDataConfirm(uint8 handle, uint8 status)
{
  uint8 retArray[2];

  retArray[0] = handle;
  retArray[1] = status;

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_SEND_DATA_CNF, 2, retArray);

}

/***************************************************************************************************
 * @fn          zb_MTCallbackBindConfirm
 *
 * @brief       Process the callback subscription for zb_BindConfirm
 *
 * @param
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackBindConfirm( uint16 commandId, uint8 status )
{
  uint8 retArray[3];

  retArray[0] = LO_UINT16(commandId);
  retArray[1] = HI_UINT16(commandId);
  retArray[2] = status;

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_BIND_CNF, 3, retArray);

}
/***************************************************************************************************
 * @fn          zb_MTCallbackAllowBindConfirm
 *
 * @brief       Indicates when another device attempted to bind to this device
 *
 * @param
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackAllowBindConfirm( uint16 source )
{
  uint8 retArray[2];

  retArray[0] = LO_UINT16(source);
  retArray[1] = HI_UINT16(source);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_ALLOW_BIND_CNF, 2, retArray);

}
/***************************************************************************************************
 * @fn          zb_MTCallbackFindDeviceConfirm
 *
 * @brief       Process the callback subscription for zb_FindDeviceConfirm
 *
 * @param
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackFindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result )
{
  uint8 retArray[SPI_CB_SAPI_FIND_DEV_CNF_LEN];
  uint16 addr = *((uint16*)searchKey);

  // Currently only supports IEEE Addr Search
  retArray[0] = ZB_IEEE_SEARCH;
  retArray[1] = LO_UINT16(addr);
  retArray[2] = HI_UINT16(addr);
  osal_memcpy(&retArray[3], result, Z_EXTADDR_LEN);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_FIND_DEV_CNF, 11, retArray);

}

/***************************************************************************************************
 * @fn          zb_MTCallbackReceiveDataIndication
 *
 * @brief       Process the callback subscription for zb_ReceiveDataIndication
 *
 * @param
 *
 * @return      none
 ***************************************************************************************************/
void zb_MTCallbackReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{
  uint8 *memPtr;
  int8 i;
  uint8 msgLen = 6 + len;

  memPtr = osal_mem_alloc(msgLen);

  if (memPtr)
  {
    memPtr[0] = LO_UINT16(source);
    memPtr[1] = HI_UINT16(source);
    memPtr[2] = LO_UINT16(command);
    memPtr[3] = HI_UINT16(command);
    memPtr[4] = LO_UINT16(len);
    memPtr[5] = HI_UINT16(len);

    for (i=0; i<len; i++)
    {
      memPtr[6+i] = pData[i];
    }

    /* Build and send back the response */
    MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_SAPI), MT_SAPI_RCV_DATA_IND, msgLen, memPtr);

    osal_mem_free( memPtr );
  }
}
#endif  /* MT_SAPI_CB_FUNC */

// 移MT_APP_CONFIG指令过来
#include "MT_APP_CONFIG.h"
static void MT_SapiBDBStartCommissioning(uint8* pBuf)
{
  uint8 retValue = ZSuccess;
  uint8 cmdId;
  
  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;
  
  bdb_StartCommissioning(*pBuf);
  
  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 1, &retValue);
}

static void MT_SapiBDBZedAttemptRecoverNWK(uint8* pBuf)
{
  uint8 retValue = ZSuccess;
  uint8 cmdId;
  
  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  retValue = bdb_ZedAttemptRecoverNwk();
  
  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_SAPI), cmdId, 1, &retValue);
}

/***************************************************************************************************
 ***************************************************************************************************/
