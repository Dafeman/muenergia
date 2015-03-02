/*
 * TMP006Representation.h
 *
 *  Created on: Feb 21, 2014
 *      Author: Saminda
 */

#ifndef TMP006REPRESENTATION_H_
#define TMP006REPRESENTATION_H_

#include "framework/Template.h"

REPRESENTATION(TMP006Representation)
class TMP006Representation: public TMP006RepresentationBase
{
  public:
    float fAmbient;
    float fObject;
    TMP006Representation() :
        fAmbient(0), fObject(0)
    {
    }

    void serialize(ObjectInput* in, ObjectOutput* out)
    {
      SERIALIZE(fAmbient);
      SERIALIZE(fObject);
    }
};

#endif /* TMP006REPRESENTATION_H_ */
