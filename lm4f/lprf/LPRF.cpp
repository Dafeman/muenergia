/*
 * LPRF.cpp
 *
 *  Created on: Sep 22, 2014
 *      Author: sam
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
//
#include "Energia.h"
//
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
//
#include "remoti_uart.h"
#include "remoti_npi.h"
#include "remoti_rti.h"
#include "remoti_rtis.h"
#include "remoti_zid.h"
//
#include "framework/TivaWare.h"
#include "framework/Framework.h"
#include "LPRF.h"

///////////////////////////////////////////////////////////////////////////////
LPRF* LPRF::theInstance = 0;

LPRF::LPRF() :
    nodeType(LPRF_CONTROLLER_TYPE), target(NULL), controller(NULL), duplex(false)
{
}

LPRF::~LPRF()
{
}

LPRF& LPRF::getInstance()
{
  if (!theInstance)
    theInstance = new LPRF;
  return *theInstance;
}

void LPRF::deleteInstance()
{
  if (theInstance)
    delete theInstance;
  theInstance = 0;
}

///////////////////////////////////////////////////////////////////////////////
#define LPRF_EVENT              3
//
// Uncomment/comment the following lines to switch between using the sensor hub
// booster pack or the em header booster pack on the receiver.  The EM header
// booster pack is much cheaper, but has the reset line routed different than
// the sensorhub booster pack.  Of course, this is moot if you're using the
// transmitter.
//

#define USE_SENSHUB
//#define USE_EM_ADAPTER

#if defined(USE_SENSHUB)
#define EM_RESET_SYSCTL_PERIPH  SYSCTL_PERIPH_GPIOC
#define EM_RESET_GPIO_PORT      GPIO_PORTC_BASE
#define EM_RESET_GPIO_PIN       GPIO_PIN_7
#elif defined(USE_EM_ADAPTER)
#define EM_RESET_SYSCTL_PERIPH  SYSCTL_PERIPH_GPIOA
#define EM_RESET_GPIO_PORT      GPIO_PORTA_BASE
#define EM_RESET_GPIO_PIN       GPIO_PIN_4
#else
#error "must define whether using em header or sens hub to talk to RF board"
#endif //defined(USE_SENSHUB or USE_EM_ADAPTER)

//*****************************************************************************
//
// Global system tick counter.  incremented by SysTickIntHandler.
//
//*****************************************************************************
volatile uint_fast32_t g_ui32SysTickCount;

volatile uint_fast32_t g_ui32MaxPairConfTries;
volatile uint_fast32_t g_ui32MaxAllowPairConfTries;

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile uint_fast32_t g_ui32Events;

//*****************************************************************************
//
// Global storage to count blink ticks.  This sets blink timing after error.
//
//*****************************************************************************
uint32_t g_ui32RGBLPRFBlinkCounter;

//*****************************************************************************
//
// Global storage for buttons state of previous LPRF packet
//
//*****************************************************************************
uint_fast8_t g_ui8LPRFButtonsPrev;

//*****************************************************************************
//
// Current state of the LPRF network link.
//
//*****************************************************************************
volatile uint_fast8_t g_vui8LinkState;

//*****************************************************************************
//
// The index into our pairing table that contains the current active link.
//
//*****************************************************************************
uint8_t g_ui8LinkDestIndex;
uint8_t g_ui8Sending;
uint8_t g_ui8ConfigureLPRF;
uint8_t g_ui8ConnectedLPRF = false;
Vector<uint8_t> g_ui8LinkDestIndexVector;
Vector<uint8_t> g_ui8LinkDestHeartbeatVector;
size_t nextDestIndexVector;
uint8_t g_ui8SendActivation = false;
unsigned long g_ui32SendActivationTime = 0;
uint8_t g_ui8SetPairReq = false;
extern uint_fast8_t g_ui8Buttons;

//*****************************************************************************
//
// IEEE address from RNP
//
//*****************************************************************************
uint8_t g_ieeeAddr[8];

//*****************************************************************************
//
// TivaWare controller that contains configuration information
//
//*****************************************************************************
static TivaWareController& g_tivaWare = TivaWareController::getInstance();

//*****************************************************************************
//
// Link States.
//
//*****************************************************************************
enum
{
  LINK_STATE_INIT,
  LINK_STATE_READY,
  LINK_STATE_PAIR,
  LINK_STATE_NDATA,
  LINK_STATE_UNPAIR,
  LINK_STATE_TEST,
  LINK_STATE_TEST_COMPLETED,
  LINK_STATE_OAD
};

//*****************************************************************************
//
// List of implemented device types.
//
//*****************************************************************************
const uint8_t pui8DevListController[RTI_MAX_NUM_DEV_TYPES] =
{
RTI_DEVICE_REMOTE_CONTROL,
RTI_DEVICE_RESERVED_INVALID,
RTI_DEVICE_RESERVED_INVALID };

const uint8_t pui8DevListTarget[RTI_MAX_NUM_DEV_TYPES] =
{
RTI_DEVICE_TELEVISION,
RTI_DEVICE_RESERVED_INVALID,
RTI_DEVICE_RESERVED_INVALID };

//*****************************************************************************
//
// List of implemented profiles.
//
//*****************************************************************************
const uint8_t pui8ProfileList[RTI_MAX_NUM_PROFILE_IDS] =
{ RTI_PROFILE_ZRC, 0, 0, 0, 0, 0, 0 };

//*****************************************************************************
//
// List of possible target types.
//
//*****************************************************************************
const unsigned char pucTargetListController[RTI_MAX_NUM_SUPPORTED_TGT_TYPES] =
{
RTI_DEVICE_REMOTE_CONTROL,
RTI_DEVICE_TELEVISION,
RTI_DEVICE_SET_TOP_BOX,
RTI_DEVICE_MEDIA_CENTER_PC,
RTI_DEVICE_GAME_CONSOLE,
RTI_DEVICE_MONITOR };

const unsigned char pucTargetListTarget[RTI_MAX_NUM_SUPPORTED_TGT_TYPES] =
{
RTI_DEVICE_TELEVISION,
RTI_DEVICE_VIDEO_PLAYER_RECORDER,
RTI_DEVICE_SET_TOP_BOX,
RTI_DEVICE_MEDIA_CENTER_PC,
RTI_DEVICE_GAME_CONSOLE,
RTI_DEVICE_MONITOR };

//*****************************************************************************
//
// String version of Vendor Name.
//
//*****************************************************************************
const unsigned char pui8VendorName[] = "TI-LPRF";

//*****************************************************************************
//
// Determine type of reset to issue based on user buttons presses.
// No buttons will cause a restore of all previous state values.
// Left button press will clear state information but not configuration.
// Right button will clear state information and configuration.
//
// Reset the RNP by toggling the RF modules external reset line.
//
//*****************************************************************************
void ZIDResetRNP(void)
{
  //
  // Assert reset to the RNP.
  //
  ROM_GPIOPinWrite(EM_RESET_GPIO_PORT, EM_RESET_GPIO_PIN, 0);

  //
  // Hold reset low for about 8 milliseconds to verify reset is detected
  //
  ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / (125 * 3));

  //
  //Release reset to the RNP
  //
  ROM_GPIOPinWrite(EM_RESET_GPIO_PORT, EM_RESET_GPIO_PIN, EM_RESET_GPIO_PIN);

  //
  // Delay to allow RNP to do its internal boot.
  //
  ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / (2 * 3));
}

//*****************************************************************************
//
// Based on desired restoration setting, configure the RNP parameters for this
// application.
//
//*****************************************************************************
void ZIDConfigParams(void)
{
  uint8_t pui8Value[MAX_AVAIL_DEVICE_TYPES];
  volatile uint8_t ui8Status;
  uint8_t ui8Tmp;

  g_ui8Sending = 0;
  UARTprintf("start config\n");

  pui8Value[0] = eRTI_CLEAR_STATE;

  //
  // Execute the desired startup control setting. Clears or restore based on
  // button status.
  //
  ui8Status = RTI_WriteItem(RTI_CP_ITEM_STARTUP_CTRL, 1, pui8Value);
  if (ui8Status != RTI_SUCCESS)
  {
    UARTprintf("Err init: wr startup ctrl: 0x%02X\n", ui8Status);
  }

  //
  // If we successfully read the startup control value and it is not set to
  // restore the previous state then we need to re-configure the RNP.
  // If we are set to RESTORE then we can skip this configuration section.
  //
  if ((ui8Status == RTI_SUCCESS) && (pui8Value[0] != eRTI_RESTORE_STATE))
  {
    if (LPRF::getInstance().isTarget())
      ui8Status = RTI_WriteItem(RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES,
      RTI_MAX_NUM_SUPPORTED_TGT_TYPES, pucTargetListTarget);
    else
      ui8Status = RTI_WriteItem(RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES,
      RTI_MAX_NUM_SUPPORTED_TGT_TYPES, pucTargetListController);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr supp tgt types\n");
    }

    //
    // Application capabilities is a bit field that the application must
    // configure. It defines number of devices and profiles that this
    // node will be presenting to the network.
    //
    // No User String pairing; 1 Device (Remote Control); 1 Profile (ZRC)
    ui8Tmp = RTI_BUILD_APP_CAPABILITIES(0, 1, 1);
    ui8Status = RTI_WriteItem( RTI_CP_ITEM_APPL_CAPABILITIES, 1, &ui8Tmp);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr appl cap\n");
    }

    //
    // Write the list of supported device types to the RNP
    //
    if (LPRF::getInstance().isTarget())
      ui8Status = RTI_WriteItem(RTI_CP_ITEM_APPL_DEV_TYPE_LIST, RTI_MAX_NUM_DEV_TYPES,
          pui8DevListTarget);
    else
      ui8Status = RTI_WriteItem(RTI_CP_ITEM_APPL_DEV_TYPE_LIST, RTI_MAX_NUM_DEV_TYPES,
          pui8DevListController);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr dev type list\n");
    }

    //
    // Write the list of supported profiles to the RNP.
    //
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_APPL_PROFILE_ID_LIST,
    RTI_MAX_NUM_PROFILE_IDS, pui8ProfileList);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr supp prof types\n");
    }

    //
    // Configure node capabilities.
    // Node capabilities is a bit field that controls security, power
    // and other node characteristics.
    //
    if (LPRF::getInstance().isTarget())
    {
      // (1 - Target, 1 - AC powered, 1 - Security capable, 0 - no channel normalizaton)
      ui8Tmp = RTI_BUILD_NODE_CAPABILITIES(1, 1, 1, 0);
    }
    else
    {
      // (0 - Controller, 1 - battery powered, 1 - security capable, 0 - no channel normalizaton)
      ui8Tmp = RTI_BUILD_NODE_CAPABILITIES(0, 1, 1, 0);
    }
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_NODE_CAPABILITIES, 1, &ui8Tmp);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr node caps\n");
    }

    //
    // Write the Vendor ID number to the RNP.
    //
    pui8Value[0] = (uint8_t) (RTI_VENDOR_TEXAS_INSTRUMENTS & 0xFF);
    pui8Value[1] = (uint8_t) ((RTI_VENDOR_TEXAS_INSTRUMENTS >> 8) & 0xFF);
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr vendor ID\n");
    }

    ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("Vendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
    }
    else
    {
      UARTprintf("Err reading vendor ID!\n");
    }

    //
    // Write the string version of vendor name to the RNP.
    //
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_VENDOR_NAME, sizeof(pui8VendorName), pui8VendorName);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr vendor name\n");
    }

    ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("aVendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
    }
    else
    {
      UARTprintf("Err reading vendor ID!\n");
    }

    //
    // Write the desired standby duty cycle to the RNP.
    //
    pui8Value[0] = (uint8_t) (1000 & 0xFF);
    pui8Value[1] = (uint8_t) (1000 >> 8);
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_STDBY_DEFAULT_DUTY_CYCLE, 2, pui8Value);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("Err init: wr stdby duty cycle\n");
    }
    ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("bVendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
    }
    else
    {
      UARTprintf("Err reading vendor ID!\n");
    }
    UARTprintf("All conf written\n");
  }

  UARTprintf("end config\n");
}

//*****************************************************************************
//
// RTI Confirm function. Confirms the receipt of an Init Request (RTI_InitReq).
//
// Called by RTI_AsynchMsgProcess which we have placed in the main application
// context.
//
//*****************************************************************************
void RTI_InitCnf(uint8_t ui8Status)
{
  uint8_t ui8MaxEntries, ui8Value;
  uint8_t pui8Value[MAX_AVAIL_DEVICE_TYPES];

  ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
  if (ui8Status == RTI_SUCCESS)
  {
    UARTprintf("1Vendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
  }
  else
  {
    UARTprintf("Err reading vendor ID!\n");
  }

  UARTprintf("init hit cnf\n");
  //
  // Verify return status.
  //
  if (ui8Status == RTI_SUCCESS)
  {
    //
    // Make sure startup control is now set back to RESTORE mode.
    //
    ui8Value = eRTI_RESTORE_STATE;
    ui8Status = RTI_WriteItem(RTI_CP_ITEM_STARTUP_CTRL, 1, &ui8Value);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("write startupctrl fail\n");
    }

    ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("2Vendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
    }
    else
    {
      UARTprintf("Err reading vendor ID!\n");
    }
    //
    // Determine the maximum number of pairing table entries.
    //
    ui8Status = RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES, 1,
        &ui8MaxEntries);
    if (ui8Status != RTI_SUCCESS)
    {
      UARTprintf("read max table entries fail\n");
    }

    //
    // Verify that read was successful.
    //
    if (ui8Status == RTI_SUCCESS)
    {
      //
      // Delay briefly so we don't overrun the RPI buffers.
      //
      ROM_SysCtlDelay(g_tivaWare.CLOCK.ui32SysClock / (100 * 3));
      //
      // Send a Disable Sleep Request.  This will wake up the RPI
      // and then start a pairing sequence.
      //
      RTI_DisableSleepReq();

      //
      // Set the link state to pairing in process
      //
      g_vui8LinkState = LINK_STATE_PAIR;
      g_ui32RGBLPRFBlinkCounter = g_ui32SysTickCount;
    }
  }
  else
  {
    UARTprintf("Err!  initcnf called w/o succes: 0x%02x\n", ui8Status);
  }

}

//*****************************************************************************
//
// RTI confirm function. Called by RTI_AsynchMsgProcess when a pairing
// request is confirmed.  Contains the status of the pairing attempt and
// if successful the destination index.
//
//*****************************************************************************
void RTI_PairCnf(uint8_t ui8Status, uint8_t ui8DestIndex, uint8_t ui8DevType)
{
  //
  // The maze side uses AllowPairReq, not InitPairReq, as it is the target in
  // this case.  As such, PairReq shouldn't be called, so paircnf does nada
  //

  if (LPRF::getInstance().isController())
  {
    //static uint8_t sui8Tx[8];
    //uint8_t ui8TXOptions = 0;
    //
    // Determine if the Pair attempt was successful.
    //
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("paired!\n");
      //
      // Save the destination index we got back from our successful pairing.
      // Write that value back to the RNP so it knows this is the current
      // link to be used.
      //
      g_ui8LinkDestIndex = ui8DestIndex;
      ui8Status = RTI_WriteItemEx(RTI_PROFILE_RTI,
      RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &g_ui8LinkDestIndex);

      //
      // Turn on the LED to show we paired and ready.
      //
      g_tivaWare.LED.colorSetRGB(0x0, 0x0, 0x4000);

      //sui8Tx[0] = 'h';

      //RTI_SendDataReq(g_ui8LinkDestIndex, RTI_PROFILE_ZRC,
      //RTI_VENDOR_TEXAS_INSTRUMENTS, ui8TXOptions, 1, sui8Tx);

      g_vui8LinkState = LINK_STATE_READY;

      // IEEE address
      RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_IEEE_ADDRESS, sizeof(g_ieeeAddr), g_ieeeAddr);
      UARTprintf("IPv6=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", g_ieeeAddr[7], g_ieeeAddr[6],
          g_ieeeAddr[5], g_ieeeAddr[4], g_ieeeAddr[3], g_ieeeAddr[2], g_ieeeAddr[1], g_ieeeAddr[0]);
      g_ui8ConfigureLPRF = 1;
      g_ui8ConnectedLPRF = 1;
    }
    else
    {
      UARTprintf("tried to pair, failed: 0x%02x\n", ui8Status);
      ++g_ui32MaxPairConfTries;
      if (g_ui32MaxPairConfTries < g_tivaWare.LPRF.maxPairingTries)
      {
        g_vui8LinkState = LINK_STATE_PAIR;
        UARTprintf("hitting PairReq again=%u!\n", g_ui32MaxPairConfTries);
        RTI_PairReq();
      }
      else
      {
        //
        // Turn off the LED to show pairing failed.
        //
        g_tivaWare.LED.colorSetRGB(0x0, 0x4000, 0x4000);
        g_vui8LinkState = LINK_STATE_UNPAIR;
      }
    }
  }
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess if pairing was aborted.
// Currently not expected to get this call so just set the link state back to
// ready.
//
//*****************************************************************************
void RTI_PairAbortCnf(uint8_t ui8Status)
{
  (void) ui8Status;

  //
  // Reset the link state.
  //
  if (LINK_STATE_PAIR == g_vui8LinkState)
  {
    g_vui8LinkState = LINK_STATE_READY;
  }
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when an allow pair
// request is recieved by the application processor.  This is not expected for
// this application.
//
//*****************************************************************************
void RTI_AllowPairCnf(uint8_t ui8Status, uint8_t ui8DestIndex, uint8_t ui8DevType)
{
  //
  // Do nothing. Controller does not trigger AllowPairReq() and hence is not
  // expecting this callback.
  //
  (void) ui8Status;

  if (LPRF::getInstance().isTarget())
  {
    //
    // For maze_receiver, we are the target, so our pairing callback is
    // AllowPairCnf, not PairCnf.
    //

    //
    // Determine if the Pair attempt was successful.
    //
    if (ui8Status == RTI_SUCCESS)
    {
      UARTprintf("paired!\n");
      //
      // Save the destination index we got back from our successful pairing.
      // Write that value back to the RNP so it knows this is the current
      // link to be used.
      //
      g_ui8LinkDestIndex = ui8DestIndex;
      ui8Status = RTI_WriteItemEx(RTI_PROFILE_RTI,
      RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &g_ui8LinkDestIndex);

      //
      // Turn on the LED to show we paired and ready.
      //
      g_tivaWare.LED.colorSetGreen(0x4000);
      ++g_ui32MaxAllowPairConfTries;
      UARTprintf("g_ui8LinkDestIndex=%d\n", g_ui8LinkDestIndex);
      g_ui8LinkDestIndexVector.push_back(g_ui8LinkDestIndex);
      g_ui8LinkDestHeartbeatVector.push_back(0);

      if (g_ui8LinkDestIndexVector.size() == g_tivaWare.LPRF.maxControllers)
      {
        UARTprintf("TARGET RX mode \n");
        g_ui8SendActivation = true;
      }
    }
    else
    {
      UARTprintf("tried to pair, failed: 0x%02x\n", ui8Status);
      //
      // Turn off the LED to show pairing failed.
      //
      g_tivaWare.LED.colorSetGreen(0x0);
    }

    if (g_ui32MaxAllowPairConfTries < g_tivaWare.LPRF.maxControllers)
    {
      UARTprintf("hitting AllowPairReq for the next device\n");
      RTI_AllowPairReq();
    }

    g_vui8LinkState = LINK_STATE_READY;
  }
}

//*****************************************************************************
//
// RTI Confirm function.  Called by RTI_AsyncMsgProcess when a unpair request
// is confirmed by the RNP. Currently not implemented.
//
//*****************************************************************************
void RTI_UnpairCnf(uint8_t ui8Status, uint8_t ui8DestIndex)
{
  //
  // unused arguments
  //
  (void) ui8Status;
  (void) ui8DestIndex;
}

//*****************************************************************************
//
// RTI indication function. Called by RTI_AsynchMsgProcess when the far side of
// the link is requesting to unpair. Currently not implemented.
//
//
//*****************************************************************************
void RTI_UnpairInd(uint8_t ui8DestIndex)
{

  //
  // unused arguments
  //
  (void) ui8DestIndex;
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when a data send is
// confirmed. It is now clear to queue the next data packet.
//
//*****************************************************************************
void RTI_SendDataCnf(uint8_t ui8Status)
{

  if (ui8Status != RTI_SUCCESS)
  {
    UARTprintf("senddatacnf error! 0x%02x\n", ui8Status);
  }

  //
  // Set the link state to ready.
  //
  if (g_vui8LinkState == LINK_STATE_NDATA)
  {
    g_vui8LinkState = LINK_STATE_READY;
  }

  //
  // Turn on the LED to show we are back to ready state.
  //
  g_tivaWare.LED.colorSetGreen(0x0);
  g_ui8Sending = 0;
  //UARTprintf("RTI_SendDataCnf: %d\n", ui8Status);
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when the RxEnable
// request has been processed by the RNP. Not implemented.
//
//*****************************************************************************
void RTI_RxEnableCnf(uint8_t ui8Status)
{
  tRemoTIMsg sMsg;
  //
  // RTI RX Enable call has completed, so prep an asynchronous message...
  //
  sMsg.ui8SubSystem = RPC_SYS_RCAF;
  sMsg.ui8CommandID = RTIS_CMD_ID_RTI_RX_ENABLE_CNF;
  sMsg.ui8Length = 1;
  sMsg.pui8Data[0] = ui8Status;

  //
  // ...and send the RX Enable confirm
  //
  NPI_SendAsynchData(&sMsg);
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when the enable sleep
// request has been proccessed by the RNP.
//
//*****************************************************************************
void RTI_EnableSleepCnf(uint8_t ui8Status)
{
  //
  // Do nothing
  //
  (void) ui8Status;
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when disable sleep
// request has been processed by the RNP. This is used during the init sequence
// as a trigger to start a pairing sequence if needed.
//
//*****************************************************************************
void RTI_DisableSleepCnf(uint8_t ui8Status)
{
  uint8_t pui8Value[MAX_AVAIL_DEVICE_TYPES];
  (void) ui8Status;

  ui8Status = RTI_ReadItem(RTI_CP_ITEM_VENDOR_ID, 2, pui8Value);
  if (ui8Status == RTI_SUCCESS)
  {
    UARTprintf("3Vendor ID: %d,%d\n", pui8Value[0], pui8Value[1]);
  }
  else
  {
    UARTprintf("Err reading vendor ID!\n");
  }

  //
  // RNP is now awake, if we don't have a pairing link then start the pair
  // process.
  //
  if (g_ui8LinkDestIndex == RTI_INVALID_PAIRING_REF)
  {
    // FixMe: use a pairing buttons
    /*if (LPRF::getInstance().isController())
     {
     UARTprintf("hitting PairReq\n");
     RTI_PairReq();
     }*/

    if (LPRF::getInstance().isTarget())
    {
      UARTprintf("hitting AllowPairReq\n");
      RTI_AllowPairReq();
    }
  }
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when the RNP would
// like to get the latest report data from the application processor.
//
//*****************************************************************************
void RTI_ReceiveDataInd(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
    uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data)
{

  if (LPRF::getInstance().isTarget())
  {
    LPRF_TARGET* target = LPRF::getInstance().getTarget();
    if (target != NULL)
    {
      target->receive(ui8SrcIndex, ui8ProfileId, ui16VendorID, ui8RXLinkQuality, ui8RXFlags,
          ui8Length, pui8Data);
      g_ui8SendActivation = true;
      //g_ui8SendRoundRobin = true;
    }
  }

  if (LPRF::getInstance().isController())
  {
    LPRF_CONTROLLER* controller = LPRF::getInstance().getController();
    if (controller != NULL)
    {
      controller->receive(ui8SrcIndex, ui8ProfileId, ui16VendorID, ui8RXLinkQuality, ui8RXFlags,
          ui8Length, pui8Data);
    }
    //RTI_RxEnableReq(RTI_RX_ENABLE_OFF);
  }
}

