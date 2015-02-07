/*
 * InterruptModule.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#include "LM4FModule.h"
//
#include "inc/hw_sysctl.h"
#include "driverlib/timer.h"

MAKE_MODULE(LM4FModule)

static bool g_interruptedMPU9150;
//
static bool g_interruptedTMP006;
static bool g_interruptedISL29023;
//
static bool g_interruptedSysTick;
static bool g_interruptedTimer3;
//
//*****************************************************************************
//
// Hold the state of the buttons on the board.
//
//*****************************************************************************
volatile uint_fast8_t g_ui8Buttons;
//
static TivaWareController& g_tivaWare = TivaWareController::getInstance();

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port B interrupt event. For this
// application GPIO port B pin 2 is the interrupt line for the MPU9150
//
//*****************************************************************************
void IntGPIOb(void)
{
  uint32_t ui32Status;

  ui32Status = GPIOIntStatus(GPIO_PORTB_BASE, true);

  //
  // Clear all the pin interrupts that are set
  //
  GPIOIntClear(GPIO_PORTB_BASE, ui32Status);

  //
  // Check which GPIO caused the interrupt event.
  //
  if (ui32Status & GPIO_PIN_2)
  {
    //
    // The MPU9150 data ready pin was asserted so start an I2C transfer
    // to go get the latest data from the device.
    //
    g_interruptedMPU9150 = true;
  }
}
}

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port E interrupt event. For this
// application GPIO port E pin 0 is the interrupt line for the TMP006
//
//*****************************************************************************
void IntGPIOe(void)
{
  uint32_t ui32Status;

  ui32Status = GPIOIntStatus(GPIO_PORTE_BASE, true);

  //
  // Clear all the pin interrupts that are set
  //
  GPIOIntClear(GPIO_PORTE_BASE, ui32Status);

  if (ui32Status & GPIO_PIN_0)
  {
    //
    // This interrupt indicates a conversion is complete and ready to be
    // fetched.  So we start the process of getting the data.
    //
    g_interruptedTMP006 = true;
  }

  if (ui32Status & GPIO_PIN_5)
  {
    //
    // ISL29023 has indicated that the light level has crossed outside of
    // the intensity threshold levels set in INT_LT and INT_HT registers.
    //
    g_interruptedISL29023 = true;
  }

}

}

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port H interrupt event. For this
// application GPIO port H pin 2 is the interrupt line for the TMP006
//
// To use the sensor hub on BoosterPack 2 modify this function to accept and
// handle interrupts on GPIO Port P pin 5.  Also move the reference to this
// function in the startup file to GPIO Port P Int handler position in the
// vector table.
//
//*****************************************************************************
void GPIOPortHIntHandler(void)
{
  uint32_t ui32Status;

  ui32Status = GPIOIntStatus(GPIO_PORTH_BASE, true);

  //
  // Clear all the pin interrupts that are set
  //
  GPIOIntClear(GPIO_PORTH_BASE, ui32Status);

  if (ui32Status & GPIO_PIN_2)
  {
    //
    // This interrupt indicates a conversion is complete and ready to be
    // fetched.  So we start the process of getting the data.
    //
    g_interruptedTMP006 = true;
  }
}
}

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port E interrupt event. For this
// application GPIO port E pin 5 is the interrupt line for the ISL29023
//
// For this application this is a very low priority interrupt, we want to
// get notification of light values outside our thresholds but it is not the
// most important thing.
//
//*****************************************************************************
void GPIOPortEIntHandler(void)
{
  unsigned long ulStatus;

  ulStatus = GPIOIntStatus(GPIO_PORTE_BASE, true);

  //
  // Clear all the pin interrupts that are set
  //
  GPIOIntClear(GPIO_PORTE_BASE, ulStatus);

  if (ulStatus & GPIO_PIN_5)
  {
    //
    // ISL29023 has indicated that the light level has crossed outside of
    // the intensity threshold levels set in INT_LT and INT_HT registers.
    //
    g_interruptedISL29023 = true;
  }
}
}

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port M interrupt event. For this
// application GPIO port M pin 3 is the interrupt line for the MPU9150
//
// For BoosterPack 2 Interface use Port M pin 7.
//
//*****************************************************************************
void GPIOPortMIntHandler(void)
{
  unsigned long ulStatus;

  //
  // Get the status flags to see which pin(s) caused the interrupt.
  //
  ulStatus = GPIOIntStatus(GPIO_PORTM_BASE, true);

  //
  // Clear all the pin interrupts that are set
  //
  GPIOIntClear(GPIO_PORTM_BASE, ulStatus);

  //
  // Check if this is an interrupt on the MPU9150 interrupt line.
  //
  // For BoosterPack 2 use Pin 7 instead.
  //
  if (ulStatus & GPIO_PIN_3)
  {
    //
    // Turn on the LED to show that transaction is starting.
    //
    //LEDWrite(CLP_D3 | CLP_D4, CLP_D3);

    //
    // MPU9150 Data is ready for retrieval and processing.
    //
    g_interruptedMPU9150 = true;
  }
}
}

extern "C"
{
//*****************************************************************************
//
// The interrupt handler for the third timer interrupt.
//
//*****************************************************************************
void Timer3AIntHandler(void)
{
  //
  // Clear the timer interrupt.
  //
  ROM_TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
  g_interruptedTimer3 = true;
}

}

extern "C"
{
//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to move the mouse.
//
//*****************************************************************************
void LM4FSysTickHandler(uint32_t ui32TimeMS)
{
  g_ui8Buttons = g_tivaWare.BUTTONS.pollButtons();
  g_interruptedSysTick = true;
}
}

void LM4FModule::init()
{
  registerSysTickCb(LM4FSysTickHandler);
}

void LM4FModule::update(InterruptVectorRepresentation& theInterruptVectorRepresentation)
{
  theInterruptVectorRepresentation.interruptedMPU9150 = g_interruptedMPU9150;
  theInterruptVectorRepresentation.interruptedTMP006 = g_interruptedTMP006;
  theInterruptVectorRepresentation.interruptedISL29023 = g_interruptedISL29023;
  theInterruptVectorRepresentation.interruptedSysTick = g_interruptedSysTick;
  theInterruptVectorRepresentation.interruptedTimer3 = g_interruptedTimer3;
  theInterruptVectorRepresentation.buttons = g_ui8Buttons;
  g_interruptedMPU9150 = g_interruptedTMP006 = g_interruptedISL29023 = g_interruptedSysTick =
      g_interruptedTimer3 = false;
}
