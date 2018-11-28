
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"

#include "nwk_util.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "onboard.h"

#include "ltlApp.h"
/* HAL */
#include "hal_led.h"
#include "hal_key.h"
#include "mcoils.h"
#include "lownwk.h"

#include "ltl_app_genattr.h"

/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte ltlApp_TaskID;


/*********************************************************************
 * GLOBAL FUNCTIONS
 */
 
/*********************************************************************
 * LOCAL VARIABLES
 */

devStates_t ltlApp_NwkState = DEV_INIT;

/*********************************************************************
 * LOCAL VARIABLES
 */
afAddrType_t ltlApp_DstAddr;

/*********************************************************************
 * SIMPLE DESCRIPTOR   by mo
 */
 
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t LchtimeApp_ClusterInList[] =
{
    LCHTIMEAPP_CLUSTERID,  
};
const cId_t LchtimeApp_ClusterOutList[] = 
{
    LCHTIMEAPP_CLUSTERID,
};
    
#define LCHTIMEAPP_MAX_INCLUSTERS   (sizeof(LchtimeApp_ClusterInList) / sizeof(LchtimeApp_ClusterInList[0]))
#define LCHTIMEAPP_MAX_OUTCLUSTERS  (sizeof(LchtimeApp_ClusterOutList) / sizeof(LchtimeApp_ClusterOutList[0]))

SimpleDescriptionFormat_t ltlApp_SimpleDesc =
{
  LCHTIMEAPP_ENDPOINT,                  //  int Endpoint;
  LCHTIMEAPP_PROFID,                    //  uint16 AppProfId[2];
  LCHTIMEAPP_DEVICEID,                  //  uint16 AppDeviceId[2];
  LCHTIMEAPP_DEVICE_VERSION,            //  int   AppDevVer:4;
  LCHTIMEAPP_FLAGS,                     //  int   AppFlags:4;
  LCHTIMEAPP_MAX_INCLUSTERS,            //  byte  AppNumInClusters;
  (cId_t *)LchtimeApp_ClusterInList,      //  byte *pAppInClusterList;
  LCHTIMEAPP_MAX_OUTCLUSTERS,           //  byte  AppNumInClusters;
  (cId_t *)LchtimeApp_ClusterOutList    //  byte *pAppInClusterList;
};

static CONST endPointDesc_t ltlApp_Ep =
{
  LCHTIMEAPP_ENDPOINT,
  0,
  &ltlApp_TaskID,
  (SimpleDescriptionFormat_t *)&ltlApp_SimpleDesc,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)noLatencyReqs            // No Network Latency req
}; 
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ltlApp_HandleKeys( byte shift, byte keys );
static void ltlApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void *ltlApp_ZdoLeaveInd(void *vPtr);
static void Meter_Leave(void);

static void hal_coilsInit(void);


/*********************************************************************
 * @fn          ltlApp_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void ltlApp_Init( byte task_id )
{
    ltlApp_TaskID = task_id;
    
    log_Init();
    log_infoln("reason: %x",ResetReason());
    log_infoln("app init!");
    
    // Set destination address to indirect
    ltlApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    ltlApp_DstAddr.endPoint = LCHTIMEAPP_ENDPOINT;
    ltlApp_DstAddr.addr.shortAddr = 0x0000;
    
    
    // This app is part of the Home Automation Profile, resgister a ep
    bdb_RegisterSimpleDescriptor( &ltlApp_SimpleDesc );
    
    // Register the ZCL General Cluster Library callback functions
    ltl_GeneralBasicAttriInit();
    
    // Register the application's attribute list
    //zclSampleLight_ResetAttributesToDefaultValues();
        
    // Register the Application to receive the unprocessed Foundation command/response messages
    LowNwk_registerForMsg( ltlApp_TaskID );
    
    // Register for all key events - This app will handle all key events
    RegisterForKeys( ltlApp_TaskID );
    
    bdb_RegisterCommissioningStatusCB( ltlApp_ProcessCommissioningStatus );

    osal_set_event( task_id, LTLAPP_DEVICE_REJOIN_EVT);  // start device join nwk
    HalLedBlink(HAL_LED_1 | HAL_LED_2 | HAL_LED_3, 50, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
    hal_coilsInit();
    log_infoln("app started");
    //ZDO_RegisterForZdoCB(ZDO_LEAVE_IND_CBID, &ltlApp_ZdoLeaveInd);

#if defined(GLOBAL_DEBUG)
    //osal_set_event(ltlApp_TaskID, LTLAPP_TEST_EVT);
#endif
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 ltlApp_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ltlApp_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          //ltlApp_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          ltlApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          ltlApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          log_alertln("zdo state: %d", ltlApp_NwkState);

          // now on the network
          if ( (ltlApp_NwkState == DEV_END_DEVICE) || (ltlApp_NwkState == DEV_ROUTER) )
          {
          
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

 if(events & LTLAPP_DEVICE_REJOIN_EVT)
  {
  
    log_alertln("attempt rejoin nwk!");
    
    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    return ( events ^ LTLAPP_DEVICE_REJOIN_EVT );
  }
 
#if ZG_BUILD_ENDDEVICE_TYPE
  if(events & LTLAPP_DEVICE_RECOVER_EVT)
  {
    log_alertln("zed attempt recover nwk!");
    if(devState == DEV_NWK_ORPHAN)
        bdb_ZedAttemptRecoverNwk();
    else
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    return ( events ^ LTLAPP_DEVICE_RECOVER_EVT );
  }
#endif
#if defined(GLOBAL_DEBUG)
  if(events & LTLAPP_TEST_EVT)
  {
    log_alertln("app test event");
    osal_start_timerEx(ltlApp_TaskID, LTLAPP_TEST_EVT, LTLAPP_END_DEVICE_REJOIN_DELAY);
    return ( events ^ LTLAPP_TEST_EVT );
  }
#endif
  // Discard unknown events
  return 0;
}


/*********************************************************************
 * @fn      ltlApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_5
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void ltlApp_HandleKeys( byte shift, byte keys )
{
    log_debugln("handle key: %x,down: %d", keys, shift);
#if defined (LIGHT_NODE_1)
    if ( keys & HAL_KEY_SW_1 ){
        LigthApp_OnOffUpdate(LIGHT_NODE_1, MCOILS_MODE_TOGGLE);
    }
#endif
#if defined (LIGHT_NODE_2)
if ( keys & HAL_KEY_SW_2 ){
        LigthApp_OnOffUpdate(LIGHT_NODE_2, MCOILS_MODE_TOGGLE);
    }
#endif
#if defined (LIGHT_NODE_3)
    if ( keys & HAL_KEY_SW_3 ){
        LigthApp_OnOffUpdate(LIGHT_NODE_3, MCOILS_MODE_TOGGLE);
    }
#endif
    if ( keys & HAL_KEY_SW_4 ){
        //Meter_Leave();
    }
  
}

/*********************************************************************
 * @fn      ltlApp_ProcessCommissioningStatus
 *
 * @brief   Callback in which the status of the commissioning process are reported
 *
 * @param   bdbCommissioningModeMsg - Context message of the status of a commissioning process
 *
 * @return  none
 */