//*****************************************************************************
//
// RTI Confirm function. Called by RTI_AsynchMsgProcess when a standby request
// has been processed by the RNP.
//
//*****************************************************************************
void RTI_StandbyCnf(uint8_t ui8Status)
{

  (void) ui8Status;

}

//*****************************************************************************
//
// RTI Callback function. The lower level UART and NPI layers have verified
// received an asynchronous message. Set the flag to indicate it needs
// processed.
//
//*****************************************************************************
void RTI_AsynchMsgCallback(uint32_t ui32Data)
{
  (void) ui32Data;

//
// Set the flag to tell LPRF Main that we need to process a message.
//
  HWREGBITW(&g_ui32Events, LPRF_EVENT) = 1;

//UARTprintf("RTI_AsynchMsgCallback \n");
}

//*****************************************************************************
//
//
//*****************************************************************************
void RTI_AsynchMsgProcess(void)
{
  tRemoTIMsg sMsg;

//
// Get the msg from the UART low level driver
//
  RemoTIUARTGetMsg((uint8_t *) &sMsg, NPI_MAX_DATA_LEN);
  if ((sMsg.ui8SubSystem & 0x1F) == RPC_SYS_RCAF)
  {
    //UARTprintf("RTI_AsynchMsgProcess: %02X \n", sMsg.ui8CommandID);
    switch ((unsigned long) sMsg.ui8CommandID)
    {
      case RTIS_CMD_ID_RTI_INIT_CNF:
        RTI_InitCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_PAIR_CNF:
        RTI_PairCnf(sMsg.pui8Data[0], sMsg.pui8Data[1], sMsg.pui8Data[2]);
        break;

      case RTIS_CMD_ID_RTI_PAIR_ABORT_CNF:
        RTI_PairAbortCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_ALLOW_PAIR_CNF:
        RTI_AllowPairCnf(sMsg.pui8Data[0], sMsg.pui8Data[1], sMsg.pui8Data[2]);
        break;

      case RTIS_CMD_ID_RTI_SEND_DATA_CNF:
        RTI_SendDataCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_REC_DATA_IND:
        RTI_ReceiveDataInd(sMsg.pui8Data[0], sMsg.pui8Data[1],
            sMsg.pui8Data[2] | (sMsg.pui8Data[3] << 8), sMsg.pui8Data[4], sMsg.pui8Data[5],
            sMsg.pui8Data[6], &sMsg.pui8Data[7]);
        break;

      case RTIS_CMD_ID_RTI_STANDBY_CNF:
        RTI_StandbyCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_ENABLE_SLEEP_CNF:
        RTI_EnableSleepCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_DISABLE_SLEEP_CNF:
        RTI_DisableSleepCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_RX_ENABLE_CNF:
        RTI_RxEnableCnf(sMsg.pui8Data[0]);
        break;

      case RTIS_CMD_ID_RTI_UNPAIR_CNF:
        RTI_UnpairCnf(sMsg.pui8Data[0], sMsg.pui8Data[1]);
        break;

      case RTIS_CMD_ID_RTI_UNPAIR_IND:
        RTI_UnpairInd(sMsg.pui8Data[0]);
        break;

      default:
        // nothing can be done here!
        break;
    }
  }
}

