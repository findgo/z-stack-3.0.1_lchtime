
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
#include "ltl_ms.h"
#include "hal_led.h"

static uint16_t temperature;
static uint16_t humidity;

static const ltlAttrRec_t TemperatureAttriList[] = {
    {
        ATTRID_MS_TEMPERATURE_MEASURED_VALUE,
        LTL_DATATYPE_UINT16,
        ACCESS_CONTROL_READ,
        (void *)&temperature
    },
};
static const ltlAttrRec_t HumidityAttriList[] = {
{
    ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE,
    LTL_DATATYPE_UINT16,
    ACCESS_CONTROL_READ,
    (void *)&humidity
},
};
void TempperatureSensorAttriInit(void)
{
    ltl_registerAttrList(LTL_TRUNK_ID_MS_TEMPERATURE_MEASUREMENT,  TEMP_NODE_1,  UBOUND(TemperatureAttriList), TemperatureAttriList);
    ltl_registerAttrList(LTL_TRUNK_ID_MS_RELATIVE_HUMIDITY,  HUMI_NODE_2,  UBOUND(HumidityAttriList), HumidityAttriList);
}
