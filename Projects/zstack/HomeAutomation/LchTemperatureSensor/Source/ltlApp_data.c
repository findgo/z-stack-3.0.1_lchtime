
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
#include "SHT2x.h"

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
void TemperatureSensorAttriInit(void)
{
    ltl_registerAttrList(LTL_TRUNK_ID_MS_TEMPERATURE_MEASUREMENT,  TEMP_NODE,  UBOUND(TemperatureAttriList), TemperatureAttriList);
    ltl_registerAttrList(LTL_TRUNK_ID_MS_RELATIVE_HUMIDITY,  HUMI_NODE,  UBOUND(HumidityAttriList), HumidityAttriList);
}


void TemperatureSensorReport(void)
{
  ltlReportCmd_t *reportCmd;
  ltlReport_t *reportList;
  SHT2x_info_t *info;
  
  SHT2x_Measure(TRIG_TEMP_MEASUREMENT_POLL);//获取SHT20 温度
  SHT2x_Measure(TRIG_HUMI_MEASUREMENT_POLL);//获取SHT20 湿度
  info = SHT2x_GetInfo();
  temperature = (uint16_t)(info->TEMP *100);
  humidity = (uint16_t)(info->HUMI*100);
  
  reportCmd =(ltlReportCmd_t *)mo_malloc(sizeof(ltlReportCmd_t) + sizeof(ltlReport_t) * 1 );
  if(!reportCmd){
        log_noticeln("no report it");
        return;
  }
  
  reportCmd->numAttr = 1;
  reportList = &(reportCmd->attrList[0]);
  reportList->attrID = TemperatureAttriList[0].attrId;
  reportList->dataType = TemperatureAttriList[0].dataType;
  reportList->attrData = TemperatureAttriList[0].dataPtr;
        
  ltl_SendReportCmd(0x0000, LTL_TRUNK_ID_MS_TEMPERATURE_MEASUREMENT, TEMP_NODE, 0, TRUE, reportCmd);

  reportCmd->numAttr = 1;
  reportList = &(reportCmd->attrList[0]);
  reportList->attrID = HumidityAttriList[0].attrId;
  reportList->dataType = HumidityAttriList[0].dataType;
  reportList->attrData = HumidityAttriList[0].dataPtr;      

  ltl_SendReportCmd(0x0000, LTL_TRUNK_ID_MS_RELATIVE_HUMIDITY, HUMI_NODE, 0, TRUE, reportCmd);

  mo_free(reportCmd);
  log_noticeln("report it");
}
