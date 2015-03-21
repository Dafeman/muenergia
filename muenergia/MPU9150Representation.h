/*
 * MPU9150Representation.h
 *
 *  Created on: Feb 21, 2014
 *      Author: Saminda
 */

#ifndef MPU9150REPRESENTATION_H_
#define MPU9150REPRESENTATION_H_

#include "framework/Template.h"

REPRESENTATION(MPU9150Representation)
class MPU9150Representation: public MPU9150RepresentationBase
{
  public:
    // Raw values
    unsigned short pui16Accel[3];
    unsigned short pui16Gyro[3];
    unsigned short pui16Mag[3];
    // Calculated values
    float fAccel[3]; // (x.., y.., z..) m/s/s
    float fGyro[3]; // (x.., y.., z..) rad/s/s
    float fMag[3]; // ?
    float fEulers[3]; // (x (roll), y (pitch), z (yaw)) rad (see the axes on the booster pack)
    float fQuaternion[4]; // (w, x, y, z)
    // Current state
    bool bUpdated;
    // Calibration factors
    float fAccelFactor;
    float fGyroFactor;
    MPU9150Representation() :
        bUpdated(false), fAccelFactor(0), fGyroFactor(0)
    {
    }

    void serialize(ObjectInput* in, ObjectOutput* out)
    {
      // We only need these raw values
      SERIALIZE_BUFFER("pui16Accel", (unsigned char* )&pui16Accel[0], 3 * sizeof(unsigned short));
      SERIALIZE_BUFFER("pui16Gyro", (unsigned char* )&pui16Gyro[0], 3 * sizeof(unsigned short));
      SERIALIZE_BUFFER("pui16Mag", (unsigned char* )&pui16Mag[0], 3 * sizeof(unsigned short));
    }
};

#endif /* MPU9150REPRESENTATION_H_ */