extern "C"
{
//*****************************************************************************
//
// Hopefully this will never get called, as it means that we tried to send a
// message over the CC2533, but we never got the callback signalling that the
// data was sent.  Something went wrong, so hopefully we can just call the
// senddata function again, but we're probably hosed.
//
//*****************************************************************************
  void Timer4AIntHandler(void)
  {

//
// Clear the interrupt
//
    TimerIntClear(TIMER4_BASE, TIMER_TIMA_TIMEOUT);

//
// Print something to let us know via debug console that the watchdog
// timed out
//
    //if (LPRF::getInstance().isController())
    //  UARTprintf("Sam: watchdog for controller time out!\n");
    //if (LPRF::getInstance().isTarget())
    //  UARTprintf("Sam: watchdog for target     time out!\n");
    TimerDisable(TIMER4_BASE, TIMER_A);

//
// Clear the sending flag, which will make it so that the lprf module will
// try to send the next packet, which will hopefully (??) get the system
// back to a good state.
//
    g_ui8Sending = 0;
    g_ui8SendActivation = true;
    g_tivaWare.LED.colorSetRGB(0x4000, 0x0, 0x4000);
  }

}

//*****************************************************************************
//
// Initialize timer 4 to be used as a watchdog for the send message function.
// It will be enabled with a long timeout on each message send, and disabled
// when the callback for the send data function is hit.
//
//*****************************************************************************
void WatchdogInit(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);
  TimerConfigure(TIMER4_BASE, TIMER_CFG_ONE_SHOT);
  IntEnable(INT_TIMER4A);
  TimerIntEnable(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
}

