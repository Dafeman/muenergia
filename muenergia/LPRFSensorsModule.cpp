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

static bool g_controllerSendActivation = false;

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

void LPRFSensorControllerModule::update(SensorAccessRepresentation& theSensorAccessRepresentation)
{
  static unsigned long prevTime = millis();
  static int state = 0;

  if (!tivaWare.LPRF.active)
    return;

  tivaWare.LPRF.updateLPRF();

  if (!LPRF::getInstance().getConnected() && theInterruptVectorRepresentation->interruptedSysTick)
    return;

  // preamble
  if (!controller->contentMsg && theInterruptVectorRepresentation->interruptedSysTick)
  {
    if (g_controllerSendActivation)
    {
      UARTprintf("preamble set!\n");
      controller->contentMsg = true;
    }
    else
    {
      if ((millis() - prevTime) > 3000)
      {
        tivaWare.LPRF.sendLPRF();
        UARTprintf("sending preamble: \n");
        prevTime = millis();
      }
      return;
    }
  }

  if (!controller->contentMsg)
    return;

  if (state == 0 && g_controllerSendActivation)
  {
    UARTprintf("received activation\n");
    prevTime = millis();
    ++state;
  }

  if (state == 1 && ((millis() - prevTime) > 1000))
  {
    ++state;
    prevTime = millis();
  }

  if (state == 2 && ((millis() - prevTime) > 1000))
  {
    ++state;
    theSensorAccessRepresentation.active = true;
    prevTime = millis();
    UARTprintf("reading sensor data\n");
  }

  if (state == 3 && ((millis() - prevTime) > 3000)) // send data
  {
    theSensorAccessRepresentation.active = false;
    if (g_controllerSendActivation)
    {
      UARTprintf("sending sensor data ...\n");
      tivaWare.LPRF.sendLPRF();
      g_controllerSendActivation = false; // fixme
    }
    ++state;
  }

  if ((state == 4) && (millis() - prevTime) > 2000)
  {
    UARTprintf("ready for next data\n");
    state = 0;
  }
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
  target = new SensorTarget(theInterruptVectorRepresentation);
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
    tivaWare.LPRF.sendLPRF();
  }
}

// ============================================================================

// ============================================================================
SensorCommon::SensorCommon() :
    buffLocation(0), receiveLocation(0), sendLocation(0), syncPktLength(
        sizeof(long int) + sizeof(bool))
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
    theTMP006Representation(theTMP006Representation), //
    contentMsg(false)
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
  receiveLocation = Z85_decode_with_padding((const char*) pui8Data, (char*) buffer, ui8Length);
  if (receiveLocation >= syncPktLength)
  {
    ((Serializable*) &repSync)->readFromBuffer(buffer);
    UARTprintf("SensorController mills: %d sendActivation: %d\n", repSync.ms,
        repSync.sendActivation);
    //g_controllerSendActivation = repSync.sendActivation;
    g_controllerSendActivation = true;
  }
  receiveLocation = 0;
}

void SensorController::execute()
{
  // we have reached to the final location to send data
  buffLocation = LPRF_DATA_OFFSET_BYTES;
  repMeta.ID = TivaWareController::getInstance().BOOSTERPACK.ID;
  buffLocation += ((Serializable*) &repMeta)->writeToBuffer(buffer + buffLocation);
  if (contentMsg)
  {
    buffLocation += ((Serializable*) theBMP180Representation)->writeToBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) theISL29023Representation)->writeToBuffer(
        buffer + buffLocation);
    buffLocation += ((Serializable*) theSHT21Representation)->writeToBuffer(buffer + buffLocation);
    buffLocation += ((Serializable*) theTMP006Representation)->writeToBuffer(buffer + buffLocation);
  }
  buffLocation = Z85_encode_with_padding((const char*) buffer, encodedStr, buffLocation);
}

// ============================================================================
SensorTarget::SensorTarget(const InterruptVectorRepresentation* theInterruptVectorRepresentation) :
    theInterruptVectorRepresentation(theInterruptVectorRepresentation), i32IntegerPart(0), //
    i32FractionPart(0)
{
}

uint8_t SensorTarget::size() const
{
  return sendLocation;
}

uint8_t* SensorTarget::data() const
{
  return (uint8_t*) encodedStr;
}

void SensorTarget::execute()
{
  switch (theInterruptVectorRepresentation->buttons & ALL_BUTTONS)
  {
    //
    // Right button is pressed at startup.
    //
    case RIGHT_BUTTON:
    {
      TivaWareController::getInstance().LED.colorSetRGB(0x0, 0x4000, 0x4000);
      repSync.sendActivation = true;
      break;
    }

    case LEFT_BUTTON:
    {
      TivaWareController::getInstance().LED.colorSetRGB(0x4000, 0x4000, 0x0);
      repSync.sendActivation = false;
      repSync.resetActivation = true;
      UARTprintf("EOE\n"); // end-of-episode
      break;
    }

  }

  repSync.ms = millis();
  sendLocation = ((Serializable*) &repSync)->writeToBuffer(buffer);
  sendLocation = Z85_encode_with_padding((const char*) buffer, encodedStr, sendLocation);
  repSync.resetActivation = false;
  //UARTprintf("SensorTarget::execute()\n");
}

void SensorTarget::reset()
{
  sendLocation = 0;
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
    UARTprintf(" %u ", millis());
    UARTprintf("\n"); // flush
    // Some delay
    ROM_SysCtlDelay(TivaWareController::getInstance().CLOCK.ui32SysClock / (100 * 3)); // 10 ms
  }
  else
  {
    UARTprintf("msg(%d): %d \n", ui8SrcIndex, ui8Length);
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


