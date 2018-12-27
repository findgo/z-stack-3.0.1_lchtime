
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

#define LightReportStatus(nodeNO, reportCmd) ltl_SendReportCmd(0x0000, LTL_TRUNK_ID_GENERAL_ONOFF, nodeNO, 0, TRUE, reportCmd);

void LigthApp_OnOffUpdate(uint8_t nodeNO, uint8_t cmd)
{
    log_debugln("node: %d,cmd: %d!", nodeNO, cmd);
    ltlReportCmd_t *reportCmd;
    ltlReport_t *reportList;

    switch (nodeNO){
#if defined(LIGHT_NODE_1)  
    case LIGHT_NODE_1: 
        COIL1_TRUN((mCoils_Mode_t)cmd);
        LIGHT1_onoff = (COIL1_STATE() > 0) ? TRUE : FALSE;
        LED1_TURN(LIGHT1_onoff);

        reportCmd =(ltlReportCmd_t *)mo_malloc(sizeof(ltlReportCmd_t) + sizeof(ltlReport_t) * 1 );
        if(reportCmd){
            reportCmd->numAttr = 1;
            reportList = &(reportCmd->attrList[0]);
            reportList->attrID = LIGHT1OnoffAttriList[0].attrId;
            reportList->dataType = LIGHT1OnoffAttriList[0].dataType;
            reportList->attrData = LIGHT1OnoffAttriList[0].dataPtr;
            
            LightReportStatus(nodeNO, reportCmd);

            mo_free(reportCmd);
        } 

        break;
#endif
#if defined(LIGHT_NODE_2)  
    case LIGHT_NODE_2: 
        COIL2_TRUN((mCoils_Mode_t)cmd);
        LIGHT2_onoff = (COIL2_STATE() > 0) ? TRUE : FALSE;
        LED2_TURN(LIGHT2_onoff);

        reportCmd =(ltlReportCmd_t *)mo_malloc(sizeof(ltlReportCmd_t) + sizeof(ltlReport_t) * 1 );
        if(reportCmd){
            reportCmd->numAttr = 1;
            reportList = &(reportCmd->attrList[0]);
            reportList->attrID = LIGHT2OnoffAttriList[0].attrId;
            reportList->dataType = LIGHT2OnoffAttriList[0].dataType;
            reportList->attrData = LIGHT2OnoffAttriList[0].dataPtr;
            
            LightReportStatus(nodeNO, reportCmd);

            mo_free(reportCmd);
        }  

        break;
#endif
#if defined(LIGHT_NODE_3)  
    case LIGHT_NODE_3: 
        COIL3_TRUN((mCoils_Mode_t)cmd);
        LIGHT3_onoff = (COIL3_STATE()  > 0) ? TRUE : FALSE;
        LED3_TURN(LIGHT3_onoff);

        reportCmd =(ltlReportCmd_t *)mo_malloc(sizeof(ltlReportCmd_t) + sizeof(ltlReport_t) * 1 );
        if(reportCmd){
            reportCmd->numAttr = 1;
            reportList = &(reportCmd->attrList[0]);
            reportList->attrID = LIGHT3OnoffAttriList[0].attrId;
            reportList->dataType = LIGHT3OnoffAttriList[0].dataType;
            reportList->attrData = LIGHT3OnoffAttriList[0].dataPtr;
            
            LightReportStatus(nodeNO, reportCmd);

            mo_free(reportCmd);
        }  

        break;
#endif
    default:
        return;
    }

    if(!ltlApp_OnNet){
        HalLedBlink(HAL_LED_1 | HAL_LED_2 | HAL_LED_3, 0, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
    }   
}

