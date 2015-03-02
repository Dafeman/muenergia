/*
 * SHT21Module.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: sam
 */

#include "SHT21Module.h"
//
#include "sensorlib/hw_sht21.h"
#include "sensorlib/sht21.h"

// Very slow
//MAKE_MODULE(SHT21Module)

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Humidity Measurement with the SHT21 (humidity_sht21)</h1>
//!
//! This example demonstrates the basic use of the Sensoror Library, TM4C123G
//! LaunchPad and SensHub BoosterPack to obtain temperature and relative
//! humidity of the environment using the Sensirion SHT21 sensor.
//!
//! Connect a serial terminal program to the LaunchPad's ICDI virtual serial
//! port at 115,200 baud.  Use eight bits per byte, no parity and one stop bit.
//! The humidity and temperature as measured by the SHT21 is printed to the
//! terminal.  The RGB LED begins to blink at 1Hz after initialization is
//! complete and the example application is running.
//
//*****************************************************************************

//*****************************************************************************
//
// Define SHT21 I2C Address.
//
//*****************************************************************************
#define SHT21_I2C_ADDRESS  0x40

//*****************************************************************************
//
// Global instance structure for the SHT21 sensor driver.
//
//*****************************************************************************
tSHT21 g_sSHT21Inst;

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

static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// SHT21 Sensor callback function.  Called at the end of SHT21 sensor driver
// transactions. This is called from I2C interrupt context. Therefore, we just
// set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void SHT21AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
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
void SHT21AppErrorHandler(char *pcFilename, uint_fast32_t ui32Line)
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

//*****************************************************************************
//
// Function to wait for the SHT21 transactions to complete.
//
//*****************************************************************************
void SHT21AppI2CWait(char *pcFilename, uint_fast32_t ui32Line)
{
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
    SHT21AppErrorHandler(pcFilename, ui32Line);
  }

  //
  // clear the data flag for next use.
  //
  g_vui8DataFlag = 0;
}

void SHT21Module::init()
{
  //
  // Initialize the TMP006
  //
  SHT21Init(&g_sSHT21Inst, &tivaWare.I2C.instance, SHT21_I2C_ADDRESS, SHT21AppCallback,
      &g_sSHT21Inst);

  //
  // Wait for the I2C transactions to complete before moving forward
  //
  SHT21AppI2CWait(__FILE__, __LINE__);

  //
  // Delay for 20 milliseconds for SHT21 reset to complete itself.
  // Datasheet says reset can take as long 15 milliseconds.
  //
  ROM_SysCtlDelay(tivaWare.CLOCK.ui32SysClock / (50 * 3));
}

void SHT21Module::update(SHT21Representation& theSHT21Representation)
{
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  //
  // Write the command to start a humidity measurement
  //
  SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_RH, g_sSHT21Inst.pui8Data, 0, SHT21AppCallback,
      &g_sSHT21Inst);

  //
  // Wait for the I2C transactions to complete before moving forward
  //
  SHT21AppI2CWait(__FILE__, __LINE__);

  //
  // Wait 33 milliseconds before attempting to get the result. Datasheet
  // claims this can take as long as 29 milliseconds
  //
  ROM_SysCtlDelay(tivaWare.CLOCK.ui32SysClock / (30 * 3));

  //
  // Get the raw data from the sensor over the I2C bus
  //
  SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

  //
  // Wait for the I2C transactions to complete before moving forward
  //
  SHT21AppI2CWait(__FILE__, __LINE__);

  //
  // Get a copy of the most recent raw data in floating point format.
  //
  SHT21DataHumidityGetFloat(&g_sSHT21Inst, &theSHT21Representation.fHumidity);

  //
  // Write the command to start a temperature measurement
  //
  SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_T, g_sSHT21Inst.pui8Data, 0, SHT21AppCallback,
      &g_sSHT21Inst);

  //
  // Wait for the I2C transactions to complete before moving forward
  //
  SHT21AppI2CWait(__FILE__, __LINE__);

  //
  // Wait 100 milliseconds before attempting to get the result. Datasheet
  // claims this can take as long as 85 milliseconds
  //
  ROM_SysCtlDelay(tivaWare.CLOCK.ui32SysClock / (10 * 3));

  //
  // Read the conversion data from the sensor over I2C.
  //
  SHT21DataRead(&g_sSHT21Inst, SHT21AppCallback, &g_sSHT21Inst);

  //
  // Wait for the I2C transactions to complete before moving forward
  //
  SHT21AppI2CWait(__FILE__, __LINE__);

  //
  // Get the most recent temperature result as a float in celcius.
  //
  SHT21DataTemperatureGetFloat(&g_sSHT21Inst, &theSHT21Representation.fTemperature);

  //
  // Convert the floats to an integer part and fraction part for easy
  // print. Humidity is returned as 0.0 to 1.0 so multiply by 100 to get
  // percent humidity.
  //
  theSHT21Representation.fHumidity *= 100.0f;
  i32IntegerPart = (int32_t) theSHT21Representation.fHumidity;
  i32FractionPart = (int32_t) (theSHT21Representation.fHumidity * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print the humidity value using the integers we just created
  //
  //tivaWare.UART.printf("Humidity %3d.%03d\t", i32IntegerPart, i32FractionPart);

  //
  // Perform the conversion from float to a printable set of integers
  //
  i32IntegerPart = (int32_t) theSHT21Representation.fTemperature;
  i32FractionPart = (int32_t) (theSHT21Representation.fTemperature * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print the temperature as integer and fraction parts.
  //
  //tivaWare.UART.printf("Temperature %3d.%03d\n", i32IntegerPart, i32FractionPart);

  //
  // Delay for one second. This is to keep sensor duty cycle
  // to about 10% as suggested in the datasheet, section 2.4.
  // This minimizes self heating effects and keeps reading more accurate.
  //
  ROM_SysCtlDelay(tivaWare.CLOCK.ui32SysClock / 3);
}

