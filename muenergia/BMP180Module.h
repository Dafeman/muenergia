/*
 * BMP180Module.h
 *
 *  Created on: Sep 12, 2014
 *      Author: sam
 */

#ifndef BMP180MODULE_H_
#define BMP180MODULE_H_

#include "framework/Template.h"
#include "framework/InterruptVectorRepresentation.h"
#include "BMP180Representation.h"

MODULE(BMP180Module)
  REQUIRES(InterruptVectorRepresentation) //
  PROVIDES(BMP180Representation) //
END_MODULE
class BMP180Module: public BMP180ModuleBase
{
  public:
    void init();
    void update(BMP180Representation& theBMP180Representation);
};

#endif /* BMP180MODULE_H_ */
