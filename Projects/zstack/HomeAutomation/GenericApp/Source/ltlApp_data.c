
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "ltlApp.h"
#include "ltl.h"
#include "ltl_general.h"

#include "hal_led.h"

#if defined(LIGHT_NODE_1)
static uint8_t LIGHT1_onoff;
#endif
#if defined(LIGHT_NODE_2)
static uint8_t LIGHT2_onoff;
#endif
#if defined(LIGHT_NODE_3)
static uint8_t LIGHT3_onoff;
#endif

#if defined(LIGHT_NODE_1)
static const ltlAttrRec_t LIGHT1OnoffAttriList[] = {
    {
        ATTRID_ONOFF_STATUS,
        LTL_DATATYPE_BOOLEAN,
        ACCESS_CONTROL_READ,
        (void *)&LIGHT1_onoff
    },
};
#endif

#if defined(LIGHT_NODE_2)   
static const ltlAttrRec_t LIGHT2OnoffAttriList[] = {
    {
        ATTRID_ONOFF_STATUS,
        LTL_DATATYPE_BOOLEAN,
        ACCESS_CONTROL_READ,
        (void *)&LIGHT2_onoff
    },
};
#endif

#if defined(LIGHT_NODE_3)  
static const ltlAttrRec_t LIGHT3OnoffAttriList[] = {
    {
        ATTRID_ONOFF_STATUS,
        LTL_DATATYPE_BOOLEAN,
        ACCESS_CONTROL_READ,
        (void *)&LIGHT3_onoff
    },
};  
#endif

void LightAttriInit(void)
{
#if defined(LIGHT_NODE_1)  
    ltl_registerAttrList(LTL_TRUNK_ID_GENERAL_ONOFF,  LIGHT_NODE_1,  UBOUND(LIGHT1OnoffAttriList), LIGHT1OnoffAttriList);
    
#endif
#if defined(LIGHT_NODE_2)  
    ltl_registerAttrList(LTL_TRUNK_ID_GENERAL_ONOFF,  LIGHT_NODE_2,  UBOUND(LIGHT2OnoffAttriList), LIGHT2OnoffAttriList);
#endif
#if defined(LIGHT_NODE_3)  
    ltl_registerAttrList(LTL_TRUNK_ID_GENERAL_ONOFF,  LIGHT_NODE_3,  UBOUND(LIGHT3OnoffAttriList), LIGHT3OnoffAttriList);    
#endif
}

void LigthApp_OnOffUpdate(uint8_t nodeNO, uint8_t cmd)
{
    log_debugln("node: %d,cmd: %d!", nodeNO, cmd);

    switch (nodeNO){
#if defined(LIGHT_NODE_1)  
    case LIGHT_NODE_1: 
        COIL1_TRUN((mCoils_Mode_t)cmd);
        LIGHT1_onoff = COIL1_STATE();
        LED1_TURN(LIGHT1_onoff);
        break;
#endif
#if defined(LIGHT_NODE_2)  
    case LIGHT_NODE_2: 
        COIL2_TRUN((mCoils_Mode_t)cmd);
        LIGHT2_onoff = COIL2_STATE();
        LED2_TURN(LIGHT2_onoff);
        break;
#endif
#if defined(LIGHT_NODE_3)  
    case LIGHT_NODE_3: 
        COIL3_TRUN((mCoils_Mode_t)cmd);
        LIGHT3_onoff = COIL3_STATE();
        LED3_TURN(LIGHT3_onoff);
        break;
#endif
    default:
        return;
    }
}

