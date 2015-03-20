/*
 * LPRFSensorsModule.h
 *
 *  Created on: Mar 1, 2015
 *      Author: sam
 */

#ifndef LPRFSENSORSMODULE_H_
#define LPRFSENSORSMODULE_H_

#include "framework/Template.h"
#include "framework/InterruptVectorRepresentation.h"
//
#include "LPRFRepresentation.h"
#include "BMP180Representation.h"
#include "ISL29023Representation.h"
#include "SHT21Representation.h"
#include "TMP006Representation.h"
#include "SensorAccessRepresentation.h"

class SensorCommon
{
  protected:
    /*NPI_MAX_DATA_LEN - 6 (but we have to use something smaller)*/
    enum
    {
      LPRF_TX_MAX_DATA_LEN = 250
    };

    unsigned char buffer[LPRF_TX_MAX_DATA_LEN];
    char encodedStr[LPRF_TX_MAX_DATA_LEN];

    int buffLocation;
    int receiveLocation;
    int sendLocation;
    const int syncPktLength;

    /*To identify the time series*/
    LPRFMetaRepresentation repMeta;
    LPRFSyncRepresentation repSync;

    /*data offset in bytes*/
    enum
    {
      LPRF_DATA_OFFSET_BYTES = 4
    };

  public:
    SensorCommon();

};

class SensorController: public SensorCommon, public LPRF_CONTROLLER
{
  private:
    /*This is the main data we send to the devices*/
    const BMP180Representation* theBMP180Representation;
    const ISL29023Representation* theISL29023Representation;
    const SHT21Representation* theSHT21Representation;
    const TMP006Representation* theTMP006Representation;
  public:
    bool contentMsg;

  public:
    SensorController(const BMP180Representation* theBMP180Representation,
        const ISL29023Representation* theISL29023Representation,
        const SHT21Representation* theSHT21Representation,
        const TMP006Representation* theTMP006Representation);

    uint8_t size() const;
    uint8_t* data() const;
    void execute();
    void reset();
    void receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
        uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data);
};

class SensorTarget: public SensorCommon, public LPRF_TARGET
{
  private:
    const InterruptVectorRepresentation* theInterruptVectorRepresentation;
    BMP180Representation repBMP180;
    ISL29023Representation repISL29023;
    SHT21Representation repSHT21;
    TMP006Representation repTMP006;
    int32_t i32IntegerPart, i32FractionPart;

  public:
    SensorTarget(const InterruptVectorRepresentation* theInterruptVectorRepresentation);
    uint8_t size() const;
    uint8_t* data() const;
    void execute();
    void reset();
    void receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
        uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data);

  private:
    void UARTprintff(const float& value);

};

MODULE(LPRFSensorControllerModule)
  REQUIRES(BMP180Representation) //
  REQUIRES(ISL29023Representation) //
  REQUIRES(SHT21Representation) //
  REQUIRES(TMP006Representation) //
  REQUIRES(InterruptVectorRepresentation) //
  PROVIDES(SensorAccessRepresentation)
END_MODULE
class LPRFSensorControllerModule: public LPRFSensorControllerModuleBase
{
  private:
    SensorController* controller; //

  public:
    LPRFSensorControllerModule();
    ~LPRFSensorControllerModule();
    void init();
    void update(SensorAccessRepresentation& theSensorAccessRepresentation);
};

MODULE(LPRFSensorTargetModule)
  REQUIRES(InterruptVectorRepresentation) //
END_MODULE
class LPRFSensorTargetModule: public LPRFSensorTargetModuleBase
{
  private:
    SensorTarget* target; //

  public:
    LPRFSensorTargetModule();
    ~LPRFSensorTargetModule();
    void init();
    void execute();
};

#endif /* LPRFSENSORSMODULE_H_ */
