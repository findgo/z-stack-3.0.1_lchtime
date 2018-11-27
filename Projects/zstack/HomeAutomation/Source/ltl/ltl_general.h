#ifndef __LTL_GENERAL_H__
#define __LTL_GENERAL_H__

#include "ltl.h"

/*************************attribute******************************************************************************/
/*** Basic Attribute***/
#define ATTRID_BASIC_LTL_VERSION        0x0000
#define ATTRID_BASIC_APPL_VERSION       0x0001
#define ATTRID_BASIC_HW_VERSION         0x0002
#define ATTRID_BASIC_MANUFACTURER_NAME  0x0003
#define ATTRID_BASIC_BUILDDATE_CODE     0x0004
#define ATTRID_BASIC_PRODUCT_ID         0x0005
#define ATTRID_BASIC_SERIAL_NUMBER      0x0006
#define ATTRID_BASIC_POWER_SOURCE       0x0007

// for basic power source
#define POWERSOURCE_UNKOWN              0x00
#define POWERSOURCE_SINGLE_PHASE        0x01
#define POWERSOURCE_THREE_PHASE         0x02
#define POWERSOURCE_DC                  0x03
#define POWERSOURCE_BATTERY             0x04
#define POWERSOURCE_EMERGENCY           0x05
// Bit b7 indicates whether the device has a secondary power source in the form of a battery backup

/*** Power Source Attribute bits  ***/
#define POWER_SOURCE_PRIMARY                              0x7F
#define POWER_SOURCE_SECONDARY                            0x80

/*** Basic Trunck Commands ***/
#define COMMAND_BASIC_RESET_FACT_DEFAULT 0x00
#define COMMAND_BASIC_REBOOT_DEVICE     0x01
#define COMMAND_BASIC_IDENTIFY          0x02

/*** On/Off Switch***/
#define ATTRID_ONOFF_STATUS              0x0000
/*** On/Off Switch Trunck Commands ***/
#define COMMAND_ONOFF_OFF               0x00
#define COMMAND_ONOFF_ON                0x01
#define COMMAND_ONOFF_TOGGLE            0x02

/*** Level Control  ***/
#define ATTRID_LEVEL_CURRENT_LEVEL      0x0000
/*** Level Control Commands ***/
#define COMMAND_LEVEL_MOVE_TO_LEVEL                       0x00
#define COMMAND_LEVEL_MOVE                                0x01
#define COMMAND_LEVEL_STEP                                0x02
#define COMMAND_LEVEL_STOP                                0x03
#define COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF           0x04
#define COMMAND_LEVEL_MOVE_WITH_ON_OFF                    0x05
#define COMMAND_LEVEL_STEP_WITH_ON_OFF                    0x06
#define COMMAND_LEVEL_STOP_WITH_ON_OFF                    0x07

/*** Level Control Move (Mode) Command values ***/
#define LEVEL_MOVE_UP                                     0x00
#define LEVEL_MOVE_DOWN                                   0x01

/*** Level Control Step (Mode) Command values ***/
#define LEVEL_STEP_UP                                     0x00
#define LEVEL_STEP_DOWN                                   0x01



typedef struct
{
    uint8_t level;
    uint8_t withOnOff;
}ltlLCMoveTolevel_t;

typedef struct
{
    uint8_t moveMode;
    uint8_t rate;
    uint8_t withOnOff;
}ltlLCMove_t;

typedef struct
{
    uint8_t stepMode;
    uint8_t stepSize;      // rate of movement in steps per second
    uint8_t withOnOff;
}ltlLCStep_t;

// This callback is called to process an Reset to Factory Defaultscommand. 
// On receipt of this command, the device resets all the attributes of all its clusters to their factory defaults.
typedef void(*ltlGenCB_BasicResetFact_t) (uint8_t nodeNo);
// This callback is called to process an reboot the device. 
typedef void(*ltlGenCB_BasicReboot_t)(void);
// This callback is called to process an incoming Identify command.
//   identifyTime - number of miniseconds the device will continue to identify itself
typedef void (*ltlGenCB_Identify_t)( uint16_t identifyTime );
// This callback is called to process an incoming On, Off or Toggle command.
//  nodeNO: node number 
//   cmd - received command, which will be either COMMAND_ONOFF_ON, COMMAND_ONOFF_OFF
//         or COMMAND_ONOFF_TOGGLE.
typedef void (*ltlGenCB_Onoff_t)(uint8_t nodeNO, uint8_t cmd);
typedef void (*ltlGenCB_LevelControlMoveToLevel_t)( uint8_t node, ltlLCMoveTolevel_t *pCmd );

// This callback is called to process a Level Control - Move command
//   moveMode - move mode which is either LEVEL_MOVE_STOP, LEVEL_MOVE_UP,
//              LEVEL_MOVE_ON_AND_UP, LEVEL_MOVE_DOWN, or LEVEL_MOVE_DOWN_AND_OFF
//   rate - rate of movement in steps per second.
//   withOnOff - with On/off command
typedef void (*ltlGenCB_LevelControlMove_t)( uint8_t node, ltlLCMove_t *pCmd );

// This callback is called to process a Level Control - Step command
//   stepMode - step mode which is either LEVEL_STEP_UP, LEVEL_STEP_ON_AND_UP,
//              LEVEL_STEP_DOWN, or LEVEL_STEP_DOWN_AND_OFF
//   amount - number of levels to step
//   transitionTime - time, in 1/10ths of a second, to take to perform the step
//   withOnOff - with On/off command
typedef void (*ltlGenCB_LevelControlStep_t)( uint8_t node, ltlLCStep_t *pCmd );

// This callback is called to process a Level Control - Stop command
typedef void (*ltlGenCB_LevelControlStop_t)(uint8_t node );



typedef struct
{
    ltlGenCB_BasicResetFact_t           pfnBasicResetFact;                // Basic trunk Reset command
    ltlGenCB_BasicReboot_t              pfnBasicReboot;                // Basic trunk reboot command
    ltlGenCB_Identify_t                 pfnIdentify;                  // Identify command
    ltlGenCB_Onoff_t                    pfnOnoff;                       // on off switch 
    ltlGenCB_LevelControlMoveToLevel_t  pfnLevelControlMoveToLevel;   // Level Control Move to Level command
    ltlGenCB_LevelControlMove_t         pfnLevelControlMove;          // Level Control Move command
    ltlGenCB_LevelControlStep_t         pfnLevelControlStep;          // Level Control Step command
    ltlGenCB_LevelControlStop_t         pfnLevelControlStop;          // Level Control Stop command
} ltlGeneral_AppCallbacks_t;



LStatus_t ltlGeneral_RegisterCmdCallBacks(ltlGeneral_AppCallbacks_t *callbacks);


#endif 

