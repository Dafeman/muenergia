/*
 * SHT21Module.h
 *
 *  Created on: Sep 12, 2014
 *      Author: sam
 */

#include "framework/Template.h"
#include "SHT21Representation.h"
#include "SensorAccessRepresentation.h"

MODULE(SHT21Module)
  PROVIDES(SHT21Representation) //
  USES(SensorAccessRepresentation)
END_MODULE
class SHT21Module: public SHT21ModuleBase
{
  public:
    void init();
    void update(SHT21Representation& theSHT21Representation);
};


