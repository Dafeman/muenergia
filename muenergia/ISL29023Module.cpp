/*
 * ISL29023Module.cpp
 *
 *  Created on: Sep 11, 2014
 *      Author: sam
 */

#include "ISL29023Module.h"
//
#include "sensorlib/hw_isl29023.h"
#include "sensorlib/isl29023.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Light Measurement with the ISL29023 (light_isl29023)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and the SensHub BoosterPack to obtain ambient and infrared light
//! measurements with the ISL29023 sensor.
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
// Define ISL29023 I2C Address.
//
//*****************************************************************************
#define ISL29023_I2C_ADDRESS    0x44

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     1
#define SYSTICK_PERIOD_MS       (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Global instance structure for the ISL29023 sensor driver.
//
//*****************************************************************************
tISL29023 g_sISL29023Inst;

//*****************************************************************************
//
// Global flags to alert main that ISL29023 data is ready or an error
// has occurred.
//
//*****************************************************************************
static volatile unsigned long g_vui8DataFlag;
static volatile unsigned long g_vui8ErrorFlag;

//*****************************************************************************
//
// Constants to hold the floating point version of the thresholds for each
// range setting. Numbers represent an 81% and 19 % threshold levels. This
// creates a +/- 1% hysteresis band between range adjustments.
//
//*****************************************************************************
const float g_fThresholdHigh[4] =
{ 810.0f, 3240.0f, 12960.0f, 64000.0f };
const float g_fThresholdLow[4] =
{ 0.0f, 760.0f, 3040.0f, 12160.0f };

