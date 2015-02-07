/*
 * BQ27510G3Module.h
 *
 *  Created on: Sep 15, 2014
 *      Author: sam
 */

#ifndef BQ27510G3MODULE_H_
#define BQ27510G3MODULE_H_

#include "framework/Template.h"

MODULE(BQ27510G3Module)
END_MODULE
class BQ27510G3Module: public BQ27510G3ModuleBase
{
  public:
    void init();
    void execute();
};

#endif /* BQ27510G3MODULE_H_ */
