/*
 * TMP006Module.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#include "TMP006Module.h"
//
#include "sensorlib/hw_tmp006.h"
#include "sensorlib/tmp006.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Temperature Measurement with the TMP006 (temperature_tmp006)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and the SensHub BoosterPack to obtain ambient and object
//! temperature measurements with the Texas Instruments TMP006 sensor.
//!
//! Connect a serial terminal program to the LaunchPad's ICDI virtual serial
//! port at 115,200 baud.  Use eight bits per byte, no parity and one stop bit.
//! The raw sensor measurements are printed to the terminal.  The RGB LED
//! blinks at 1Hz once the initialization is complete and the example is
//! running.
//
//*****************************************************************************

//*****************************************************************************
//
// Define TMP006 I2C Address.
//
//*****************************************************************************
#define TMP006_I2C_ADDRESS      0x41

//*****************************************************************************
//
// Global new data flag to alert main that TMP006 data is ready.
//
//*****************************************************************************
static volatile uint_fast8_t g_vui8DataFlag;

//*****************************************************************************
//
// Global new error flag to store the error condition if encountered.
//
//*****************************************************************************
static volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// Global instance structure for the TMP006 sensor driver.
//
//*****************************************************************************
tTMP006 g_sTMP006Inst;

static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// TMP006 Sensor callback function.  Called at the end of TMP006 sensor driver
// transactions. This is called from I2C interrupt context. Therefore, we just
// set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void TMP006AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
  //
  // If the transaction succeeded set the data flag to indicate to
  // application that this transaction is complete and data may be ready.
  //
  if (ui8Status == I2CM_STATUS_SUCCESS)
  {
    g_vui8DataFlag = 1;
  }

  //
  // Store the most recent status in case it was an error condition
  //
  g_vui8ErrorFlag = ui8Status;
}

//*****************************************************************************
//
// TMP006 Application error handler.
//
//*****************************************************************************
void TMP006AppErrorHandler(char *pcFilename, uint_fast32_t ui32Line)
{
  //
  // Set terminal color to red and print error status and locations
  //

  g_tivaWare.UART.printf("\033[31;1m");
  g_tivaWare.UART.printf("Error: %d, File: %s, Line: %d\n"
      "See I2C status definitions in utils\\i2cm_drv.h\n", g_vui8ErrorFlag, pcFilename, ui32Line);

  //
  // Return terminal color to normal
  //
  g_tivaWare.UART.printf("\033[0m");

#ifdef TARGET_IS_BLIZZARD_RB1
  //
  // Set RGB Color to RED
  //
  g_tivaWare.LED.colorSetRGB(0xFFFF, 0, 0);

  //
  // Increase blink rate to get attention
  //
  g_tivaWare.LED.blinkRateSetRGB(10.0f);

  //
  // Go to sleep wait for interventions.  A more robust application could
  // attempt corrective actions here.
  //
  while (1)
  {
    ROM_SysCtlSleep();
  }
#else
  uint32_t ui32LEDState;
  //
  // Read the initial LED state and clear the CLP_D3 LED
  //
  LEDRead(&ui32LEDState);
  ui32LEDState &= ~CLP_D3;

  //
  // Do nothing and wait for interventions.  A more robust application could
  // attempt corrective actions here.
  //
  while (1)
  {
    //
    // Toggle LED 4 to indicate the error.
    //
    ui32LEDState ^= CLP_D4;
    LEDWrite(CLP_D3 | CLP_D4, ui32LEDState);

    //
    // Do Nothing
    //
    ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / (10 * 3));
  }
#endif
}

void TMP006Module::init()
{
  //
  // Initialize the TMP006
  //
  TMP006Init(&g_sTMP006Inst, &tivaWare.I2C.instance, TMP006_I2C_ADDRESS, TMP006AppCallback,
      &g_sTMP006Inst);

  //
  // Put the processor to sleep while we wait for the I2C driver to
  // indicate that the transaction is complete.
  //
  while ((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
  {
    ROM_SysCtlSleep();
  }

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    TMP006AppErrorHandler(__FILE__, __LINE__);
  }

  //
  // clear the data flag for next use.
  //
  g_vui8DataFlag = 0;

  //
  // Delay for 10 milliseconds for TMP006 reset to complete.
  // Not explicitly required. Datasheet does not say how long a reset takes.
  //
  ROM_SysCtlDelay(tivaWare.CLOCK.ui32SysClock / (100 * 3));

  //
  // Enable the DRDY pin indication that a conversion is in progress.
  //
  TMP006ReadModifyWrite(&g_sTMP006Inst, TMP006_O_CONFIG, ~TMP006_CONFIG_EN_DRDY_PIN_M,
  TMP006_CONFIG_EN_DRDY_PIN, TMP006AppCallback, &g_sTMP006Inst);

  //
  // Wait for the DRDY enable I2C transaction to complete.
  //
  while ((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
  {
    ROM_SysCtlSleep();
  }

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    TMP006AppErrorHandler(__FILE__, __LINE__);
  }

  //
  // clear the data flag for next use.
  //
  g_vui8DataFlag = 0;

}

void TMP006Module::update(TMP006Representation& theTMP006Representation)
{
  if (!theInterruptVectorRepresentation->interruptedTMP006)
    return;

  //
  // This interrupt indicates a conversion is complete and ready to be
  // fetched.  So we start the process of getting the data.
  //
  TMP006DataRead(&g_sTMP006Inst, TMP006AppCallback, &g_sTMP006Inst);

  //
  // Put the processor to sleep while we wait for the TMP006 to
  // signal that data is ready.  Also continue to sleep while I2C
  // transactions get the raw data from the TMP006
  //
  if ((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
    return;

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    TMP006AppErrorHandler(__FILE__, __LINE__);
  }

  //
  // Reset the flag
  //
  g_vui8DataFlag = 0;

  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  //
  // Get a local copy of the latest data in float format.
  //
  TMP006DataTemperatureGetFloat(&g_sTMP006Inst, &theTMP006Representation.fAmbient,
      &theTMP006Representation.fObject);

  //
  // Convert the floating point ambient temperature  to an integer part
  // and fraction part for easy printing.
  //
  i32IntegerPart = (int32_t) theTMP006Representation.fAmbient;
  i32FractionPart = (int32_t) (theTMP006Representation.fAmbient * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }
  //tivaWare.UART.printf("Ambient %3d.%03d\t", i32IntegerPart, i32FractionPart);

  //
  // Convert the floating point ambient temperature  to an integer part
  // and fraction part for easy printing.
  //
  i32IntegerPart = (int32_t) theTMP006Representation.fObject;
  i32FractionPart = (int32_t) (theTMP006Representation.fObject * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }
  //tivaWare.UART.printf("Object %3d.%03d\n", i32IntegerPart, i32FractionPart);
}

