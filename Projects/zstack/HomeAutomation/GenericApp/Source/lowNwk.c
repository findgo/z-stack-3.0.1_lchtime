

#include "lowNwk.h"
#include "ZComDef.h"
#include "AF.h"
#include "OSAL_Tasks.h"
#include "bdb_interface.h"

// List record for external handler for unhandled ZCL Foundation commands/rsps
typedef struct nwkExternalFoundationHandlerList
{
    struct nwkExternalFoundationHandlerList *next;
    uint8 nwk_ExternalTaskID;
    uint8 nwk_ExternalEndPoint;
} nwkExternalFoundationHandlerList;

static nwkExternalFoundationHandlerList *NwkexternalEndPointHandlerList = (nwkExternalFoundationHandlerList *)NULL;
static uint8 LowNwk_addExternalFoundationHandler( uint8 taskId, uint8 endPointId  );
static uint8 LowNwk_getExternalFoundationHandler( afIncomingMSGPacket_t *pInMsg );



byte LowNwk_TaskID;

void LowNwk_Init( uint8 task_id )
{
  LowNwk_TaskID = task_id;
}


 /* 所有的消息处理都先发送的这里来处理,如果需要处理一些其它层面的消息,调用下面的LowNwk_registerForMsg或 LowNwk_registerForMsgExt*/
/* 另外bdb_RegisterSimpleDescriptor 就是注册对应端点, 只有注册了对应端点,才会在这里收到对应AF层来的消息*/
uint16 LowNwk_event_loop( uint8 task_id, uint16 events )
{
  uint8 *msgPtr;

  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    msgPtr = osal_msg_receive( LowNwk_TaskID );
    while ( msgPtr != NULL )
    {
      uint8 dealloc = TRUE;

      log_alertln("lownwk(%x) receive a message!",*msgPtr);
      if ( *msgPtr == AF_INCOMING_MSG_CMD )
      {
        if(((afIncomingMSGPacket_t *)msgPtr)->srcAddr.addrMode == afAddr16Bit) 
            bxNwk_ProcessMessageMSG( (afIncomingMSGPacket_t *)msgPtr );
      }
      else
      {
        uint8 taskID;
        taskID = LowNwk_getExternalFoundationHandler( (afIncomingMSGPacket_t *)msgPtr );

        if ( taskID != TASK_NO_TASK )
        {
          // send it to another task to process.
          osal_msg_send( taskID, msgPtr );
          dealloc = FALSE;
       }
      }

      // Release the memory
      if ( dealloc )
      {
        osal_msg_deallocate( msgPtr );
      }

      // Next
      msgPtr = osal_msg_receive( LowNwk_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  // Discard unknown events
  return 0;
}


uint8 LowNwk_registerForMsg( uint8 taskId )
{
  return LowNwk_addExternalFoundationHandler( taskId, AF_BROADCAST_ENDPOINT );
}

uint8 LowNwk_registerForMsgExt( uint8 taskId, uint8 endPointId  )
{
  return ( LowNwk_addExternalFoundationHandler( taskId, endPointId  ) );
}
static uint8 LowNwk_addExternalFoundationHandler( uint8 taskId, uint8 endPointId  )
{
  nwkExternalFoundationHandlerList *pNewItem;
  nwkExternalFoundationHandlerList *pLoop;
  nwkExternalFoundationHandlerList *pLoopPrev;

  // Fill in the new endpoint registrant list
  pNewItem = osal_mem_alloc( sizeof( nwkExternalFoundationHandlerList ) );
  if ( pNewItem == NULL )
  {
    return ( false );
  }

  pNewItem->nwk_ExternalEndPoint = endPointId;
  pNewItem->nwk_ExternalTaskID = taskId;
  pNewItem->next = NULL;

  // Add to the list
  if ( NwkexternalEndPointHandlerList == NULL )
  {
    NwkexternalEndPointHandlerList = pNewItem;
  }
  else
  {
    // make sure no one else tried to register for this endpoint
    pLoop = NwkexternalEndPointHandlerList;
    pLoopPrev = NwkexternalEndPointHandlerList;

    while ( pLoop != NULL )
    {
      if ( ( pLoop->nwk_ExternalEndPoint ) == endPointId )
      {
        osal_mem_free(pNewItem);
        return ( false );
      }
      pLoopPrev = pLoop;
      pLoop = pLoop->next;
    }

    if ( endPointId == AF_BROADCAST_ENDPOINT )
    {
      // put new registration at the end of the list
      pLoopPrev->next = pNewItem;
      pNewItem->next = NULL;
    }
    else
    {
      // put new registration at the front of the list
      nwkExternalFoundationHandlerList *temp = NwkexternalEndPointHandlerList;
      NwkexternalEndPointHandlerList = pNewItem;
      pNewItem->next = temp;
    }
  }

  return ( true );

}
static uint8 LowNwk_getExternalFoundationHandler( afIncomingMSGPacket_t *pInMsg )
{
  nwkExternalFoundationHandlerList *pLoop;
  uint8 addressedEndPointId = pInMsg->endPoint;

  // make sure no one else tried to register for this endpoint
  pLoop = NwkexternalEndPointHandlerList;

  while ( pLoop != NULL )
  {
    if ( ( ( pLoop->nwk_ExternalEndPoint ) == addressedEndPointId ) ||
         ( ( pLoop->nwk_ExternalEndPoint ) == AF_BROADCAST_ENDPOINT ) )
    {
      return ( pLoop->nwk_ExternalTaskID );
    }
    pLoop = pLoop->next;
  }

  return ( TASK_NO_TASK );
}


