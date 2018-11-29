
#ifndef _LTL_App_H__
#define _LTL_App_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "log.h" 


#define TEMP_NODE  1
#define HUMI_NODE  2


#define LTLAPP_DEVICE_REJOIN_EVT    0x0001
#define LTLAPP_DEVICE_RECOVER_EVT   0x0002
#define LTLAPP_DEVICE_LEAVE_TIMEOUT_EVT 0x0004
#define LTLAPP_DEVICE_MEASURE_EVT  0x0008
#define LTLAPP_TEST_EVT             0x0100

#define LTLAPP_END_DEVICE_REJOIN_DELAY      10000
#define LTLAPP_DEVICE_LEAVE_TIME_DELAY      2000
#define LTLAPP_DEVICE_MEASURE_TIME_DELAY    30000
#define LED1_TURN(state)       HalLedSet(HAL_LED_1, (state > 0) ? HAL_LED_MODE_ON : HAL_LED_MODE_OFF)

/*********************************************************************
 * MACROS
 */

/* 自定义双向传输 API传输方式 */
#define LCHTIMEAPP_CLUSTERID        0xabcd
// device's needs
#define LCHTIMEAPP_ENDPOINT           0xaa

#define LCHTIMEAPP_PROFID             0xabcd
#define LCHTIMEAPP_DEVICEID           0xabcd
#define LCHTIMEAPP_DEVICE_VERSION     1
#define LCHTIMEAPP_FLAGS              0

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern uint8_t ltlApp_OnNet;
extern SimpleDescriptionFormat_t ltlApp_SimpleDesc;

// ltlApp_TODO: Declare application specific attributes here


/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void ltlApp_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 ltlApp_event_loop( byte task_id, UINT16 events );

extern void TemperatureSensorAttriInit(void);
extern void TemperatureSensorReport(void);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _ltlApp_H */
