/*
 * LPRFRepresentation.h
 *
 *  Created on: Mar 1, 2015
 *      Author: sam
 */

#ifndef LPRFREPRESENTATION_H_
#define LPRFREPRESENTATION_H_

#include "framework/Template.h"

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
    bool sendActivation;bool resetActivation;
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

#endif /* LPRFREPRESENTATION_H_ */
