
#ifndef __LTL_MS_H__
#define __LTL_MS_H__

#include "ltl.h"

/*****************************************************************************/
/***    光照测量集id                                                            ***/
/*****************************************************************************/
    // Illuminance Measurement Information attribute set
#define ATTRID_MS_ILLUMINANCE_MEASURED_VALUE                             0x0000
#define ATTRID_MS_ILLUMINANCE_MIN_MEASURED_VALUE                         0x0001
#define ATTRID_MS_ILLUMINANCE_MAX_MEASURED_VALUE                         0x0002
#define ATTRID_MS_ILLUMINANCE_TOLERANCE                                  0x0003

/*****************************************************************************/
/***    光照水平感知配置集                                                          ***/
/*****************************************************************************/
    // Illuminance Level Sensing Information attribute set
#define ATTRID_MS_ILLUMINANCE_LEVEL_STATUS                               0x0000
/***  Level Status attribute values  ***/
#define MS_ILLUMINANCE_LEVEL_ON_TARGET                                   0x00
#define MS_ILLUMINANCE_LEVEL_BELOW_TARGET                                0x01
#define MS_ILLUMINANCE_LEVEL_ABOVE_TARGET                                0x02
    // Illuminance Level Sensing Settings attribute set
#define ATTRID_MS_ILLUMINANCE_TARGET_LEVEL                               0x0010

/*****************************************************************************/
/***    温度测量集                                                              ***/
/*****************************************************************************/
  // Temperature Measurement Information attributes set
#define ATTRID_MS_TEMPERATURE_MEASURED_VALUE                             0x0000
#define ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE                         0x0001
#define ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE                         0x0002
#define ATTRID_MS_TEMPERATURE_TOLERANCE                                  0x0003

/*****************************************************************************/
/***    压力测量集                                                              ***/
/*****************************************************************************/
  // Pressure Measurement Information attribute set
#define ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE                    0x0000
#define ATTRID_MS_PRESSURE_MEASUREMENT_MIN_MEASURED_VALUE                0x0001
#define ATTRID_MS_PRESSURE_MEASUREMENT_MAX_MEASURED_VALUE                0x0002
#define ATTRID_MS_PRESSURE_MEASUREMENT_TOLERANCE                         0x0003

/*****************************************************************************/
/***        流量测量集                                                          ***/
/*****************************************************************************/
  // Flow Measurement Information attribute set
#define ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE                        0x0000
#define ATTRID_MS_FLOW_MEASUREMENT_MIN_MEASURED_VALUE                    0x0001
#define ATTRID_MS_FLOW_MEASUREMENT_MAX_MEASURED_VALUE                    0x0002
#define ATTRID_MS_FLOW_MEASUREMENT_TOLERANCE                             0x0003

/*****************************************************************************/
/***        相对湿度测量集                                                        ***/
/*****************************************************************************/
  // Relative Humidity Information attribute set
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE                       0x0000
#define ATTRID_MS_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE                   0x0001
#define ATTRID_MS_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE                   0x0002
#define ATTRID_MS_RELATIVE_HUMIDITY_TOLERANCE                            0x0003

/*****************************************************************************/
/***         占有率                                                           ***/
/*****************************************************************************/
    // Occupancy Sensor Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY                     0x0000
    // PIR Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_PIR_O_TO_U_DELAY              0x0010
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_PIR_U_TO_O_DELAY              0x0011
    // Ultrasonic Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_ULTRASONIC_O_TO_U_DELAY       0x0020
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_ULTRASONIC_U_TO_O_DELAY       0x0021

typedef void (*ltlMS_PlaceHolder_t)( void );

// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct			
{
  ltlMS_PlaceHolder_t               pfnMSPlaceHolder; // Place Holder
} LtlMs_AppCallbacks_t;


LStatus_t ltlMs_RegisterCmdCallBacks(LtlMs_AppCallbacks_t *callbacks);

#endif
