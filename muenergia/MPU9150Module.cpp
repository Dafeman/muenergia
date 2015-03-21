/*
 * MPU9150Module.cpp
 *
 *  Created on: Sep 11, 2014
 *      Author: sam
 */

#include "MPU9150Module.h"
//
#include "sensorlib/hw_mpu9150.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/ak8975.h"
#include "sensorlib/mpu9150.h"
#include "sensorlib/comp_dcm.h"
//
#include <algorithm>

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Nine Axis Sensor Fusion with the MPU9150 and Complimentary-Filtered
//! DCM (compdcm_mpu9150)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and SensHub BoosterPack to obtain nine axis motion measurements
//! from the MPU9150.  The example fuses the nine axis measurements into a set
//! of Euler angles: roll, pitch and yaw.  It also produces the rotation
//! quaternions.  The fusion mechanism demonstrated is complimentary-filtered
//! direct cosine matrix (DCM) algorithm is provided as part of the Sensor
//! Library.
//!
//! Connect a serial terminal program to the LaunchPad's ICDI virtual serial
//! port at 115,200 baud.  Use eight bits per byte, no parity and one stop bit.
//! The raw sensor measurements, Euler angles and quaternions are printed to
//! the terminal.  The RGB LED begins to blink at 1Hz after initialization is
//! completed and the example application is running.
//
//*****************************************************************************

//*****************************************************************************
//
// Define MPU9150 I2C Address.
//
//*****************************************************************************
#define MPU9150_I2C_ADDRESS     0x68

//*****************************************************************************
//
// Define MPU9150 data sampling frequency.
//
//*****************************************************************************
#define MOTION_SAMPLE_FREQ_HZ   50

//*****************************************************************************
//
// Weights the DCM should use for each sensor.  Must add to 1.0
//
//*****************************************************************************
#define DCM_MAG_WEIGHT          0.2f
#define DCM_GYRO_WEIGHT         0.6f
#define DCM_ACCEL_WEIGHT        0.2f

//*****************************************************************************
//
// Define the states of the motion state machine.
//
//*****************************************************************************
#define MOTION_STATE_INIT       0
#define MOTION_STATE_RUN        1
#define MOTION_STATE_ERROR      2

//*****************************************************************************
//
// Global instance structure for the MPU9150 sensor driver.
//
//*****************************************************************************
tMPU9150 g_sMPU9150Inst;

//*****************************************************************************
//
// Global Instance structure to manage the DCM state.
//
//*****************************************************************************
tCompDCM g_sCompDCMInst;

//*****************************************************************************
//
// Global state variable for the motion state machine.
//
//*****************************************************************************
uint_fast8_t g_ui8MotionState;

//*****************************************************************************
//
// Global flags to alert main that MPU9150 I2C transaction is complete
//
//*****************************************************************************
static volatile uint_fast8_t g_vui8I2CDoneFlag;

//*****************************************************************************
//
// Global flags to alert main that MPU9150 I2C transaction error has occurred.
//
//*****************************************************************************
static volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// Global storage for most recent Euler angles and Sensor Data
//
//*****************************************************************************
float g_pfAccel[3];
float g_pfGyro[3];
float g_pfMag[3];
float g_pfEulers[3];
float g_pfQuaternion[4];

uint_fast16_t g_pui16Accel[3];
uint_fast16_t g_pui16Gyro[3];
uint_fast16_t g_pui16Mag[3];

