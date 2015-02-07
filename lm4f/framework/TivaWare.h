/*
 * TivaWare.h
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#ifndef TIVAWARE_H_
#define TIVAWARE_H_

#if defined(ENERGIA)
#define EMBEDDED_MODE
#endif

#if defined(EMBEDDED_MODE)
//
#include <stdarg.h>
//
#include "Energia.h"
//
#include "sensorlib/i2cm_drv.h"
#include "inc/hw_ints.h"
#include "utils/uartstdio.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "drivers/pinout.h"
#include "lprf/LPRF.h"

class I2CMWrapper
{
  public:
    bool initializedI2CM;
    tI2CMInstance instance;

    I2CMWrapper() :
        initializedI2CM(false)
    {
    }

    void initI2CM()
    {
      initializedI2CM = true;
    }

    bool initialized() const
    {
      return initializedI2CM;
    }

  protected:
    I2CMWrapper(I2CMWrapper const&);
    I2CMWrapper& operator=(I2CMWrapper const&);

};

class LEDWrapper
{
  private:
    uint32_t colors[3];

  public:
    LEDWrapper()
    {
      for (uint32_t i = 0; i < 3; i++)
        colors[i] = 0x0;
    }

    void initRGB(uint32_t ui32Enable)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBInit(ui32Enable);
#endif
    }

    void enableRGB()
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBEnable();
#endif
    }

    void disableRGB()
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBDisable();
#endif
    }

    void setRGB(uint32_t red, uint32_t green, uint32_t blue, float fIntensity)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      colors[RED] = red;
      colors[GREEN] = green;
      colors[BLUE] = blue;
      RGBSet(colors, fIntensity);
#endif
    }
    void colorSetRGB(uint32_t red, uint32_t green, uint32_t blue)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      colors[RED] = red;
      colors[GREEN] = green;
      colors[BLUE] = blue;
      RGBColorSet(colors);
#endif
    }

    void colorSetRed(uint32_t red)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      colors[RED] = red;
      RGBColorSet(colors);
#endif
    }

    void colorSetGreen(uint32_t green)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      colors[GREEN] = green;
      RGBColorSet(colors);
#endif
    }

    void colorSetBlue(uint32_t blue)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      colors[BLUE] = blue;
      RGBColorSet(colors);
#endif
    }

    void intensitySetRGB(float fIntensity)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBIntensitySet(fIntensity);
#endif
    }

    void blinkRateSetRGB(float fRate)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBBlinkRateSet(fRate);
#endif
    }

    uint32_t* getRGB(float * pfIntensity)
    {
#ifdef TARGET_IS_BLIZZARD_RB1
      RGBGet(colors, pfIntensity);
#endif
      return colors;
    }

  protected:
    LEDWrapper(LEDWrapper const&);
    LEDWrapper& operator=(LEDWrapper const&);

};

class CLOCKWrapper
{
  public:
    uint32_t ui32SrcClock;
    uint32_t ui32SysClock;
    uint32_t ui32SysTickHz;
    uint32_t ui32SysTickMs;

    CLOCKWrapper() :
        ui32SrcClock(0), ui32SysClock(0), ui32SysTickHz(100), ui32SysTickMs(1000 / ui32SysTickHz)
    {
    }

  protected:
    CLOCKWrapper(CLOCKWrapper const&);
    CLOCKWrapper& operator=(CLOCKWrapper const&);
};

class UARTWrapper
{
  private:
    uint32_t ui32SrcClock;
    uint32_t ui32SysClock;

  public:
    UARTWrapper() :
        ui32SrcClock(0), ui32SysClock(0)
    {
    }

    void initUART(uint32_t ui32Port, uint32_t ui32Baud, uint32_t ui32SrcClock,
        uint32_t ui32SysClock)
    {
      this->ui32SrcClock = ui32SrcClock;
      this->ui32SysClock = ui32SysClock;
      UARTStdioConfig(ui32Port, ui32Baud, ui32SrcClock);
    }

    int gets(char *pcBuf, uint32_t ui32Len)
    {
      return UARTgets(pcBuf, ui32Len);
    }

    void printf(const char *pcString, ...)
    {
      va_list vaArgP;

      //
      // Start the varargs processing.
      //
      va_start(vaArgP, pcString);

      UARTvprintf(pcString, vaArgP);

      //
      // We're finished with the varargs now.
      //
      va_end(vaArgP);

// Slow down by 2 ms to send data as UART is much slower
// than ARM
      ROM_SysCtlDelay(ui32SysClock / (500 * 3));
    }

    int write(const char *pcBuf, uint32_t ui32Len)
    {
      return UARTwrite(pcBuf, ui32Len);
    }

    /**
     * I recommend this way of writing a string
     */
    int write(const String& str)
    {
      return UARTwrite(str.c_str(), str.length());
    }

  protected:
    UARTWrapper(UARTWrapper const&);
    UARTWrapper& operator=(UARTWrapper const&);

};

class ButtonWrapper
{
  public:
    uint_fast8_t buttons;

    ButtonWrapper() :
        buttons(0)
    {
    }

    void initButtons()
    {
      ButtonsInit();
    }

    uint8_t pollButtons(uint8_t* pui8Delta = 0, uint8_t* pui8RawState = 0)
    {
      buttons = ButtonsPoll(pui8Delta, pui8RawState);
      return buttons;
    }

  protected:
    ButtonWrapper(ButtonWrapper const&);
    ButtonWrapper& operator=(ButtonWrapper const&);
};

class LPRFWrapper
{
  public:
    bool active; //
    bool isTarget;
    uint32_t maxControllers;
    uint32_t maxPairingTries;
    LPRF& theLPRF;

    LPRFWrapper() :
        active(false), isTarget(false), maxControllers(1/*fixMe*/), maxPairingTries(5), theLPRF(
            LPRF::getInstance())
    {
    }

    void initLPRF()
    {
      if (isTarget)
        theLPRF.thisNodeType(LPRF::LPRF_TARGET_TYPE);
      else
        theLPRF.thisNodeType(LPRF::LPRF_CONTROLLER_TYPE);
      theLPRF.init();
    }

    void updateLPRF()
    {
      theLPRF.update();
    }

    void sendLPRF()
    {
      theLPRF.send();
    }

    void targetLPRF(LPRF_TARGET* target)
    {
      theLPRF.thisTarget(target);
    }

    void controllerLPRF(LPRF_CONTROLLER* controller)
    {
      theLPRF.thisController(controller);
    }

    uint8_t isSending() const
    {
      return theLPRF.isSending();
    }

  protected:
    LPRFWrapper(LPRFWrapper const&);
    LPRFWrapper& operator=(LPRFWrapper const&);
};

class BOOSTERPACKWrapper
{
  public:
    uint32_t ID;
    BOOSTERPACKWrapper() :
        ID(1)
    {
    }
  protected:
    BOOSTERPACKWrapper(BOOSTERPACKWrapper const&);
    BOOSTERPACKWrapper& operator=(BOOSTERPACKWrapper const&);
};

class TivaWareController
{
  protected:
    TivaWareController();
    ~TivaWareController();
    TivaWareController(TivaWareController const&);
    TivaWareController& operator=(TivaWareController const&);
    static TivaWareController* theInstance;

  public:
    static TivaWareController& getInstance();
    static void deleteInstance();

    I2CMWrapper I2C;
    LEDWrapper LED;
    CLOCKWrapper CLOCK;
    UARTWrapper UART;
    ButtonWrapper BUTTONS;
    LPRFWrapper LPRF;
    BOOSTERPACKWrapper BOOSTERPACK;
};

#endif

#endif /* TIVAWARE_H_ */
