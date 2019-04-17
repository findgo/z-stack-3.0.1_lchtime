

#include "ltl_general.h"


// local function
static LStatus_t ltlGeneral_SpecificTrunckhandle( ltlApduMsg_t *ApduMsg );
static LStatus_t ltlGeneral_ProcessSpecificInbasic( ltlApduMsg_t *ApduMsg, ltlGeneral_AppCallbacks_t *pCB);
static LStatus_t ltlGeneral_ProcessSpecificInonoff( ltlApduMsg_t *ApduMsg, ltlGeneral_AppCallbacks_t *pCB);
static LStatus_t ltlGeneral_ProcessSpecificInLevelControl( ltlApduMsg_t *ApduMsg, ltlGeneral_AppCallbacks_t *pCB);

// local variable
static uint8_t ltlGeneralRegisted = FALSE;
static ltlGeneral_AppCallbacks_t *ltlGeneralCBs = NULL;

/*********************************************************************
 * @brief   注册通用集下所有集命令解析回调
 *
 * @param   callbacks - in --- callback list
 *
 * @return 
 */
LStatus_t ltlGeneral_RegisterCmdCallBacks(ltlGeneral_AppCallbacks_t *callbacks)
{
    if(ltlGeneralRegisted == FALSE){
        
        ltl_registerPlugin(LTL_TRUNK_ID_GENERAL_BASIC, LTL_TRUNK_ID_GENERAL_LEVEL_CONTROL, ltlGeneral_SpecificTrunckhandle);
        ltlGeneralCBs = callbacks;

        ltlGeneralRegisted = TRUE;
    }

    return LTL_SUCCESS;
}


/*********************************************************************
 * @brief   通用集下所有集命令解析的回调
 *
 * @param   ApduMsg - in --- apdu message
 *
 * @return 
 */
static LStatus_t ltlGeneral_SpecificTrunckhandle( ltlApduMsg_t *ApduMsg )
{
    LStatus_t status = LTL_STATUS_INVALID_FIELD;

    if( ltl_IsTrunkCmd(ApduMsg->hdr.fc.type) ){
        if(ltlGeneralCBs == NULL)
            return LTL_STATUS_FAILURE;
        // gerneral specific trunk process add here         
        switch (ApduMsg->hdr.trunkID){
        case LTL_TRUNK_ID_GENERAL_BASIC:
            status = ltlGeneral_ProcessSpecificInbasic(ApduMsg, ltlGeneralCBs);
            break;
        case LTL_TRUNK_ID_GENERAL_ONOFF:
            status = ltlGeneral_ProcessSpecificInonoff(ApduMsg, ltlGeneralCBs);
            break;
        case LTL_TRUNK_ID_GENERAL_LEVEL_CONTROL:
            status = ltlGeneral_ProcessSpecificInLevelControl(ApduMsg, ltlGeneralCBs);
            break;


             
         default:
            status = LTL_STATUS_FAILURE;
            break;
        }           
    }
    else{
        status = LTL_STATUS_FAILURE;
        //should not go here

    }

    return(status);
}
/*********************************************************************
 * @brief   通用集下 基本集命令解析回调
 *
 * @param   ApduMsg - in --- apdu message
 *
 * @return 
 */
static LStatus_t ltlGeneral_ProcessSpecificInbasic( ltlApduMsg_t *ApduMsg , ltlGeneral_AppCallbacks_t *pCB)
{
    uint32_t tmpvalue;

    switch( ApduMsg->hdr.commandID ){
    case COMMAND_BASIC_RESET_FACT_DEFAULT:
        if(pCB->pfnBasicResetFact)
            pCB->pfnBasicResetFact(ApduMsg->hdr.nodeNo);
        break;
        
    case COMMAND_BASIC_REBOOT_DEVICE:
        if(pCB->pfnBasicReboot)
            pCB->pfnBasicReboot();
        break;
        
    case COMMAND_BASIC_IDENTIFY:
        tmpvalue = BUILD_UINT16(ApduMsg->pdata[0], ApduMsg->pdata[1]);
        if(pCB->pfnIdentify)
            pCB->pfnIdentify((uint16_t)tmpvalue);
        break;
        
    default:
        return LTL_STATUS_FAILURE;
        //break;
    }

    return LTL_STATUS_CMD_HAS_RSP;
}
/*********************************************************************
 * @brief   通用集下 onoff命令解析回调
 *
 * @param   ApduMsg - in --- apdu message
 *
 * @return 
 */
static LStatus_t ltlGeneral_ProcessSpecificInonoff( ltlApduMsg_t *ApduMsg, ltlGeneral_AppCallbacks_t *pCB)
{
    if(ApduMsg->hdr.commandID > COMMAND_ONOFF_TOGGLE){
        return LTL_STATUS_FAILURE;
    }
    else{
        if(pCB->pfnOnoff)
            pCB->pfnOnoff(ApduMsg->hdr.nodeNo, ApduMsg->hdr.commandID);
    }

    return LTL_STATUS_SUCCESS;
}

/*********************************************************************
 * @brief   通用集下 onoff命令解析回调
 *
 * @param   ApduMsg - in --- apdu message
 *
 * @return 
 */
static LStatus_t ltlGeneral_ProcessSpecificInLevelControl( ltlApduMsg_t *ApduMsg, ltlGeneral_AppCallbacks_t *pCB)
{
    uint8_t withOnOff = FALSE;
    union {
        ltlLCMoveTolevel_t LCMTL;
        ltlLCMove_t lcM;
        ltlLCStep_t lcS;
    }cmd;
    
    switch (ApduMsg->hdr.commandID){
    case COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF:
        withOnOff = TRUE;
    case COMMAND_LEVEL_MOVE_TO_LEVEL:
        if(pCB->pfnLevelControlMoveToLevel){
            cmd.LCMTL.withOnOff = withOnOff;
            cmd.LCMTL.level = ApduMsg->pdata[0];
            pCB->pfnLevelControlMoveToLevel(ApduMsg->hdr.nodeNo, &(cmd.LCMTL));
        }
        break;

    case COMMAND_LEVEL_MOVE_WITH_ON_OFF:
        withOnOff = TRUE;
    case COMMAND_LEVEL_MOVE:
        if(pCB->pfnLevelControlMove){
            cmd.lcM.withOnOff = withOnOff;
            cmd.lcM.moveMode = ApduMsg->pdata[0];           
            cmd.lcM.rate = ApduMsg->pdata[1];
            pCB->pfnLevelControlMove(ApduMsg->hdr.nodeNo, &(cmd.lcM));
        }
        break;
    case COMMAND_LEVEL_STEP_WITH_ON_OFF:
        withOnOff = TRUE;
    case COMMAND_LEVEL_STEP:
        if(pCB->pfnLevelControlStep){
            cmd.lcS.withOnOff = withOnOff;
            cmd.lcS.stepMode = ApduMsg->pdata[0];           
            cmd.lcS.stepSize = ApduMsg->pdata[1];
            pCB->pfnLevelControlStep(ApduMsg->hdr.nodeNo, &(cmd.lcS));
        }
        break;

    case COMMAND_LEVEL_STOP_WITH_ON_OFF:
    case COMMAND_LEVEL_STOP:
        if(pCB->pfnLevelControlStop)
            pCB->pfnLevelControlStop(ApduMsg->hdr.nodeNo);
        break;
    
    default:
        return LTL_STATUS_FAILURE;
    }


    return LTL_STATUS_SUCCESS;
}