static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// MPU9150 Sensor callback function.  Called at the end of MPU9150 sensor
// driver transactions. This is called from I2C interrupt context.
//
//*****************************************************************************
void MotionCallback(void* pvCallbackData, uint_fast8_t ui8Status)
{
  //
  // If the transaction succeeded set the data flag to indicate to
  // application that this transaction is complete and data may be ready.
  //
  if (ui8Status == I2CM_STATUS_SUCCESS)
  {
    if (g_ui8MotionState == MOTION_STATE_RUN)
    {
      //
      // Get local copies of the raw motion sensor data.
      //
      MPU9150DataAccelGetRaw(&g_sMPU9150Inst, g_pui16Accel, g_pui16Accel + 1, g_pui16Accel + 2);
      MPU9150DataAccelGetFloat(&g_sMPU9150Inst, g_pfAccel, g_pfAccel + 1, g_pfAccel + 2);

      MPU9150DataGyroGetRaw(&g_sMPU9150Inst, g_pui16Gyro, g_pui16Gyro + 1, g_pui16Gyro + 2);
      MPU9150DataGyroGetFloat(&g_sMPU9150Inst, g_pfGyro, g_pfGyro + 1, g_pfGyro + 2);

      MPU9150DataMagnetoGetRaw(&g_sMPU9150Inst, g_pui16Mag, g_pui16Mag + 1, g_pui16Mag + 2);
      MPU9150DataMagnetoGetFloat(&g_sMPU9150Inst, g_pfMag, g_pfMag + 1, g_pfMag + 2);

      //
      // Update the DCM. Do this in the ISR so that timing between the
      // calls is consistent and accurate.
      //
      CompDCMMagnetoUpdate(&g_sCompDCMInst, g_pfMag[0], g_pfMag[1], g_pfMag[2]);
      CompDCMAccelUpdate(&g_sCompDCMInst, g_pfAccel[0], g_pfAccel[1], g_pfAccel[2]);
      CompDCMGyroUpdate(&g_sCompDCMInst, -g_pfGyro[0], -g_pfGyro[1], -g_pfGyro[2]);
      CompDCMUpdate(&g_sCompDCMInst);
    }

    g_vui8I2CDoneFlag = 1;
  }

  //
  // Store the most recent status in case it was an error condition
  //
  g_vui8ErrorFlag = ui8Status;
}

