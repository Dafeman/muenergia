/*
 * BQ27510G3Module.h
 *
 *  Created on: Sep 15, 2014
 *      Author: sam
 */

#ifndef BQ27510G3MODULE_H_
#define BQ27510G3MODULE_H_

#include "framework/Template.h"
#include "SensorAccessRepresentation.h"

MODULE(BQ27510G3Module)
  USES(SensorAccessRepresentation) //
END_MODULE
class BQ27510G3Module: public BQ27510G3ModuleBase
{
  public:
    void init();
    void execute();
};

#endif /* BQ27510G3MODULE_H_ */
