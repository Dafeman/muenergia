/*
 * BQ27510G3Module.cpp
 *
 *  Created on: Sep 15, 2014
 *      Author: sam
 */

#include "BQ27510G3Module.h"
//
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
//#include "utils/ustdlib.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/bq27510g3.h"
#include "sensorlib/hw_bq27510g3.h"
#include "drivers/rgb.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Fuel Tank BoosterPack Measurement example application (boostxl_battpack)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and the Fuel Tank BoosterPack to obtain state-of-charge,
//! battery voltage, temperature, and several other supported  measurements
//! via the BQ27510G3 gas gauge sensor on the Fuel tank boosterpack.
//!
//! Connect a serial terminal program to the LaunchPad's ICDI virtual serial
//! port at 115,200 baud.  Use eight bits per byte, no parity and one stop bit.
//! The raw sensor measurements are printed to the terminal.
//
//*****************************************************************************

//*****************************************************************************
//
// Define BQ27510G3 I2C Address.
//
//*****************************************************************************
#define BQ27510G3_I2C_ADDRESS      0x55

//*****************************************************************************
//
// Global instance structure for the BQ27510G3 sensor driver.
//
//*****************************************************************************
tBQ27510G3 g_sBQ27510G3Inst;

//*****************************************************************************
//
// Global new data flag to alert main that BQ27510G3 data is ready.
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
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// Global buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// BQ27510G3 Sensor callback function.  Called at the end of BQ27510G3 sensor
// driver transactions. This is called from I2C interrupt context. Therefore,
// we just set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void BQ27510G3AppCallback(void* pvCallbackData, uint_fast8_t ui8Status)
{
  if (ui8Status == I2CM_STATUS_SUCCESS)
  {
    //
    // If I2C transaction is successful, set data ready flag.
    //
    g_vui8DataFlag = 1;
  }
  else
  {
    //
    // If I2C transaction fails, set error flag.
    //
    g_vui8ErrorFlag = ui8Status;
  }
}

//*****************************************************************************
//
// Function to wait for the BQ27510G3 transactions to complete.
//
//*****************************************************************************
uint_fast8_t BQ27510G3AppI2CWait(void)
{
  uint_fast8_t ui8RC = 0;

  //
  // Put the processor to sleep while we wait for the I2C driver to
  // indicate that the transaction is complete.
  //
  while ((g_vui8DataFlag == 0) && (g_vui8ErrorFlag == 0))
  {
    //
    // Wait for I2C Transactions to complete.
    //
  }

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    g_tivaWare.UART.printf("\033[31;1m");
    g_tivaWare.UART.printf("I2C Error!\n\n");
    g_tivaWare.UART.printf("Please ensure FuelTank BP is installed correctly\n");
    g_tivaWare.UART.printf("and is sufficiently charged for minimum functionality.\n");
    g_tivaWare.UART.printf("\033[0m");
    ui8RC = g_vui8ErrorFlag;
  }

  //
  // Clear the data flags for next use.
  //
  g_vui8DataFlag = 0;
  g_vui8ErrorFlag = 0;

  //
  // Return original error code to caller.
  //
  return ui8RC;
}

//*****************************************************************************
//
// Prompts the user for a command, and blocks while waiting for the user's
// input. This function will return after the execution of a single command.
//
//*****************************************************************************
void CheckForUserCommands(void)
{
  int iStatus;

  //
  // Peek to see if a full command is ready for processing
  //
  if (UARTPeek('#') == -1)
  {
    //
    // If not, return so other functions get a chance to run.
    //
    return;
  }

  //
  // If we do have commands, process them immediately in the order they were
  // received.
  //
  while (UARTPeek('#') != -1)
  {
    //
    // Get a user command back
    //
    UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

    //
    // Process the received command
    //
    iStatus = CmdLineProcess(g_cCmdBuf);

    //
    // Handle the case of bad command.
    //
    if (iStatus == CMDLINE_BAD_CMD)
    {
      g_tivaWare.UART.printf("Bad command! Type 'h' for help!\n");
    }

    //
    // Handle the case of too many arguments.
    //
    else if (iStatus == CMDLINE_TOO_MANY_ARGS)
    {
      g_tivaWare.UART.printf("Too many arguments for command processor!\n");
    }
  }

  //
  // Print the prompt
  //
  g_tivaWare.UART.printf("\nBattpack> ");
}

