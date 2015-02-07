/*
 * InterruptVectorRepresentation.h
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#ifndef INTERRUPTREPRESENTATION_H_
#define INTERRUPTREPRESENTATION_H_

#include "Template.h"

REPRESENTATION(InterruptVectorRepresentation)
class InterruptVectorRepresentation: public InterruptVectorRepresentationBase
{
  public:
    // PORT B
    bool interruptedMPU9150;
    // PORT E
    bool interruptedTMP006;
    //
    bool interruptedISL29023;
    // SysTick
    bool interruptedSysTick;
    // Timer 3
    bool interruptedTimer3;
    // Buttons
    uint_fast8_t buttons;
    InterruptVectorRepresentation() :
        interruptedMPU9150(false), interruptedTMP006(false), interruptedISL29023(false), interruptedSysTick(
        false), interruptedTimer3(false), buttons(0)
    {
    }
};

#endif /* INTERRUPTREPRESENTATION_H_ */