static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// ISL29023 Sensor callback function.  Called at the end of ISL29023 sensor
// driver transactions. This is called from I2C interrupt context. Therefore,
// we just set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void ISL29023AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
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
// ISL29023 Application error handler.
//
//*****************************************************************************
void ISL29023AppErrorHandler(char *pcFilename, uint_fast32_t ui32Line)
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
// Function to wait for the ISL29023 transactions to complete.
//
//*****************************************************************************
void ISL29023AppI2CWait(char *pcFilename, uint_fast32_t ui32Line)
{
  //
  // Put the processor to sleep while we wait for the I2C driver to
  // indicate that the transaction is complete.
  //
  while ((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
  {
    //
    // Do Nothing
    //
  }

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    ISL29023AppErrorHandler(pcFilename, ui32Line);
  }

  //
  // clear the data flag for next use.
  //
  g_vui8DataFlag = 0;
}

//*****************************************************************************
//
// Intensity and Range Tracking Function.  This adjusts the range and interrupt
// thresholds as needed.  Uses an 80/20 rule. If light is greather then 80% of
// maximum value in this range then go to next range up. If less than 20% of
// potential value in this range go the next range down.
//
//*****************************************************************************
void ISL29023AppAdjustRange(tISL29023 *pInst)
{
  float fAmbient;
  uint8_t ui8NewRange;

  ui8NewRange = g_sISL29023Inst.ui8Range;

  //
  // Get a local floating point copy of the latest light data
  //
  ISL29023DataLightVisibleGetFloat(&g_sISL29023Inst, &fAmbient);

  //
  // Check if we crossed the upper threshold.
  //
  if (fAmbient > g_fThresholdHigh[g_sISL29023Inst.ui8Range])
  {
    //
    // The current intensity is over our threshold so adjsut the range
    // accordingly
    //
    if (g_sISL29023Inst.ui8Range < ISL29023_CMD_II_RANGE_64K)
    {
      ui8NewRange = g_sISL29023Inst.ui8Range + 1;
    }
  }

  //
  // Check if we crossed the lower threshold
  //
  if (fAmbient < g_fThresholdLow[g_sISL29023Inst.ui8Range])
  {
    //
    // If possible go to the next lower range setting and reconfig the
    // thresholds.
    //
    if (g_sISL29023Inst.ui8Range > ISL29023_CMD_II_RANGE_1K)
    {
      ui8NewRange = g_sISL29023Inst.ui8Range - 1;
    }
  }

  //
  // If the desired range value changed then send the new range to the sensor
  //
  if (ui8NewRange != g_sISL29023Inst.ui8Range)
  {
    ISL29023ReadModifyWrite(&g_sISL29023Inst, ISL29023_O_CMD_II, ~ISL29023_CMD_II_RANGE_M,
        ui8NewRange, ISL29023AppCallback, &g_sISL29023Inst);

    //
    // Wait for transaction to complete
    //
    ISL29023AppI2CWait(__FILE__, __LINE__);
  }
}

void ISL29023Module::init()
{
  //
  // Initialize the ISL29023 Driver.
  //
  ISL29023Init(&g_sISL29023Inst, &g_tivaWare.I2C.instance,
  ISL29023_I2C_ADDRESS, ISL29023AppCallback, &g_sISL29023Inst);

  //
  // Wait for transaction to complete
  //
  ISL29023AppI2CWait(__FILE__, __LINE__);

  //
  // Configure the ISL29023 to measure ambient light continuously. Set a 8
  // sample persistence before the INT pin is asserted. Clears the INT flag.
  // Persistence setting of 8 is sufficient to ignore camera flashes.
  //
  uint8_t ui8Mask = (ISL29023_CMD_I_OP_MODE_M | ISL29023_CMD_I_INT_PERSIST_M |
  ISL29023_CMD_I_INT_FLAG_M);
  ISL29023ReadModifyWrite(&g_sISL29023Inst, ISL29023_O_CMD_I, ~ui8Mask,
      (ISL29023_CMD_I_OP_MODE_ALS_CONT |
      ISL29023_CMD_I_INT_PERSIST_8), ISL29023AppCallback, &g_sISL29023Inst);

  //
  // Wait for transaction to complete
  //
  ISL29023AppI2CWait(__FILE__, __LINE__);

  //
  // Configure the upper threshold to 80% of maximum value
  //
  g_sISL29023Inst.pui8Data[1] = 0xCC;
  g_sISL29023Inst.pui8Data[2] = 0xCC;
  ISL29023Write(&g_sISL29023Inst, ISL29023_O_INT_HT_LSB, g_sISL29023Inst.pui8Data, 2,
      ISL29023AppCallback, &g_sISL29023Inst);

  //
  // Wait for transaction to complete
  //
  ISL29023AppI2CWait(__FILE__, __LINE__);

  //
  // Configure the lower threshold to 20% of maximum value
  //
  g_sISL29023Inst.pui8Data[1] = 0x33;
  g_sISL29023Inst.pui8Data[2] = 0x33;
  ISL29023Write(&g_sISL29023Inst, ISL29023_O_INT_LT_LSB, g_sISL29023Inst.pui8Data, 2,
      ISL29023AppCallback, &g_sISL29023Inst);
  //
  // Wait for transaction to complete
  //
  ISL29023AppI2CWait(__FILE__, __LINE__);

}

void ISL29023Module::update(ISL29023Representation& theISL29023Representation)
{
  if (!theInterruptVectorRepresentation->interruptedSysTick)
    return;
  //
  // Go get the latest data from the sensor.
  //
  ISL29023DataRead(&g_sISL29023Inst, ISL29023AppCallback, &g_sISL29023Inst);

  //
  // Wait for transaction to complete
  //
  ISL29023AppI2CWait(__FILE__, __LINE__);

  int32_t i32IntegerPart, i32FractionPart;

  g_vui8DataFlag = 0;

  //
  // Get a local floating point copy of the latest light data
  //
  ISL29023DataLightVisibleGetFloat(&g_sISL29023Inst, &theISL29023Representation.fAmbient);

  //
  // Perform the conversion from float to a printable set of integers
  //
  i32IntegerPart = (int32_t) theISL29023Representation.fAmbient;
  i32FractionPart = (int32_t) (theISL29023Representation.fAmbient * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print the temperature as integer and fraction parts.
  //
  tivaWare.UART.printf("Visible Lux: %3d.%03d\n", i32IntegerPart, i32FractionPart);

  //
  // Check if the intensity of light has crossed a threshold. If so
  // then adjust range of sensor readings to track intensity.
  //
  if (theInterruptVectorRepresentation->interruptedISL29023)
  {
    //
    // Disable the low priority interrupts leaving only the I2C
    // interrupt enabled.
    //
    ROM_IntPriorityMaskSet(0x40);

    //
    // Adjust the lux range.
    //
    ISL29023AppAdjustRange(&g_sISL29023Inst);

    //
    // Now we must manually clear the flag in the ISL29023
    // register.
    //
    ISL29023Read(&g_sISL29023Inst, ISL29023_O_CMD_I, g_sISL29023Inst.pui8Data, 1,
        ISL29023AppCallback, &g_sISL29023Inst);

    //
    // Wait for transaction to complete
    //
    ISL29023AppI2CWait(__FILE__, __LINE__);

    //
    // Disable priority masking so all interrupts are enabled.
    //
    ROM_IntPriorityMaskSet(0);
  }
}