//*****************************************************************************
//
// This function implements the "Cmd_temp" command.
// This function displays the current temp of the battery boosterpack
//
//*****************************************************************************
int Cmd_temp(int argc, char *argv[])
{
  float fTemperature;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  g_tivaWare.UART.printf("\nPolling Temperature, press any key to stop\n\n");

  while (1)
  {
    //
    // Detect if any key has been pressed.
    //
    if (UARTRxBytesAvail() > 0)
    {
      UARTFlushRx();
      break;
    }

    //
    // Get the raw data from the sensor over the I2C bus.
    //
    BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

    //
    // Wait for callback to indicate request is complete.
    //
    ui8Status = BQ27510G3AppI2CWait();

    //
    // If initial read is not successful, exit w/o trying other reads.
    //
    if (ui8Status != 0)
    {
      return (0);
    }

    //
    // Call routine to retrieve data in float format.
    //
    BQ27510G3DataTemperatureInternalGetFloat(&g_sBQ27510G3Inst, &fTemperature);

    //
    // Convert the temperature to an integer part and fraction part for
    // easy print.
    //
    i32IntegerPart = (int32_t) fTemperature;
    i32FractionPart = (int32_t) (fTemperature * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if (i32FractionPart < 0)
    {
      i32FractionPart *= -1;
    }

    //
    // Print temperature with one digit of decimal precision.
    //
    g_tivaWare.UART.printf("Current Temperature: \t%3d.%01d C\t\t", i32IntegerPart,
        i32FractionPart);

    //
    // Print new line.
    //
    g_tivaWare.UART.printf("\n");

    //
    // Delay for one second. This is to keep sensor duty cycle
    // to about 10% as suggested in the datasheet, section 2.4.
    // This minimizes self heating effects and keeps reading more accurate.
    //
    ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / 3);
  }

  //
  // Return success.
  //
  return (0);

}

//*****************************************************************************
//
// This function implements the "Cmd_volt" command.
// This function displays the current battery voltage of the battery
// boosterpack.
//
//*****************************************************************************
int Cmd_volt(int argc, char *argv[])
{
  float fData;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  g_tivaWare.UART.printf("\nPolling battery voltage, press any key to stop\n\n");

  while (1)
  {
    //
    // Detect if any key has been pressed.
    //
    if (UARTRxBytesAvail() > 0)
    {
      UARTFlushRx();
      break;
    }

    //
    // Get the raw data from the sensor over the I2C bus.
    //
    BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

    //
    // Wait for callback to indicate request is complete.
    //
    ui8Status = BQ27510G3AppI2CWait();

    //
    // If initial read is not successful, exit w/o trying other reads.
    //
    if (ui8Status != 0)
    {
      return (0);
    }

    //
    // Call routine to retrieve data in float format.
    //
    BQ27510G3DataVoltageBatteryGetFloat(&g_sBQ27510G3Inst, &fData);

    //
    // Convert the data to an integer part and fraction part for easy
    // print.
    //
    i32IntegerPart = (int32_t) fData;
    i32FractionPart = (int32_t) (fData * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if (i32FractionPart < 0)
    {
      i32FractionPart *= -1;
    }

    //
    // Print voltage with one digit of decimal precision.
    //
    g_tivaWare.UART.printf("Battery Voltage: \t%3d.%01d mV\t\t", i32IntegerPart, i32FractionPart);

    //
    // Print new line.
    //
    g_tivaWare.UART.printf("\n");

    //
    // Delay for one second. This is to keep sensor duty cycle
    // to about 10% as suggested in the datasheet, section 2.4.
    // This minimizes self heating effects and keeps reading more accurate.
    //
    ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / 3);
  }

  //
  // Return success.
  //
  return (0);
}

//*****************************************************************************
//
// This function implements the "Cmd_current" command.
// This function displays the battery current to/from the battery boosterpack
//
//*****************************************************************************
int Cmd_current(int argc, char *argv[])
{
  float fData;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  g_tivaWare.UART.printf("\nPolling battery current, press any key to stop\n\n");

  while (1)
  {
    //
    // Detect if any key has been pressed.
    //
    if (UARTRxBytesAvail() > 0)
    {
      UARTFlushRx();
      break;
    }

    //
    // Get the raw data from the sensor over the I2C bus
    //
    BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

    //
    // Wait for callback to indicate request is complete.
    //
    ui8Status = BQ27510G3AppI2CWait();

    //
    // If initial read is not successful, exit w/o trying other reads.
    //
    if (ui8Status != 0)
    {
      return (0);
    }

    //
    // Call routine to retrieve data in float format.
    //
    BQ27510G3DataCurrentInstantaneousGetFloat(&g_sBQ27510G3Inst, &fData);
    fData *= 1000.0f;

    //
    // Convert the data to an integer part and fraction part for easy
    // print.
    //
    i32IntegerPart = (int32_t) fData;
    i32FractionPart = (int32_t) (fData * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if (i32FractionPart < 0)
    {
      i32FractionPart *= -1;
    }

    //
    // Print voltage with one digit of decimal precision.
    //
    g_tivaWare.UART.printf("Battery Current: \t%3d.%01d mA\t\t", i32IntegerPart, i32FractionPart);

    //
    // Print new line.
    //
    g_tivaWare.UART.printf("\n");

    //
    // Delay for one second. This is to keep sensor duty cycle
    // to about 10% as suggested in the datasheet, section 2.4.
    // This minimizes self heating effects and keeps reading more accurate.
    //
    ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / 3);
  }

  //
  // Return success.
  //
  return (0);
}

