/*
 * LPRFSensorsModule.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: sam
 */

#include "LPRFSensorsModule.h"
//
#include <algorithm>
#include "utils/z85.h"
#include "drivers/buttons.h"

// ============================================================================
LPRFSensorControllerModule::LPRFSensorControllerModule() :
    controller(0)
{
}

LPRFSensorControllerModule::~LPRFSensorControllerModule()
{
  delete controller;
}

void LPRFSensorControllerModule::init()
{
  controller = new SensorController(theBMP180Representation, theISL29023Representation,
      theSHT21Representation, theTMP006Representation);
  tivaWare.LPRF.controllerLPRF(controller);
}

void LPRFSensorControllerModule::execute()
{
  if (!tivaWare.LPRF.active)
    return;

  tivaWare.LPRF.updateLPRF();
  tivaWare.LPRF.sendLPRF();
}

// ============================================================================
LPRFSensorTargetModule::LPRFSensorTargetModule() :
    target(0)
{
}

LPRFSensorTargetModule::~LPRFSensorTargetModule()
{
  delete target;
}

void LPRFSensorTargetModule::init()
{
  target = new SensorTarget;
  tivaWare.LPRF.targetLPRF(target);
}

void LPRFSensorTargetModule::execute()
{
  if (!tivaWare.LPRF.active)
    return;

  if (theInterruptVectorRepresentation->interruptedTimer3)
  {
    //UARTprintf("target: \n", millis());
    tivaWare.LPRF.updateLPRF();
  }
}

// ============================================================================

// ============================================================================
SensorCommon::SensorCommon() :
    buffLocation(0)
{
  memset(buffer, 0, LPRF_TX_MAX_DATA_LEN * sizeof(unsigned char));
  memset(encodedStr, 0, LPRF_TX_MAX_DATA_LEN * sizeof(char));
}

// ============================================================================
SensorController::SensorController(const BMP180Representation* theBMP180Representation,
    const ISL29023Representation* theISL29023Representation,
    const SHT21Representation* theSHT21Representation,
    const TMP006Representation* theTMP006Representation) :
    theBMP180Representation(theBMP180Representation), //
    theISL29023Representation(theISL29023Representation), //
    theSHT21Representation(theSHT21Representation), //
    theTMP006Representation(theTMP006Representation)
{
}

uint8_t SensorController::size() const
{
  return buffLocation;
}

uint8_t* SensorController::data() const
{
  return (uint8_t*) encodedStr;
}

void SensorController::reset()
{
  buffLocation = 0;
}

void SensorController::receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
    uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data)
{
  // Target is simplex. Therefore, controller does not receive anything.
}

void SensorController::execute()
{
  // we have reached to the final location to send data
  buffLocation = LPRF_DATA_OFFSET_BYTES;
  repMeta.ID = TivaWareController::getInstance().BOOSTERPACK.ID;
  buffLocation += ((Serializable*) &repMeta)->writeToBuffer(buffer + buffLocation);
  buffLocation += ((Serializable*) theBMP180Representation)->writeToBuffer(buffer + buffLocation);
  buffLocation += ((Serializable*) theISL29023Representation)->writeToBuffer(buffer + buffLocation);
  buffLocation += ((Serializable*) theSHT21Representation)->writeToBuffer(buffer + buffLocation);
  buffLocation += ((Serializable*) theTMP006Representation)->writeToBuffer(buffer + buffLocation);
  buffLocation = Z85_encode_with_padding((const char*) buffer, encodedStr, buffLocation);
}

// ============================================================================
SensorTarget::SensorTarget() :
    i32IntegerPart(0), i32FractionPart(0)
{
}

uint8_t SensorTarget::size() const
{
  return buffLocation;
}

uint8_t* SensorTarget::data() const
{
  return (uint8_t*) encodedStr;
}

void SensorTarget::execute()
{
  // Target is simplex.
}

void SensorTarget::reset()
{
  buffLocation = 0;
}

void SensorTarget::receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
    uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data)
{
  buffLocation = Z85_decode_with_padding((const char*) pui8Data, (char*) buffer, ui8Length);
  if (buffLocation >= (40 + LPRF_DATA_OFFSET_BYTES))
  {
    buffLocation = LPRF_DATA_OFFSET_BYTES; // offset
    buffLocation += ((Serializable*) &repMeta)->readFromBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) &repBMP180)->readFromBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) &repISL29023)->readFromBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) &repSHT21)->readFromBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) &repTMP006)->readFromBuffer(buffer + buffLocation);

    // printf stuff
    UARTprintf("%x ", repMeta.ID);
    //
    UARTprintff(repBMP180.fTemperature);
    UARTprintff(repBMP180.fPressure);
    UARTprintff(repBMP180.fAltitude);
    //
    UARTprintff(repISL29023.fAmbient);
    //
    UARTprintff(repSHT21.fHumidity);
    UARTprintff(repSHT21.fTemperature);
    //
    UARTprintff(repTMP006.fAmbient);
    UARTprintf("\n"); // flush
    // Some delay
    ROM_SysCtlDelay(TivaWareController::getInstance().CLOCK.ui32SysClock / (1000 * 3)); // 1 ms
  }
}

void SensorTarget::UARTprintff(const float& fValue)
{
  //
  // Perform the conversion from float to a printable set of integers
  //
  i32IntegerPart = (int32_t) fValue;
  i32FractionPart = (int32_t) (fValue * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
    i32FractionPart *= -1;
  UARTprintf("%3d.%03d ", i32IntegerPart, i32FractionPart);
}
// ============================================================================

