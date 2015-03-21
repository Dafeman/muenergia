/*
 * BMP180Module.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: sam
 */

#include "BMP180Module.h"
//
#include "sensorlib/hw_bmp180.h"
#include "sensorlib/bmp180.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>pressure Measurement with the BMP180 (pressure_bmp180)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and the SensHub BoosterPack to obtain air pressure and
//! temperature measurements with the BMP180 sensor.
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
// Define BMP180 I2C Address.
//
//*****************************************************************************
#define BMP180_I2C_ADDRESS      0x77

//*****************************************************************************
//
// Global instance structure for the BMP180 sensor driver.
//
//*****************************************************************************
tBMP180 g_sBMP180Inst;

//*****************************************************************************
//
// Global new data flag to alert main that BMP180 data is ready.
//
//*****************************************************************************
static volatile uint_fast8_t g_vui8DataFlag;

//*****************************************************************************
//
// BMP180 Sensor callback function.  Called at the end of BMP180 sensor driver
// transactions. This is called from I2C interrupt context. Therefore, we just
// set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void BMP180AppCallback(void* pvCallbackData, uint_fast8_t ui8Status)
{
  if (ui8Status == I2CM_STATUS_SUCCESS)
  {
    g_vui8DataFlag = 1;
  }
}

void BMP180Module::init()
{
  //
  // Initialize the BMP180.
  //
  BMP180Init(&g_sBMP180Inst, &tivaWare.I2C.instance, BMP180_I2C_ADDRESS, BMP180AppCallback,
      &g_sBMP180Inst);

  //
  // Wait for initialization callback to indicate reset request is complete.
  //
  while (g_vui8DataFlag == 0)
  {
    //
    // Wait for I2C Transactions to complete.
    //
  }

  //
  // Reset the data ready flag
  //
  g_vui8DataFlag = 0;
}

void BMP180Module::update(BMP180Representation& theBMP180Representation)
{
  if (!theSensorAccessRepresentation.isNull() && !theSensorAccessRepresentation->active)
    return;

  if (!theInterruptVectorRepresentation->interruptedSysTick)
    return;
  //
  // Read the data from the BMP180 over I2C.  This command starts a
  // temperature measurement.  Then polls until temperature is ready.
  // Then automatically starts a pressure measurement and polls for that
  // to complete. When both measurement are complete and in the local
  // buffer then the application callback is called from the I2C
  // interrupt context.  Polling is done on I2C interrupts allowing
  // processor to continue doing other tasks as needed.
  //
  BMP180DataRead(&g_sBMP180Inst, BMP180AppCallback, &g_sBMP180Inst);
  while (g_vui8DataFlag == 0)
  {
    //
    // Wait for the new data set to be available.
    //
  }

  //
  // Reset the data ready flag.
  //
  g_vui8DataFlag = 0;

  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  //
  // Get a local copy of the latest temperature data in float format.
  //
  BMP180DataTemperatureGetFloat(&g_sBMP180Inst, &theBMP180Representation.fTemperature);

  //
  // Convert the floats to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) theBMP180Representation.fTemperature;
  i32FractionPart = (int32_t) (theBMP180Representation.fTemperature * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print temperature with three digits of decimal precision.
  //
  //tivaWare.UART.printf("Temperature %3d.%03d\t\t", i32IntegerPart, i32FractionPart);

  //
  // Get a local copy of the latest air pressure data in float format.
  //
  BMP180DataPressureGetFloat(&g_sBMP180Inst, &theBMP180Representation.fPressure);

  //
  // Convert the floats to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) theBMP180Representation.fPressure;
  i32FractionPart = (int32_t) (theBMP180Representation.fPressure * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print Pressure with three digits of decimal precision.
  //
  //tivaWare.UART.printf("Pressure %3d.%03d\t\t", i32IntegerPart, i32FractionPart);

  //
  // Calculate the altitude.
  //
  theBMP180Representation.fAltitude = 44330.0f
      * (1.0f - powf(theBMP180Representation.fPressure / 101325.0f, 1.0f / 5.255f));

  //
  // Convert the floats to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) theBMP180Representation.fAltitude;
  i32FractionPart = (int32_t) (theBMP180Representation.fAltitude * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print altitude with three digits of decimal precision.
  //
  //tivaWare.UART.printf("Altitude %3d.%03d", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  //tivaWare.UART.printf("\n");
}