//*****************************************************************************
//
// This function implements the "Cmd_charge" command.
// This function displays the remaining charge of the battery boosterpack
//
//*****************************************************************************
int Cmd_charge(int argc, char *argv[])
{
  float fData;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  g_tivaWare.UART.printf("\nPolling remaining charge, press any key to stop\n\n");

  while (1)
  {
    //
    // Detect if any key has been pressed.
    //
    if (UARTRxBytesAvail() > 0)
    {
      UARTFlushRx();
      break;
    }

    //
    // Get the raw data from the sensor over the I2C bus.
    //
    BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

    //
    // Wait for callback to indicate request is complete.
    //
    ui8Status = BQ27510G3AppI2CWait();

    //
    // If initial read is not successful, exit w/o trying other reads.
    //
    if (ui8Status != 0)
    {
      return (0);
    }

    //
    // Call routine to retrieve data in float format.
    //
    BQ27510G3DataChargeStateGetFloat(&g_sBQ27510G3Inst, &fData);

    //
    // Convert the data to an integer part and fraction part for easy
    // print.
    //
    i32IntegerPart = (int32_t) fData;
    i32FractionPart = (int32_t) (fData * 1000.0f);
    i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
    if (i32FractionPart < 0)
    {
      i32FractionPart *= -1;
    }

    //
    // Print SoC with one digit of decimal precision.
    //
    g_tivaWare.UART.printf("State of Charge: \t%3d.%01d %%\t\t", i32IntegerPart, i32FractionPart);

    //
    // Print new line.
    //
    g_tivaWare.UART.printf("\n");

    //
    // Delay for one second. This is to keep sensor duty cycle
    // to about 10% as suggested in the datasheet, section 2.4.
    // This minimizes self heating effects and keeps reading more accurate.
    //
    ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / 3);
  }

  //
  // Return success.
  //
  return (0);
}
//*****************************************************************************
//
// This function implements the "Cmd_health" command.
// This function displays the current health of the battery boosterpack
//
//*****************************************************************************
int Cmd_health(int argc, char *argv[])
{
  float fData;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;

  //
  // Get the raw data from the sensor over the I2C bus.
  //
  BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

  //
  // Wait for callback to indicate request is complete.
  //
  ui8Status = BQ27510G3AppI2CWait();

  //
  // If initial read is not successful, exit w/o trying other reads.
  //
  if (ui8Status != 0)
  {
    return (0);
  }

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataHealthGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the data to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print health with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("State of Health: \t%3d.%01d %%\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  // Return success.
  //
  return (0);
}

//*****************************************************************************
//
// This function implements the "Cmd_status" command.
// This function displays all battery boosterpack status readings at once
//
//*****************************************************************************
int Cmd_status(int argc, char *argv[])
{
  float fData;
  uint_fast8_t ui8Status;
  int32_t i32IntegerPart;
  int32_t i32FractionPart;
  int32_t i32Current;

  //
  // Get the raw data from the sensor over the I2C bus.
  //
  BQ27510G3DataRead(&g_sBQ27510G3Inst, BQ27510G3AppCallback, &g_sBQ27510G3Inst);

  //
  // Wait for callback to indicate request is complete.
  //
  ui8Status = BQ27510G3AppI2CWait();

  //
  // If initial read is not successful, exit w/o trying other reads.
  //
  if (ui8Status != 0)
  {
    return (0);
  }

  //
  //*********  Capacity *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataCapacityFullAvailableGetFloat(&g_sBQ27510G3Inst, &fData);
  fData *= 1000.0f;

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print capacity with 1 digits of decimal precision.
  //
  g_tivaWare.UART.printf("Battery Capacity: \t%3d.%01d mAH\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* Remaining Capacity *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataCapacityRemainingGetFloat(&g_sBQ27510G3Inst, &fData);
  fData *= 1000.0f;

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("Remaining Capacity: \t%3d.%01d mAH\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //*********  Voltage *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataVoltageBatteryGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with 1 digit of decimal precision.
  //
  g_tivaWare.UART.printf("Battery Voltage: \t%3d.%01d mV\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //*********  Temperature *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataTemperatureInternalGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print temperature with one digits of decimal precision.
  //
  g_tivaWare.UART.printf("Internal Temperature: \t%3d.%01d C\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* State of Charge *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataChargeStateGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("State of Charge: \t%3d.%01d %%\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* Health *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataHealthGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("State of Health: \t%3d.%01d %%\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* Current *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataCurrentAverageGetFloat(&g_sBQ27510G3Inst, &fData);
  fData *= 1000.0f;

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  i32Current = i32IntegerPart;

  //
  // Print data with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("Average Current: \t%3d.%01d mA\t\t", i32IntegerPart, i32FractionPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* Cycles *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataCycleCountGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with one digit of decimal precision.
  //
  g_tivaWare.UART.printf("Recharge Cycles: \t%3d cyc\t\t", i32IntegerPart);

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n");

  //
  //********* Time to Empty *********
  //

  //
  // Call routine to retrieve data in float format.
  //
  BQ27510G3DataTimeToEmptyGetFloat(&g_sBQ27510G3Inst, &fData);

  //
  // Convert the float to an integer part and fraction part for easy
  // print.
  //
  i32IntegerPart = (int32_t) fData;
  i32FractionPart = (int32_t) (fData * 1000.0f);
  i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
  if (i32FractionPart < 0)
  {
    i32FractionPart *= -1;
  }

  //
  // Print data with one digit of decimal precision.
  //
  if (i32IntegerPart > 60)
  {
    g_tivaWare.UART.printf("Time Until Empty: \t%3d hrs\t\t", i32IntegerPart / 60);
  }
  else if (i32IntegerPart == -1)
  {
    g_tivaWare.UART.printf("Time Until Empty: \t   NA\t\t");
  }
  else
  {
    g_tivaWare.UART.printf("Time Until Empty: \t%3d min\t\t", i32IntegerPart);
  }

  //
  // Print new line.
  //
  g_tivaWare.UART.printf("\n\n");

  if (i32Current > 0)
  {
    g_tivaWare.UART.printf("The battery is charging!\r\n");
  }
  else
  {
    g_tivaWare.UART.printf("The battery is discharging!\r\n");
  }
  //
  // Return success.
  //
  return (0);
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int Cmd_help(int argc, char *argv[])
{
  tCmdLineEntry *pEntry;

  //
  // Print some header text.
  //
  g_tivaWare.UART.printf("\nAvailable commands\n");
  g_tivaWare.UART.printf("------------------\n");

  //
  // Point at the beginning of the command table.
  //
  pEntry = &g_psCmdTable[0];

  //
  // Enter a loop to read each entry from the command table.  The
  // end of the table has been reached when the command name is NULL.
  //
  while (pEntry->pcCmd)
  {
    //
    // Print the command name and the brief description.
    //
    g_tivaWare.UART.printf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

    //
    // Advance to the next entry in the table.
    //
    pEntry++;
  }

  //
  // Return success.
  //
  return (0);
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
{ "help", Cmd_help, "      : Display list of commands" },
{ "h", Cmd_help, "         : alias for help" },
{ "?", Cmd_help, "         : alias for help" },
{ "s", Cmd_status, "         : alias for status" },
{ "status", Cmd_status, "    : Display all status" },
{ "health", Cmd_health, "    : Display health" },
{ "charge", Cmd_charge, "    : Poll remaining charge" },
{ "temp", Cmd_temp, "      : Poll temperature" },
{ "volt", Cmd_volt, "      : Poll battery voltage" },
{ "current", Cmd_current, "   : Poll battery current" },
{ 0, 0, 0 } };

void BQ27510G3Module::init()
{
  //
  // Clear the terminal and print the welcome message.
  //
  tivaWare.UART.printf("\033[2J\033[H");
  tivaWare.UART.printf("Fuel Tank BoosterPack (BQ27510-G3) Example\n");
  tivaWare.UART.printf("Type 'help#' for a list of commands\n");
  tivaWare.UART.printf("\nBattpack> ");

  //
  // Initialize the BQ27510G3
  //
  BQ27510G3Init(&g_sBQ27510G3Inst, &tivaWare.I2C.instance, BQ27510G3_I2C_ADDRESS,
      BQ27510G3AppCallback, &g_sBQ27510G3Inst);

  //
  // Wait for initialization callback to indicate reset request is complete.
  //
  BQ27510G3AppI2CWait();

}

void BQ27510G3Module::execute()
{
  if (!theSensorAccessRepresentation.isNull() && !theSensorAccessRepresentation->active)
    return;
  //
  // Begin the data collection and printing.  Loop Forever.
  //
  // Infinite loop to process user commands from prompt
  //
  CheckForUserCommands();
}


