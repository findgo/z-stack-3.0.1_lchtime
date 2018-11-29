

#ifndef __LOW_NWK_H__
#define __LOW_NWK_H__

#include "AF.h"
#include "aps_groups.h"
#include "log.h"
#include "bxnwk.h"


extern byte LowNwk_TaskID;

extern void LowNwk_Init( uint8 task_id );
extern uint16 LowNwk_event_loop( uint8 task_id, uint16 events );
extern uint8 LowNwk_registerForMsg( uint8 taskId );
extern uint8 LowNwk_registerForMsgExt( uint8 taskId, uint8 endPointId  );


#endif
