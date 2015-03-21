/*
 * MPU9150Module.h
 *
 *  Created on: Sep 11, 2014
 *      Author: sam
 */

#ifndef MPU9150MODULE_H_
#define MPU9150MODULE_H_

#include "framework/Template.h"
#include "framework/InterruptVectorRepresentation.h"
#include "MPU9150Representation.h"
#include "SensorAccessRepresentation.h"

MODULE(MPU9150Module)
  REQUIRES(InterruptVectorRepresentation) //
  PROVIDES(MPU9150Representation) //
  USES(SensorAccessRepresentation) //
END_MODULE
class MPU9150Module: public MPU9150ModuleBase
{
  private:
    uint_fast32_t ui32CompDCMStarted;
  public:
    void init();
    void update(MPU9150Representation& theMPU9150Representation);
};

#endif /* MPU9150MODULE_H_ */

