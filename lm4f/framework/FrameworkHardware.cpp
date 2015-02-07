/*
 * FrameworkHardware.cpp
 *
 *  Created on: Sep 8, 2014
 *      Author: sam
 */

#include "Framework.h"

#if defined(EMBEDDED_MODE)
//
#include <stdint.h>
#include <stdbool.h>
//
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "sensorlib/i2cm_drv.h"
#include "driverlib/timer.h"
//
#endif

void Controller::configureCLOCK()
{
  ///
  // Enable lazy stacking for interrupt handlers.  This allows floating-point
  // instructions to be used within interrupt handlers, but at the expense of
  // extra stack usage.
  //
  ROM_FPUEnable();
  ROM_FPULazyStackingEnable();

#ifdef TARGET_IS_BLIZZARD_RB1
  //
  //  Run at system clock at 80MHz
  //
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  tivaWare.CLOCK.ui32SrcClock = 16000000;
  tivaWare.CLOCK.ui32SysClock = ROM_SysCtlClockGet(); // This is F_CPU
#else
      //
      // Configure the system clock to run at 120MHz.
      //
      tivaWare.CLOCK.ui32SysClock = MAP_SysCtlClockFreqSet(
          (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
              SYSCTL_CFG_VCO_480), 120000000);
      tivaWare.CLOCK.ui32SrcClock = tivaWare.CLOCK.ui32SysClock;
      //
      // Configure the device pins for this board.
      // This application does not use Ethernet or USB.
      //
      PinoutSet(false, false);
#endif

}

void Controller::configureUART(unsigned long baudRate)
{
  //
  // Enable the GPIO Peripheral used by the UART.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  //
  // Enable UART0
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

  //
  // Configure GPIO Pins for UART mode.
  //
  ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
  ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

#ifdef TARGET_IS_BLIZZARD_RB1
  //
  // Use the internal 16MHz oscillator as the UART clock source.
  //
  UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
#endif

  //
  // Initialize the UART for console I/O.
  //
  tivaWare.UART.initUART(0, baudRate, tivaWare.CLOCK.ui32SrcClock, tivaWare.CLOCK.ui32SysClock);
  tivaWare.UART.printf("UART0_BASE with UARTStdioIntHandler\n");
}

void Controller::configureLED()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  // Setup the color of the RGB LED.
  //
  // Initialize the RGB Driver and start RGB blink operation.
  //
  tivaWare.LED.initRGB(0);
  tivaWare.LED.colorSetRGB(0, 0, 0);
  tivaWare.LED.intensitySetRGB(0.1f);
  tivaWare.LED.enableRGB();
#else
  //
  // Turn on the LED to show a transaction is starting.
  //
  LEDWrite(CLP_D3 | CLP_D4, CLP_D3);
#endif
}

void Controller::configureBUTTON()
{
  TivaWareController::getInstance().BUTTONS.initButtons();
  tivaWare.UART.printf("BUTTON: enabled\n");
}

void Controller::configureRADIO()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  if (tivaWare.LPRF.active)
  {
    //
    // Initialize the Radio Systems.
    //
    tivaWare.LPRF.initLPRF();
    tivaWare.UART.printf("UART1_BASE with RemoTIUARTIntHandler; target=%d\n",
        tivaWare.LPRF.isTarget);
  }
#endif
}

void Controller::configurePORT()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  //
  // Enable port B used for motion interrupt.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

  //
  // Enable the peripherals used by the framework.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

  ROM_IntMasterDisable();

  //
  // Configure and Enable the GPIO interrupt. Used for INT signal from the
  // MPU9150
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_2);
  GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_2);
  ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_FALLING_EDGE);
  ROM_IntEnable(INT_GPIOB);

  //
  // Configure and Enable the GPIO interrupt. Used for DRDY from the TMP006
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0);
  GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_0);
  ROM_GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);

  //
  // Configure and Enable the GPIO interrupt. Used for INT signal from the
  // ISL29023
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);
  GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_5);
  ROM_GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_FALLING_EDGE);
  ROM_IntEnable(INT_GPIOE);
