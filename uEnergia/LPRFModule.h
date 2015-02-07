/*
 * LPRFModule.h
 *
 *  Created on: Sep 13, 2014
 *      Author: sam
 */

#ifndef LPRFMODULE_H_
#define LPRFMODULE_H_

#include "framework/Template.h"
#include "framework/InterruptVectorRepresentation.h"
#include "MPU9150Representation.h"

REPRESENTATION(LPRFMetaRepresentation)
class LPRFMetaRepresentation: public LPRFMetaRepresentationBase
{
  public:
    int ID;
    int pktLost;

    LPRFMetaRepresentation() :
        ID(0), pktLost(0)
    {
    }
    void serialize(ObjectInput* in, ObjectOutput* out)
    {
      SERIALIZE(ID);
      SERIALIZE(pktLost);
    }
};

REPRESENTATION(LPRFSyncRepresentation)
class LPRFSyncRepresentation: public LPRFSyncRepresentationBase
{
  public:
    unsigned long ms; //
    bool sendActivation;
    bool resetActivation;
    LPRFSyncRepresentation() :
        ms(0), sendActivation(false), resetActivation(false)
    {
    }
    void serialize(ObjectInput* in, ObjectOutput* out)
    {
      SERIALIZE(ms);
      SERIALIZE(sendActivation)
      SERIALIZE(resetActivation);
    }
};

class TX_RX_COMMON
{
  protected:
    /*NPI_MAX_DATA_LEN - 6 (but we have to use something smaller)*/
    enum
    {
      LPRF_TX_MAX_DATA_LEN = 250
    };

    unsigned char buffer[LPRF_TX_MAX_DATA_LEN];
    char encodedStr[LPRF_TX_MAX_DATA_LEN];

    int sendLocation;
    int receiveLocation;
    int bufferLocation;

    const int dataPktLength;
    const int metaPktLength;
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
    TX_RX_COMMON();

};

class CONTROLLER: public TX_RX_COMMON, public LPRF_CONTROLLER
{
  private:
    enum
    {
      LPRF_APP_MAX_BUFFER_LENGTH = 8010 // segmentLength * 445
    };
    unsigned char appBuffer[LPRF_APP_MAX_BUFFER_LENGTH];
    int appBufferLocation;
    int accumulatedPktSize;
    int toSendSize;

    const int maxEnsembleSize; /*Max payload size without radio overflow*/
    const int dataPktEnsembleLength;

  public:
    CONTROLLER();
    uint8_t size() const;
    uint8_t* data() const;
    void execute();
    void reset();
    void receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
        uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data);

    void serialize(const MPU9150Representation* inRep);
    void clear();
};

class TARGET: public TX_RX_COMMON, public LPRF_TARGET
{
  private:
    /*This is the main data we send to the devices*/
    const MPU9150Representation* theMPU9150Representation;
    const InterruptVectorRepresentation* theInterruptVectorRepresentation;
    MPU9150Representation repDATA;
    int_fast32_t i32IPart[16], i32FPart[16];

  public:
    TARGET();
    uint8_t size() const;
    uint8_t* data() const;
    void execute();
    void reset();
    void receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
        uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data);
    void setMPU9150Representation(const MPU9150Representation* theMPU9150Representation);
    void setInterruptVectorRepresentation(
        const InterruptVectorRepresentation* theInterruptVectorRepresentation);

};

MODULE(LPRFModule)
  REQUIRES(InterruptVectorRepresentation) //
  REQUIRES(MPU9150Representation) //
END_MODULE
class LPRFModule: public LPRFModuleBase
{
  private:
    TARGET target; //
    CONTROLLER controller; //
    bool sendLoop;

  public:
    LPRFModule();
    void init();
    void execute();
};

#endif /* LPRFMODULE_H_ */
