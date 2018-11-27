

#include "ltl_ms.h"

static LStatus_t ltlMs_SpecificTrunckhandle( ltlApduMsg_t *ApduMsg );


// local variable
static uint8_t ltlMsRegisted = FALSE;
static LtlMs_AppCallbacks_t *ltlMsCBs = NULL;

LStatus_t ltlMs_RegisterCmdCallBacks(LtlMs_AppCallbacks_t *callbacks)
{

    // Register as a LTL Plugin
    if ( !ltlMsRegisted ){
        ltl_registerPlugin(LTL_TRUNK_ID_MS_ILLUMINANCE_MEASUREMENT, LTL_TRUNK_ID_MS_OCCUPANCY_SENSING, ltlMs_SpecificTrunckhandle);
        ltlMsCBs = callbacks;
    
        ltlMsRegisted = TRUE;
    }

    return ( LTL_STATUS_SUCCESS );
}

static LStatus_t ltlMs_SpecificTrunckhandle( ltlApduMsg_t *ApduMsg )
{
    LStatus_t status = LTL_STATUS_SUCCESS;

    if(ltl_IsTrunkCmd(ApduMsg->hdr.fc.type)){
        if(ltlMsCBs == NULL)
            return LTL_STATUS_FAILURE;
        
        switch (ApduMsg->hdr.trunkID){
        case LTL_TRUNK_ID_MS_ILLUMINANCE_MEASUREMENT:
//                if(ltlMsCBs->pfnMSPlaceHolder)
//                    status = ltlMsCBs->pfnMSPlaceHolder(ApduMsg->hdr.nodeNo);
//                break;
        case LTL_TRUNK_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG:
        case LTL_TRUNK_ID_MS_TEMPERATURE_MEASUREMENT:
        case LTL_TRUNK_ID_MS_PRESSURE_MEASUREMENT:
        case LTL_TRUNK_ID_MS_FLOW_MEASUREMENT:
        case LTL_TRUNK_ID_MS_RELATIVE_HUMIDITY:
        case LTL_TRUNK_ID_MS_OCCUPANCY_SENSING:
        default:
           status = LTL_STATUS_FAILURE;
           break;
        }       
    }
    else{
        status = LTL_STATUS_FAILURE;
    }

    return status;
}


