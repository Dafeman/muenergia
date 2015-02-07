/*
 * TMP006Module.h
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#ifndef TMP006MODULE_H_
#define TMP006MODULE_H_

#include "framework/Template.h"
#include "framework/InterruptVectorRepresentation.h"
#include "TMP006Representation.h"

MODULE(TMP006Module)
  REQUIRES(InterruptVectorRepresentation) //
  PROVIDES(TMP006Representation) //
END_MODULE
class TMP006Module: public TMP006ModuleBase
{
  public:
    void init();
    void update(TMP006Representation& theTMP006Representation);
};

#endif /* TMP006MODULE_H_ */
