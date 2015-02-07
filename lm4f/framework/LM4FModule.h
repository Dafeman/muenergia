/*
 * InterruptModule.h
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#ifndef LM4FMODULE_H_
#define LM4FMODULE_H_

#include "Template.h"
#include "InterruptVectorRepresentation.h"

MODULE(LM4FModule)
  PROVIDES(InterruptVectorRepresentation) //
END_MODULE
class LM4FModule: public LM4FModuleBase
{
  public:
    void init();
    void update(InterruptVectorRepresentation& theInterruptVectorRepresentation);
};

#endif /* LM4FMODULE_H_ */
