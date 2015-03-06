/*
 * SensorHubBoosterPack.cpp
 *
 *  Created on: Sep 13, 2014
 *      Author: sam
 */

#include "BMP180Module.h"
#include "ISL29023Module.h"
#include "MPU9150Module.h"
#include "SHT21Module.h"
#include "TMP006Module.h"
#include "LPRFModule.h"
#include "LPRFSensorsModule.h"
#include "BQ27510G3Module.h"
//

//#define LPRF_SENSOR_TARGET
#define LPRF_SENSOR_CONTROLLER

#ifdef LPRF_SENSOR_TARGET
MAKE_MODULE(LPRFSensorTargetModule)
#endif

#ifdef LPRF_SENSOR_CONTROLLER
MAKE_MODULE(BMP180Module)
MAKE_MODULE(ISL29023Module)
MAKE_MODULE(SHT21Module) // This sensor is very slow
MAKE_MODULE(TMP006Module) //
MAKE_MODULE(LPRFSensorControllerModule);
#endif

// others
//MAKE_MODULE(MPU9150Module)
//MAKE_MODULE(LPRFModule) // This is the radio connected to EM headers
//MAKE_MODULE(BQ27510G3Module)