//*****************************************************************************
//
// MPU9150 Application error handler.
//
//*****************************************************************************
void MotionErrorHandler(char * pcFilename, uint_fast32_t ui32Line)
{
  //
  // Set terminal color to red and print error status and locations
  //
  g_tivaWare.UART.printf("\033[31;1m");
  g_tivaWare.UART.printf("Error: %d, File: %s, Line: %d\nSee I2C status definitions in "
      "utils\\i2cm_drv.h\n", g_vui8ErrorFlag, pcFilename, ui32Line);

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
// Function to wait for the MPU9150 transactions to complete.
//
//*****************************************************************************
void MotionI2CWait(char* pcFilename, uint_fast32_t ui32Line)
{
  //
  // Put the processor to sleep while we wait for the I2C driver to
  // indicate that the transaction is complete.
  //
  while ((g_vui8I2CDoneFlag == 0) && (g_vui8ErrorFlag == 0))
  {
    //
    // Do Nothing
    //
  }

  //
  // clear the done flag.
  //
  g_vui8I2CDoneFlag = 0;

  //
  // If an error occurred call the error handler immediately.
  //
  if (g_vui8ErrorFlag)
  {
    MotionErrorHandler(pcFilename, ui32Line);
    g_vui8ErrorFlag = 0;
  }

  return;
}

void MPU9150Module::init()
{
  //
  // Set the motion state to initializing.
  //
  g_ui8MotionState = MOTION_STATE_INIT;

  //
  // Initialize the MPU9150 Driver.
  //
  MPU9150Init(&g_sMPU9150Inst, &g_tivaWare.I2C.instance, MPU9150_I2C_ADDRESS, MotionCallback,
      &g_sMPU9150Inst);

  //
  // Wait for transaction to complete
  //
  MotionI2CWait(__FILE__, __LINE__);

  //
  // Write application specifice sensor configuration such as filter settings
  // and sensor range settings.
  //
  g_sMPU9150Inst.pui8Data[0] = MPU9150_CONFIG_DLPF_CFG_94_98;
  g_sMPU9150Inst.pui8Data[1] = MPU9150_GYRO_CONFIG_FS_SEL_250;
  g_sMPU9150Inst.pui8Data[2] = (MPU9150_ACCEL_CONFIG_ACCEL_HPF_5HZ |
  MPU9150_ACCEL_CONFIG_AFS_SEL_2G);
  MPU9150Write(&g_sMPU9150Inst, MPU9150_O_CONFIG, g_sMPU9150Inst.pui8Data, 3, MotionCallback,
      &g_sMPU9150Inst);

  //
  // Wait for transaction to complete
  //
  MotionI2CWait(__FILE__, __LINE__);

  //
  // Configure the data ready interrupt pin output of the MPU9150.
  //
  g_sMPU9150Inst.pui8Data[0] = (MPU9150_INT_PIN_CFG_INT_LEVEL | MPU9150_INT_PIN_CFG_INT_RD_CLEAR |
  MPU9150_INT_PIN_CFG_LATCH_INT_EN);
  g_sMPU9150Inst.pui8Data[1] = MPU9150_INT_ENABLE_DATA_RDY_EN;
  MPU9150Write(&g_sMPU9150Inst, MPU9150_O_INT_PIN_CFG, g_sMPU9150Inst.pui8Data, 2, MotionCallback,
      &g_sMPU9150Inst);

  //
  // Wait for transaction to complete
  //
  MotionI2CWait(__FILE__, __LINE__);

  //
  // Initialize the DCM system.
  //
  CompDCMInit(&g_sCompDCMInst, 1.0f / ((float) MOTION_SAMPLE_FREQ_HZ),
  DCM_ACCEL_WEIGHT, DCM_GYRO_WEIGHT, DCM_MAG_WEIGHT);

  tivaWare.UART.printf("\033[2J\033[H");
  tivaWare.UART.printf("MPU9150 9-Axis Simple Data Application Example\n\n");
  tivaWare.UART.printf("\033[20GX\033[31G|\033[43GY\033[54G|\033[66GZ\n\n");
  tivaWare.UART.printf("Accel\033[8G|\033[31G|\033[54G|\n\n");
  tivaWare.UART.printf("Gyro\033[8G|\033[31G|\033[54G|\n\n");
  tivaWare.UART.printf("Mag\033[8G|\033[31G|\033[54G|\n\n");
  tivaWare.UART.printf("\n\033[20GRoll\033[31G|\033[43GPitch\033[54G|\033[66GYaw\n\n");
  tivaWare.UART.printf("Eulers\033[8G|\033[31G|\033[54G|\n\n");

  tivaWare.UART.printf("\n\033[17GQ1\033[26G|\033[35GQ2\033[44G|\033[53GQ3\033[62G|"
      "\033[71GQ4\n\n");
  tivaWare.UART.printf("Q\033[8G|\033[26G|\033[44G|\033[62G|\n\n");

}

void MPU9150Module::update(MPU9150Representation& theMPU9150Representation)
{
  if (!theSensorAccessRepresentation.isNull() && !theSensorAccessRepresentation->active)
    return;

  theMPU9150Representation.bUpdated = false;
  if (!(/*theInterruptVectorRepresentation->interruptedMPU9150 ||*/theInterruptVectorRepresentation->interruptedTimer3/*50Hz*/))
    return;

  //
  // The MPU9150 data ready pin was asserted so start an I2C transfer
  // to go get the latest data from the device.
  //
  MPU9150DataRead(&g_sMPU9150Inst, MotionCallback, &g_sMPU9150Inst);

  //
  // Wait for transaction to complete
  //
  MotionI2CWait(__FILE__, __LINE__);

  // Slow down by 10 ms to send data as UART is much slower
  // than ARM
  //ROM_SysCtlDelay(ROM_SysCtlClockGet() / (100 * 3));

  switch (g_ui8MotionState)
  {
    //
    // This is our initial data set from the MPU9150, start the DCM.
    //
    case MOTION_STATE_INIT:
    {
      //
      // Check the read data buffer of the MPU9150 to see if the
      // Magnetometer data is ready and present. This may not be the case
      // for the first few data captures.
      //
      if (g_sMPU9150Inst.pui8Data[14] & AK8975_ST1_DRDY)
      {
        //
        // Get local copy of Accel and Mag data to feed to the DCM
        // start.
        //
        MPU9150DataAccelGetRaw(&g_sMPU9150Inst, g_pui16Accel, g_pui16Accel + 1, g_pui16Accel + 2);
        MPU9150DataAccelGetFloat(&g_sMPU9150Inst, g_pfAccel, g_pfAccel + 1, g_pfAccel + 2);
        MPU9150DataMagnetoGetRaw(&g_sMPU9150Inst, g_pui16Mag, g_pui16Mag + 1, g_pui16Mag + 2);
        MPU9150DataMagnetoGetFloat(&g_sMPU9150Inst, g_pfMag, g_pfMag + 1, g_pfMag + 2);
        MPU9150DataGyroGetRaw(&g_sMPU9150Inst, g_pui16Gyro, g_pui16Gyro + 1, g_pui16Gyro + 2);
        MPU9150DataGyroGetFloat(&g_sMPU9150Inst, g_pfGyro, g_pfGyro + 1, g_pfGyro + 2);

        //
        // Feed the initial measurements to the DCM and start it.
        // Due to the structure of our MotionMagCallback function,
        // the floating point magneto data is already in the local
        // data buffer.
        //
        CompDCMMagnetoUpdate(&g_sCompDCMInst, g_pfMag[0], g_pfMag[1], g_pfMag[2]);
        CompDCMAccelUpdate(&g_sCompDCMInst, g_pfAccel[0], g_pfAccel[1], g_pfAccel[2]);
        CompDCMStart(&g_sCompDCMInst);

        //
        // Proceed to the run state.
        //
        g_ui8MotionState = MOTION_STATE_RUN;
      }

      //
      // Finished
      //
      break;
    }

      //
      // DCM has been started and we are ready for normal operations.
      //
    case MOTION_STATE_RUN:
    {
      //
      // Get the latest Euler data from the DCM. DCMUpdate is done
      // inside the interrupt routine to insure it is not skipped and
      // that the timing is consistent.
      //
      CompDCMComputeEulers(&g_sCompDCMInst, g_pfEulers, g_pfEulers + 1, g_pfEulers + 2);

      //
      // Get Quaternions.
      //
      CompDCMComputeQuaternion(&g_sCompDCMInst, g_pfQuaternion);

      //
      // convert mag data to micro-tesla for better human interpretation.
      //
      g_pfMag[0] *= 1e6;
      g_pfMag[1] *= 1e6;
      g_pfMag[2] *= 1e6;

      //
      // Convert Eulers to degrees. 180/PI = 57.29...
      // Convert Yaw to 0 to 360 to approximate compass headings.
      //
      //g_pfEulers[0] *= 57.295779513082320876798154814105f;
      //g_pfEulers[1] *= 57.295779513082320876798154814105f;
      //g_pfEulers[2] *= 57.295779513082320876798154814105f;
      //if (g_pfEulers[2] < 0)
      //{
      //  g_pfEulers[2] += 360.0f;
      //}

      //
      // Finished
      //
      break;
    }

      //
      // An I2C error has occurred at some point. Usually these are due to
      // asynchronous resets of the main MCU and the I2C peripherals. This
      // can cause the slave to hold the bus and the MCU to think it cannot
      // send.  In practice there are ways to clear this condition.  They are
      // not implemented here.  To clear power cycle the board.
      //
    case MOTION_STATE_ERROR:
    {
      // nothing
      break;
    }
  }

  std::copy(g_pfAccel, g_pfAccel + 3, theMPU9150Representation.fAccel);
  std::copy(g_pfGyro, g_pfGyro + 3, theMPU9150Representation.fGyro);
  std::copy(g_pfMag, g_pfMag + 3, theMPU9150Representation.fMag);
  std::copy(g_pfEulers, g_pfEulers + 3, theMPU9150Representation.fEulers);
  std::copy(g_pfQuaternion, g_pfQuaternion + 4, theMPU9150Representation.fQuaternion);

  std::copy(g_pui16Accel, g_pui16Accel + 3, theMPU9150Representation.pui16Accel);
  std::copy(g_pui16Gyro, g_pui16Gyro + 3, theMPU9150Representation.pui16Gyro);
  std::copy(g_pui16Mag, g_pui16Mag + 3, theMPU9150Representation.pui16Mag);

  MPU9150AccelFactor(&g_sMPU9150Inst, &theMPU9150Representation.fAccelFactor);
  MPU9150GyroFactor(&g_sMPU9150Inst, &theMPU9150Representation.fGyroFactor);

  theMPU9150Representation.bUpdated = true;

  if (false) /*DEBUG*/
  {
    // Conversion test
    static MPU9150Representation repDATA;
    for (int i = 0; i < 3; i++)
    {
      repDATA.fAccel[i] = ((float) (int16_t) (theMPU9150Representation.pui16Accel[i])
          * theMPU9150Representation.fAccelFactor);
      repDATA.fGyro[i] = ((float) (int16_t) (theMPU9150Representation.pui16Gyro[i])
          * theMPU9150Representation.fGyroFactor);
      repDATA.fMag[i] = ((float) theMPU9150Representation.pui16Mag[i]);
    }
    // debug
    int_fast32_t i32IPart[16], i32FPart[16];
    uint_fast32_t ui32Idx, ui32CompDCMStarted;
    float* pfData = &theMPU9150Representation.fAccel[0];
    //float* pfData = &repDATA.fAccel[0];

    //
    // Now drop back to using the data as a single array for the
    // purpose of decomposing the float into a integer part and a
    // fraction (decimal) part.
    //
    for (ui32Idx = 0; ui32Idx < 16; ui32Idx++)
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
    tivaWare.UART.printf("\033[5;17H%3d.%03d", i32IPart[0], i32FPart[0]);
    tivaWare.UART.printf("\033[5;40H%3d.%03d", i32IPart[1], i32FPart[1]);
    tivaWare.UART.printf("\033[5;63H%3d.%03d", i32IPart[2], i32FPart[2]);

    //
    // Print the angular velocities in the table.
    //
    tivaWare.UART.printf("\033[7;17H%3d.%03d", i32IPart[3], i32FPart[3]);
    tivaWare.UART.printf("\033[7;40H%3d.%03d", i32IPart[4], i32FPart[4]);
    tivaWare.UART.printf("\033[7;63H%3d.%03d", i32IPart[5], i32FPart[5]);

    //
    // Print the magnetic data in the table.
    //
    tivaWare.UART.printf("\033[9;17H%3d.%03d", i32IPart[6], i32FPart[6]);
    tivaWare.UART.printf("\033[9;40H%3d.%03d", i32IPart[7], i32FPart[7]);
    tivaWare.UART.printf("\033[9;63H%3d.%03d", i32IPart[8], i32FPart[8]);

    //
    // Print the Eulers in a table.
    //
    tivaWare.UART.printf("\033[14;17H%3d.%03d", i32IPart[9], i32FPart[9]);
    tivaWare.UART.printf("\033[14;40H%3d.%03d", i32IPart[10], i32FPart[10]);
    tivaWare.UART.printf("\033[14;63H%3d.%03d", i32IPart[11], i32FPart[11]);

    //
    // Print the quaternions in a table format.
    //
    tivaWare.UART.printf("\033[19;14H%3d.%03d", i32IPart[12], i32FPart[12]);
    tivaWare.UART.printf("\033[19;32H%3d.%03d", i32IPart[13], i32FPart[13]);
    tivaWare.UART.printf("\033[19;50H%3d.%03d", i32IPart[14], i32FPart[14]);
    tivaWare.UART.printf("\033[19;68H%3d.%03d", i32IPart[15], i32FPart[15]);
    tivaWare.UART.printf("\n");
  }

}


