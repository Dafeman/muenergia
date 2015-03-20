/*
 * LPRFModule.cpp
 *
 *  Created on: Sep 13, 2014
 *      Author: sam
 */

#include "LPRFModule.h"
//
#include <algorithm>
#include "utils/z85.h"
#include "drivers/buttons.h"
//
// Global variable
static bool g_sendActivation = false;
static bool g_startPersistanceStorage = false;
static bool g_resetActivation = false;

LPRFModule::LPRFModule() :
    sendLoop(false)
{
}

void LPRFModule::init()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  tivaWare.LPRF.targetLPRF(&target);
  tivaWare.LPRF.controllerLPRF(&controller);
  target.setMPU9150Representation(theMPU9150Representation);
  target.setInterruptVectorRepresentation(theInterruptVectorRepresentation);
#endif
}

void LPRFModule::execute()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  if (!tivaWare.LPRF.active)
    return;

  tivaWare.LPRF.updateLPRF();
  sendLoop = false;
  if (!tivaWare.LPRF.isTarget && g_startPersistanceStorage)
  {
    //
    // The controllers that contains sensor values
    //
    controller.serialize(theMPU9150Representation);
  }

  if (!tivaWare.LPRF.isTarget && !g_startPersistanceStorage && g_resetActivation)
  {
    //
    // Clear the controller state
    //
    controller.clear();
  }

  //
  // Send information when interrupted
  //
  if (tivaWare.LPRF.isTarget)
    sendLoop = true;

  if (!tivaWare.LPRF.isTarget && g_sendActivation)
    sendLoop = true;

  if (sendLoop)
  {
    // Loop the Radio Systems.
    tivaWare.LPRF.sendLPRF();
  }
#endif
}

// ============================================================================
TX_RX_COMMON::TX_RX_COMMON() :
    sendLocation(0), receiveLocation(0), bufferLocation(0), dataPktLength(
        9 * sizeof(unsigned short)), metaPktLength(2 * sizeof(int)), syncPktLength(
        sizeof(long int) + sizeof(bool))
{
  memset(buffer, 0, LPRF_TX_MAX_DATA_LEN * sizeof(unsigned char));
  memset(encodedStr, 0, LPRF_TX_MAX_DATA_LEN * sizeof(char));
}

// ============================================================================
CONTROLLER::CONTROLLER() :
    appBufferLocation(0), accumulatedPktSize(0), toSendSize(0), maxEnsembleSize(
        4/*FixMe: this needs change*/), dataPktEnsembleLength(dataPktLength * maxEnsembleSize)
{
}

uint8_t CONTROLLER::size() const
{
  return sendLocation;
}

uint8_t* CONTROLLER::data() const
{
  return (uint8_t*) encodedStr;
}

void CONTROLLER::reset()
{
  sendLocation = 0;
  g_sendActivation = false;
}

void CONTROLLER::receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
    uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data)
{
  receiveLocation = Z85_decode_with_padding((const char*) pui8Data, (char*) buffer, ui8Length);
  if (receiveLocation >= syncPktLength)
  {
    ((Serializable*) &repSync)->readFromBuffer(buffer);
    UARTprintf("CONTROLLER mills: %d sendActivation: %d\n", repSync.ms, repSync.sendActivation);
    g_sendActivation = repSync.sendActivation;
    g_resetActivation = repSync.resetActivation;
    if (g_sendActivation && !g_startPersistanceStorage)
      g_startPersistanceStorage = true;
    if (!g_sendActivation && g_resetActivation && g_startPersistanceStorage)
      g_startPersistanceStorage = false;
  }
  receiveLocation = 0;
}

void CONTROLLER::execute()
{
  toSendSize = std::min(accumulatedPktSize, maxEnsembleSize);
  //UARTprintf("accumulatedPkts: %d maxEnsembleSize: %d \n", accumulatedPkts, maxEnsembleSize);
  if (toSendSize > 0) /*If there are something to be send*/
  {
    sendLocation = LPRF_DATA_OFFSET_BYTES;

    // First serialize the meta data
    if (((sendLocation + metaPktLength) < LPRF_TX_MAX_DATA_LEN))
    {
      // we have reached to the final location to send data
      repMeta.ID = TivaWareController::getInstance().BOOSTERPACK.ID;
      sendLocation += ((Serializable*) &repMeta)->writeToBuffer(buffer + sendLocation);
    }

    for (int i = 0; i < toSendSize; i++)
    {
      if (sendLocation + dataPktLength < LPRF_TX_MAX_DATA_LEN)
      {
        if ((bufferLocation + dataPktLength) > LPRF_APP_MAX_BUFFER_LENGTH)
          bufferLocation = 0;

        memcpy(buffer + sendLocation, appBuffer + bufferLocation, dataPktLength);
        sendLocation += dataPktLength;
        bufferLocation += dataPktLength;
        --accumulatedPktSize;

        if (bufferLocation > LPRF_APP_MAX_BUFFER_LENGTH)
          bufferLocation = 0;
      }
    }

    if (((sendLocation * 5) / 4 + 1) < LPRF_TX_MAX_DATA_LEN)
    {
      //UARTprintf("Good: before=%d ", sendLocation);
      sendLocation = Z85_encode_with_padding((const char*) buffer, encodedStr, sendLocation);
      //UARTprintf("after=%d\n", sendLocation);
    }
    else
    {
      sendLocation = 0;
      UARTprintf("Err: bufferLocation=%d\n", sendLocation);
    }
  }
}