#else
  //
  // Configure and Enable the GPIO interrupt. Used for DRDY from the TMP006
  //
  // For BoosterPack 2 interface use PP5.
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_2);
  GPIOIntEnable(GPIO_PORTH_BASE, GPIO_PIN_2);
  ROM_GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_FALLING_EDGE);
  ROM_IntEnable(INT_GPIOH);

  //
  // Configure and Enable the GPIO interrupt. Used for INT signal from the
  // ISL29023
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);
  GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_5);
  ROM_GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_FALLING_EDGE);
  ROM_IntEnable(INT_GPIOE);

  //
  // Configure and Enable the GPIO interrupt. Used for INT signal from the
  // MPU9150.
  //
  // For BoosterPack 2 interface change this to PM7.
  //
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_3);
  GPIOIntEnable(GPIO_PORTM_BASE, GPIO_PIN_3);
  ROM_GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_FALLING_EDGE);
  ROM_IntEnable(INT_GPIOM);

#endif

}

// Hardware specific implementation
void Controller::configureHAL()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  //
  // Keep only some parts of the systems running while in sleep mode.
  // GPIOB is for the MPU9150 interrupt pin.
  // GPIOE is for the ISL29023 interrupt pin.
  // UART0 is the virtual serial port
  // TIMER0, TIMER1 and WTIMER5 are used by the RGB driver
  // I2C3 is the I2C interface to the ISL29023
  //
  ROM_SysCtlPeripheralClockGating(true);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C3);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WTIMER5);
#else
  //
  // Keep only some parts of the systems running while in sleep mode.
  // GPIOH is for the TMP006 data ready interrupt.
  // GPIOE is for the ISL29023 interrupt pin.
  // UART0 is the virtual serial port
  // TIMER0, TIMER1 and WTIMER5 are used by the RGB driver
  // I2C7 is the I2C interface to the TMP006
  //
  // For BoosterPack 2 Interface use GPIOP and I2C8
  //
  ROM_SysCtlPeripheralClockGating(true);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOH);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOM);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C7);
#endif

  //
  // Enable timer 3
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);

  //
  // Enable interrupts to the processor.
  //
  ROM_IntMasterEnable();

  //
  // Configure the 32-bit periodic timer 3 for x times per second
  //
  ROM_TimerConfigure(TIMER3_BASE, TIMER_CFG_PERIODIC);
  ROM_TimerLoadSet(TIMER3_BASE, TIMER_A, tivaWare.CLOCK.ui32SysClock / 50);
  //
  // Setup the interrupts for the timer timeouts.
  //
  ROM_IntEnable(INT_TIMER3A);
  ROM_TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
  //
  // Enable the timers.
  //
  ROM_TimerEnable(TIMER3_BASE, TIMER_A);

  //
  //Initialize Timer5 to be used as time-tracker since beginning of time
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);  //not tied to launchpad pin
  ROM_TimerConfigure(TIMER5_BASE, TIMER_CFG_PERIODIC_UP);
  ROM_TimerLoadSet(TIMER5_BASE, TIMER_A, tivaWare.CLOCK.ui32SysClock / 1000);
  ROM_IntEnable(INT_TIMER5A);
  ROM_TimerIntEnable(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
  ROM_TimerEnable(TIMER5_BASE, TIMER_A);

  //
  // Set the system tick to fire x times per second, i.e., (x := 50, 100 Hz)
  //
  ROM_SysTickPeriodSet(tivaWare.CLOCK.ui32SysClock / tivaWare.CLOCK.ui32SysTickHz);
  ROM_SysTickIntEnable();
  ROM_SysTickEnable();

}

