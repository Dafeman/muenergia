/*
 * Test1Module.h
 *
 *  Created on: Sep 9, 2014
 *      Author: sam
 */

#ifndef TEST1MODULE_H_
#define TEST1MODULE_H_

#include "framework/Template.h"
//
#include "Test1Representation.h"

MODULE(Test1Module)
  PROVIDES(Test1Representation) //
END_MODULE
class Test1Module: public Test1ModuleBase
{
  public:
    void init();
    void execute();
    void update(Test1Representation& theTest1Representation);
};

#endif /* TEST1MODULE_H_ */
