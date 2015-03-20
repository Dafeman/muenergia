/*
 * LPRF.h
 *
 *  Created on: Sep 22, 2014
 *      Author: sam
 */

#ifndef LPRF_H_
#define LPRF_H_

#include <stdint.h>

class LPRF_TX_RX_BASE
{
  public:
    virtual ~LPRF_TX_RX_BASE() {}
    virtual uint8_t size() const =0;
    virtual uint8_t* data() const =0;
    virtual void execute() =0;/*sometimes pre-processing is needed*/
    virtual void reset() =0;/*sometimes post-processing is needed*/
    virtual void receive(uint8_t ui8SrcIndex, uint8_t ui8ProfileId, uint16_t ui16VendorID,
        uint8_t ui8RXLinkQuality, uint8_t ui8RXFlags, uint8_t ui8Length, uint8_t *pui8Data) =0;
};

class LPRF_TARGET : public LPRF_TX_RX_BASE
{
};

class LPRF_CONTROLLER : public LPRF_TX_RX_BASE
{
};

class LPRF
{
  public:
    enum LPRF_TYPE
    {
      LPRF_TARGET_TYPE, //
      LPRF_CONTROLLER_TYPE
    };

  protected:
    LPRF();
    ~LPRF();
    LPRF(LPRF const&);
    LPRF& operator=(LPRF const&);
    static LPRF* theInstance;
    LPRF_TYPE nodeType;
    LPRF_TARGET* target;
    LPRF_CONTROLLER* controller;
    bool duplex;

  public:
    static LPRF& getInstance();
    static void deleteInstance();
    void init();
    void update();
    void send();

    uint8_t isSending() const;
    void thisNodeType(const LPRF_TYPE& nodeType);
    void thisTarget(LPRF_TARGET* target);
    void thisController(LPRF_CONTROLLER* controller);
    void thisDuplex(const bool& duplex);
    bool isTarget() const;
    bool isController() const;
    bool isDuplex() const;
    LPRF_TARGET* getTarget() const;
    LPRF_CONTROLLER* getController() const;
    const uint8_t* getIEEEAddr() const;
    bool getConnected() const;
};

#endif /* LPRF_H_ */
