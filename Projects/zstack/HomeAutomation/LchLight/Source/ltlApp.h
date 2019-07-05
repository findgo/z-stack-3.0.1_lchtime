
#ifndef _LTL_App_H__
#define _LTL_App_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "log.h" 
#include "mcoils.h"


#define LIGHT_NODE_1  1
//#define LIGHT_NODE_2  2
//#define LIGHT_NODE_3  3


#define LTLAPP_DEVICE_REJOIN_EVT    0x0001
#define LTLAPP_DEVICE_RECOVER_EVT   0x0002
#define LTLAPP_DEVICE_LEAVE_TIMEOUT_EVT 0x0004
#define LTLAPP_TEST_EVT             0x0100

#define LTLAPP_END_DEVICE_REJOIN_DELAY 10000
#define LTLAPP_DEVICE_LEAVE_TIME_DELAY 2000
#define LIGHT_OFF                       0x00
#define LIGHT_ON                        0x01

#define COIL1_TRUN(mode)        mCoilsSet(MCOILS_1, mode)
#define COIL1_TOGGLE()          mCoilsSet(MCOILS_1, MCOILS_MODE_TOGGLE)
#define COIL1_STATE()           mCoilsGetStasus(MCOILS_1)
#define COIL2_TRUN(mode)        mCoilsSet(MCOILS_2, mode)
#define COIL2_TOGGLE()          mCoilsSet(MCOILS_2, MCOILS_MODE_TOGGLE)
#define COIL2_STATE()           mCoilsGetStasus(MCOILS_2)
#define COIL3_TRUN(mode)        mCoilsSet(MCOILS_3,mode)
#define COIL3_TOGGLE()          mCoilsSet(MCOILS_3, MCOILS_MODE_TOGGLE)
#define COIL3_STATE()           mCoilsGetStasus(MCOILS_3)

#define LED1_TURN(state)       HalLedSet(HAL_LED_1, (state > 0) ? HAL_LED_MODE_ON : HAL_LED_MODE_OFF)
#define LED2_TURN(state)       HalLedSet(HAL_LED_2, (state > 0) ? HAL_LED_MODE_ON : HAL_LED_MODE_OFF)
#define LED3_TURN(state)       HalLedSet(HAL_LED_3, (state > 0) ? HAL_LED_MODE_ON : HAL_LED_MODE_OFF)

#define LED_BLINK()      HalLedSet(HAL_LED_1 | HAL_LED_2 | HAL_LED_3, HAL_LED_MODE_FLASH)

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

extern void LightAttriInit(void);
void LigthApp_OnOffUpdate(uint8_t nodeNO, uint8_t cmd);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _ltlApp_H */