//*****************************************************************************
//
//
//*****************************************************************************
void LPRF::init(void)
{
//
// Initialize watchdog timer for the controller
//
  WatchdogInit();

// FixMe: Sam pin muxing for other devices
// Enable the RemoTI UART pin muxing.
// UART init is done in the RemoTI/uart_drv functions
//
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
  ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
  ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

//
// Configure the CC2533 Reset pin
//
  ROM_SysCtlPeripheralEnable(EM_RESET_SYSCTL_PERIPH);
  ROM_GPIOPinTypeGPIOOutput(EM_RESET_GPIO_PORT, EM_RESET_GPIO_PIN);

  g_ui8LinkDestIndex = RTI_INVALID_PAIRING_REF;

  RemoTIUARTInit(UART1_BASE);
  IntEnable(INT_UART1);

  NPI_Init(RTI_AsynchMsgCallback);
  RTI_Init();

  ZIDResetRNP();
  ZIDConfigParams();
  RTI_InitReq();
}

//*****************************************************************************
//
//
//*****************************************************************************
void LPRF::update(void)
{
  // Button pairing
  if (LPRF::getInstance().isController() && (g_ui8LinkDestIndex == RTI_INVALID_PAIRING_REF)
      && !g_ui8SetPairReq)
  {
    switch (g_ui8Buttons & ALL_BUTTONS)
    {
      //
      // Right button is pressed at startup.
      //
      case RIGHT_BUTTON:
      {
        g_tivaWare.LED.colorSetRGB(0x0, 0x4000, 0x4000);
        g_ui8SetPairReq = true;
        UARTprintf("hitting PairReq\n");
        RTI_PairReq();
        break;
      }
    }
  }

  //
  // First determine if we need to process a asynchronous message, such as
  // a send data confirmation or pairing confirmation.
  //
  while (HWREGBITW(&g_ui32Events, LPRF_EVENT) != 0)
  {
    //
    // Processes the message and calls the appropriate RTI callback.
    //
    RTI_AsynchMsgProcess();

    //
    // Clear the event flag.
    //
    if (RemoTIUARTGetRxMsgCount() == 0)
    {
      HWREGBITW(&g_ui32Events, LPRF_EVENT) = 0;
    }
  }

  if (g_ui8ConfigureLPRF && LPRF::getInstance().isController()
      && (g_vui8LinkState == LINK_STATE_READY))
  {
    // Enable receiver for the controller
    if (isDuplex())
    {
      UARTprintf("Enable: RTI_RxEnableReq\n");
      RTI_RxEnableReq(0xFFFE);
    }
    g_ui8ConfigureLPRF = 0;
  }
}

