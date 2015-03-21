/*
 * SensorAccessRepresentation.h
 *
 *  Created on: Mar 20, 2015
 *      Author: sam
 */

#ifndef SENSORACCESSREPRESENTATION_H_
#define SENSORACCESSREPRESENTATION_H_

#include "framework/Template.h"

REPRESENTATION(SensorAccessRepresentation)
class SensorAccessRepresentation : public SensorAccessRepresentationBase
{
  public:
    bool active;
    SensorAccessRepresentation() : active(false) {}
};

#endif /* SENSORACCESSREPRESENTATION_H_ */