void CONTROLLER::serialize(const MPU9150Representation* inRep)
{
  if (!inRep->bUpdated)
    return;

  if ((appBufferLocation + dataPktLength) > LPRF_APP_MAX_BUFFER_LENGTH)
    appBufferLocation = 0;

  //UARTprintf("appBufferLocation: %d \n", appBufferLocation);

  if ((accumulatedPktSize > 0) && (appBufferLocation == bufferLocation))
  {
    ++repMeta.pktLost;
  }
  else
  {
    appBufferLocation += ((Serializable*) inRep)->writeToBuffer(appBuffer + appBufferLocation);
    ++accumulatedPktSize;
    repMeta.pktLost = 0;
    if (appBufferLocation > LPRF_APP_MAX_BUFFER_LENGTH)
      appBufferLocation = 0;
  }
}

void CONTROLLER::clear()
{
  bufferLocation = 0;
  appBufferLocation = 0;
  accumulatedPktSize = 0;
}

// ============================================================================
TARGET::TARGET() :
    theMPU9150Representation(0), theInterruptVectorRepresentation(0)
{
}

uint8_t TARGET::size() const
{
  return sendLocation;
}

uint8_t* TARGET::data() const
{
  return (uint8_t*) encodedStr;
}

void TARGET::execute()
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
}

void TARGET::reset()
{
  sendLocation = 0;
}

void TARGET::receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
    uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data)
{
  bufferLocation = Z85_decode_with_padding((const char*) pui8Data, (char*) buffer, ui8Length);
  if (bufferLocation >= (dataPktLength + metaPktLength + LPRF_DATA_OFFSET_BYTES))
  {
    receiveLocation = LPRF_DATA_OFFSET_BYTES; // offset

    // First we get the meta information
    receiveLocation += ((Serializable*) &repMeta)->readFromBuffer(buffer + receiveLocation);
    //UARTprintf("%03X %d ", repMeta.ID, repMeta.pktLost);

    do
    {
      receiveLocation += ((Serializable*) &repDATA)->readFromBuffer(buffer + receiveLocation);

      for (int i = 0; i < 3; i++)
      {
        repDATA.fAccel[i] = ((float) (int16_t) (repDATA.pui16Accel[i])
            * theMPU9150Representation->fAccelFactor);
        repDATA.fGyro[i] = ((float) (int16_t) (repDATA.pui16Gyro[i])
            * theMPU9150Representation->fGyroFactor);
        repDATA.fMag[i] = ((float) repDATA.pui16Mag[i]);
      }

      float* pfData = &repDATA.fAccel[0];
      //
      // Now drop back to using the data as a single array for the
      // purpose of decomposing the float into a integer part and a
      // fraction (decimal) part.
      //
      for (uint_fast32_t ui32Idx = 0; ui32Idx < 16; ui32Idx++)
      {
        //
        // Conver float value to a integer truncating the decimal part.
        //
        i32IPart[ui32Idx] = (int32_t) pfData[ui32Idx];

        //
        // Multiply by 1000 to preserve first three decimal values.
        // Truncates at the 3rd decimal place.
        //
        i32FPart[ui32Idx] = (int32_t) (pfData[ui32Idx] * 1000.0f);

        //
        // Subtract off the integer part from this newly formed decimal
        // part.
        //
        i32FPart[ui32Idx] = i32FPart[ui32Idx] - (i32IPart[ui32Idx] * 1000);

        //
        // make the decimal part a positive number for display.
        //
        if (i32FPart[ui32Idx] < 0)
        {
          i32FPart[ui32Idx] *= -1;
        }
      }

      //
      // Print the acceleration numbers in the table.
      //
      UARTprintf("%3d.%03d ", i32IPart[0], i32FPart[0]);
      UARTprintf("%3d.%03d ", i32IPart[1], i32FPart[1]);
      UARTprintf("%3d.%03d ", i32IPart[2], i32FPart[2]);

      //
      // Print the angular velocities in the table.
      //
      UARTprintf("%3d.%03d ", i32IPart[3], i32FPart[3]);
      UARTprintf("%3d.%03d ", i32IPart[4], i32FPart[4]);
      UARTprintf("%3d.%03d ", i32IPart[5], i32FPart[5]);

      //
      // Print the magnetic data in the table.
      //
      UARTprintf("%3d.%03d ", i32IPart[6], i32FPart[6]);
      UARTprintf("%3d.%03d ", i32IPart[7], i32FPart[7]);
      UARTprintf("%3d.%03d ", i32IPart[8], i32FPart[8]);

    } while (receiveLocation < bufferLocation);
  }
  UARTprintf("\n"); // flush
  // UARTprintf("ui8Length: %d bufferLocation: %d receiveLocation: %d \n", ui8Length, bufferLocation, receiveLocation);
  // some delay to serial port
  ROM_SysCtlDelay(TivaWareController::getInstance().CLOCK.ui32SysClock / (1000 * 3)); // 1 ms
}

void TARGET::setMPU9150Representation(const MPU9150Representation* theMPU9150Representation)
{
  this->theMPU9150Representation = theMPU9150Representation;
}

void TARGET::setInterruptVectorRepresentation(
    const InterruptVectorRepresentation* theInterruptVectorRepresentation)
{
  this->theInterruptVectorRepresentation = theInterruptVectorRepresentation;
}

// ============================================================================