void LPRF::send()
{
  if (g_vui8LinkState == LINK_STATE_PAIR)
  {
    //
    // Pairing in process so do a steady quick blink of the LED.
    //
    if (g_ui32SysTickCount > (g_ui32RGBLPRFBlinkCounter + 20))
    {
      //
      // 20 ticks have expired since we last toggled so turn off the
      // LED and reset the counter.
      //
      g_ui32RGBLPRFBlinkCounter = g_ui32SysTickCount;
      g_tivaWare.LED.colorSetGreen(0x0);
    }
    else if (g_ui32SysTickCount == (g_ui32RGBLPRFBlinkCounter + 10))
    {
      //
      // 10 ticks have expired since the last counter reset.  turn
      // on the green LED.
      //
      g_tivaWare.LED.colorSetGreen(0x4000);
    }
  }
  else if (g_vui8LinkState == LINK_STATE_READY)
  {
    //
    // We are connected, turn on the green LED, turn off the red.
    //
    g_tivaWare.LED.colorSetRGB(0x0, 0x4000, 0x0);
    //
    if (LPRF::getInstance().isController())
    {
      // FixMe:
      // Re-enable receiver for the controller
      if (isDuplex())
        RTI_RxEnableReq(0xFFFE);

      if (!g_ui8Sending)
      {
        LPRF_CONTROLLER* controller = LPRF::getInstance().getController();
        if (controller != NULL)
        {
          uint8_t ui8TXOptions = 0;
          controller->execute();
          if ((controller->size() > 0) && (controller->size() < (NPI_MAX_DATA_LEN - 6)))
          {
            g_ui8Sending = 1;
            RTI_SendDataReq(g_ui8LinkDestIndex, RTI_PROFILE_ZRC, RTI_VENDOR_TEXAS_INSTRUMENTS,
                ui8TXOptions, controller->size(), controller->data());
            controller->reset();
            TimerLoadSet(TIMER4_BASE, TIMER_A, /*1 s*/g_tivaWare.CLOCK.ui32SysClock);
            TimerEnable(TIMER4_BASE, TIMER_A);
          }
        }
      }
    }

    if (LPRF::getInstance().isTarget() && isDuplex())
    {
      if (!g_ui8SendActivation
          && (g_ui8LinkDestIndexVector.size() == g_tivaWare.LPRF.maxControllers)
          && ((millis() - g_ui32SendActivationTime) > 12000))
      {
        if (g_ui8LinkDestHeartbeatVector[nextDestIndexVector] < 10)
          g_ui8LinkDestHeartbeatVector[nextDestIndexVector]++;
        UARTprintf("idx: %d backoff: %d \n", nextDestIndexVector,
            g_ui8LinkDestHeartbeatVector[nextDestIndexVector]);

        // Something is wrong with a controller
        g_ui8SendActivation = true; // Force the activation
      }

      if (!g_ui8Sending && g_ui8SendActivation
          && (g_ui8LinkDestIndexVector.size() == g_tivaWare.LPRF.maxControllers))
      {
        LPRF_TARGET* target = LPRF::getInstance().getTarget();
        if (target != NULL)
        {
          uint8_t ui8TXOptions = 0;
          target->execute();
          if ((target->size() > 0) && (target->size() < (NPI_MAX_DATA_LEN - 6)))
          {
            //UARTprintf("target sending ....\n");
            g_ui8Sending = 1;
            g_ui8SendActivation = false;
            g_ui32SendActivationTime = millis();
            nextDestIndexVector = (nextDestIndexVector + 1) % g_ui8LinkDestIndexVector.size();

            const int startDest = nextDestIndexVector;
            bool sendToNextDevice = true;

            while (g_ui8LinkDestHeartbeatVector[nextDestIndexVector] >= 10)
            {
              nextDestIndexVector = (nextDestIndexVector + 1) % g_ui8LinkDestIndexVector.size();
              if (startDest == nextDestIndexVector)
              {
                sendToNextDevice = false;
                UARTprintf("backoff from all devices\n");
                break;
              }
            }

            if (sendToNextDevice)
            {
              RTI_SendDataReq(g_ui8LinkDestIndexVector[nextDestIndexVector], RTI_PROFILE_ZRC,
              RTI_VENDOR_TEXAS_INSTRUMENTS, ui8TXOptions, target->size(), target->data());
              target->reset();
              TimerLoadSet(TIMER4_BASE, TIMER_A, 15/*10s*/* g_tivaWare.CLOCK.ui32SysClock /*/ 10*/);
              TimerEnable(TIMER4_BASE, TIMER_A);
            } else
              g_ui8SendActivation = false;
          }
        }
      }
    }
  }
}

//*****************************************************************************
//
// Helper methods
//
//*****************************************************************************
uint8_t LPRF::isSending() const
{
  return g_ui8Sending;
}

void LPRF::thisNodeType(const LPRF::LPRF_TYPE& nodeType)
{
  this->nodeType = nodeType;
}

void LPRF::thisTarget(LPRF_TARGET* target)
{
  this->target = target;
}

void LPRF::thisController(LPRF_CONTROLLER* controller)
{
  this->controller = controller;
}

void LPRF::thisDuplex(const bool& duplex)
{
  this->duplex = duplex;
}

bool LPRF::isTarget() const
{
  return nodeType == LPRF_TARGET_TYPE;
}

bool LPRF::isController() const
{
  return nodeType == LPRF_CONTROLLER_TYPE;
}

bool LPRF::isDuplex() const
{
  return duplex;
}

LPRF_TARGET* LPRF::getTarget() const
{
  return target;
}

LPRF_CONTROLLER* LPRF::getController() const
{
  return controller;
}

const uint8_t* LPRF::getIEEEAddr() const
{
  return g_ieeeAddr;
}

bool LPRF::getConnected() const
{
  return g_ui8ConnectedLPRF;
}

///////////////////////////////////////////////////////////////////////////////