static void ltlApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
  switch(bdbCommissioningModeMsg->bdbCommissioningMode)
  {
/*    case BDB_COMMISSIONING_FORMATION: // only coor
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        log_debugln("bdb process nwk formation success!");
        //After formation, perform nwk steering again plus the remaining commissioning modes that has not been process yet
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
      }
      else
      {
        log_debugln("bdb process nwk formation failed!");
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    */
    case BDB_COMMISSIONING_NWK_STEERING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        //We are on the nwk, what now?
        // show on net
#if defined (LIGHT_NODE_1)
        LED1_TURN(COIL1_STATE());
#endif
#if defined (LIGHT_NODE_2)
        LED2_TURN(COIL2_STATE());
#endif
#if defined (LIGHT_NODE_3)
        LED3_TURN(COIL3_STATE());
#endif
        log_debugln("bdb process nwk success,and on nwk!");
        
        osal_stop_timerEx( ltlApp_TaskID, LTLAPP_DEVICE_REJOIN_EVT);
      }
      else
      {
        //See the possible errors for nwk steering procedure
        //No suitable networks found
        //Want to try other channels?
        //try with bdb_setChannelAttribute
        // try again??
        log_debugln("bdb process nwk failed(%d),rejoin after a monment!",bdbCommissioningModeMsg->bdbCommissioningStatus);
        osal_start_timerEx( ltlApp_TaskID, LTLAPP_DEVICE_REJOIN_EVT, LTLAPP_END_DEVICE_REJOIN_DELAY);
      }
    break;
/*    case BDB_COMMISSIONING_FINDING_BINDING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {   
        log_debugln("bdb process find and binding success!");
        //YOUR JOB:
      }
      else
      {
        log_debugln("bdb process find and binding failed!");
        //YOUR JOB:
        //retry?, wait for user interaction?
      }
    break;
    */
    case BDB_COMMISSIONING_INITIALIZATION:
      log_debugln("bdb process Initialization!");
      //Initialization notification can only be successful. Failure on initialization 
      //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification
      
      //YOUR JOB:
      //We are on a network, what now?
      
    break;
#if ZG_BUILD_ENDDEVICE_TYPE    
    case BDB_COMMISSIONING_PARENT_LOST:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
      {
        log_debugln("bdb process recover from losing parent!");
        osal_stop_timerEx( ltlApp_TaskID, LTLAPP_DEVICE_RECOVER_EVT);
        //We did recover from losing parent
      }
      else
      {      
        log_debugln("bdb process parent not found,and rejoin a nwk!");
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(ltlApp_TaskID, LTLAPP_DEVICE_RECOVER_EVT, LTLAPP_END_DEVICE_REJOIN_DELAY);
      }
    break;
#endif 
  }
}

static void *ltlApp_ZdoLeaveInd(void *vPtr)
{
  NLME_LeaveInd_t *pInd = (NLME_LeaveInd_t *)vPtr;

  log_alertln("leave ind: %x",pInd->srcAddr);

  return NULL;
}

static void hal_coilsInit(void)
{
// init coils pin
    COIL1_DDR |= COIL1_BV;
    COIL2_DDR |= COIL2_BV;    
    COIL3_DDR |= COIL3_BV;
//    MCU_IO_DIR_OUTPUT(0, 6);
//    MCU_IO_DIR_OUTPUT(1, 2);
//    MCU_IO_DIR_OUTPUT(1, 3);
    mCoilsInit();
}

static void Meter_Leave(void)
{

 NLME_LeaveReq_t leaveReq;

 osal_memset((uint8 *)&leaveReq,0,sizeof(NLME_LeaveReq_t));

 osal_memcpy(leaveReq.extAddr,NLME_GetExtAddr(),Z_EXTADDR_LEN);

 leaveReq.removeChildren = 1;

 leaveReq.rejoin = 0;

 leaveReq.silent = 0;

 NLME_LeaveReq( &leaveReq );
}