void Controller::configurePRIORITY()
{
#ifdef TARGET_IS_BLIZZARD_RB1
  // Configure desired interrupt priorities.  Setting the I2C interrupt to be
  // of more priority than SysTick and the GPIO interrupt means those
  // interrupt routines can use the I2CM_DRV Application context does not use
  // I2CM_DRV API and GPIO and SysTick are at the same priority level. This
  // prevents re-entrancy problems with I2CM_DRV but keeps the MCU in sleep
  // state as much as possible. UART is at least priority so it can operate
  // in the background.
  //
  // Configure desired interrupt priorities. This makes certain that the DCM
  // is fed data at a consistent rate. Lower numbers equal higher priority.
  //
  ROM_IntPrioritySet(INT_I2C3, 0x00);
  ROM_IntPrioritySet(INT_GPIOB, 0x10);
  ROM_IntPrioritySet(FAULT_SYSTICK, 0x20);
  ROM_IntPrioritySet(INT_UART1, 0x60);
  ROM_IntPrioritySet(INT_UART0, 0x70);
  ROM_IntPrioritySet(INT_GPIOE, 0x80);
  ROM_IntPrioritySet(INT_WTIMER5B, 0x80);
#else
  //
  // Configure desired interrupt priorities.  Setting the I2C interrupt to be
  // of more priority than SysTick and the GPIO interrupt means those
  // interrupt routines can use the I2CM_DRV Application context does not use
  // I2CM_DRV API and GPIO and SysTick are at the same priority level. This
  // prevents re-entrancy problems with I2CM_DRV but keeps the MCU in sleep
  // state as much as possible. UART is at least priority so it can operate
  // in the background.
  //
  // For BoosterPack 2 use I2C8.
  //
  ROM_IntPrioritySet(INT_I2C7, 0x00);
  ROM_IntPrioritySet(INT_GPIOM, 0x20);
  ROM_IntPrioritySet(FAULT_SYSTICK, 0x30);
  ROM_IntPrioritySet(INT_UART1, 0x40);
  ROM_IntPrioritySet(INT_UART0, 0x50);
  ROM_IntPrioritySet(INT_GPIOH, 0x60);
  ROM_IntPrioritySet(INT_GPIOE, 0x70);

#endif

}

void Controller::configureI2C()
{
  tivaWare.I2C.initI2CM();
#ifdef TARGET_IS_BLIZZARD_RB1
  //
  // The I2C3 peripheral must be enabled before use.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

  //
  // Configure the pin muxing for I2C3 functions on port D0 and D1.
  // This step is not necessary if your part does not support pin muxing.
  //
  ROM_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
  ROM_GPIOPinConfigure(GPIO_PD1_I2C3SDA);

  //
  // Select the I2C function for these pins.  This function will also
  // configure the GPIO pins pins for I2C operation, setting them to
  // open-drain operation with weak pull-ups.  Consult the data sheet
  // to see which functions are allocated per pin.
  //
  GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
  ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

  //*****************************************************************************
  //
  // Called by the NVIC as a result of I2C Interrupt. I2C7 is the I2C connection
  // to the TMP006 for BoosterPack 1 interface.  I2C8 must be used for
  // BoosterPack 2 interface.  Must also move this function pointer in the
  // startup file interrupt vector table for your tool chain if using BoosterPack
  // 2 interface headers.
  //
  //*****************************************************************************
  //
  // Initialize I2C3 peripheral.
  //
  I2CMInit(&tivaWare.I2C.instance, I2C3_BASE, INT_I2C3, 0xff, 0xff, tivaWare.CLOCK.ui32SysClock);
#else
  //
  // The I2C7 peripheral must be enabled before use.
  //
  // For BoosterPack 2 interface use I2C8.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);

  //
  // Configure the pin muxing for I2C7 functions on port D0 and D1.
  // This step is not necessary if your part does not support pin muxing.
  //
  // For BoosterPack 2 interface use PA2 and PA3.
  //
  ROM_GPIOPinConfigure(GPIO_PD0_I2C7SCL);
  ROM_GPIOPinConfigure(GPIO_PD1_I2C7SDA);

  //
  // Select the I2C function for these pins.  This function will also
  // configure the GPIO pins pins for I2C operation, setting them to
  // open-drain operation with weak pull-ups.  Consult the data sheet
  // to see which functions are allocated per pin.
  //
  // For BoosterPack 2 interface use PA2 and PA3.
  //
  GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
  ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

  //
  // Initialize I2C peripheral.
  //
  // For BoosterPack 2 interface use I2C8
  //
  I2CMInit(&tivaWare.I2C.instance, I2C7_BASE, INT_I2C7, 0xff, 0xff, tivaWare.CLOCK.ui32SysClock);

#endif
  tivaWare.UART.printf("I2CM: I2CX with frameworkI2CIntHandler\n");
}

extern "C"
{
//*****************************************************************************
//
// Called by the NVIC as a result of I2C3 Interrupt.
//
//*****************************************************************************
void I2CXIntHandler(void)
{
  //
  // Pass through to the I2CM interrupt handler provided by sensor library.
  // This is required to be at application level so that I2CMIntHandler can
  // receive the instance structure pointer as an argument.
  //
  if (TivaWareController::getInstance().I2C.initialized())
    I2CMIntHandler(&TivaWareController::getInstance().I2C.instance);
}
}
