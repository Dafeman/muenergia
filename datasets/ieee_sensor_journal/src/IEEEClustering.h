/*
 * IEEEClustering.h
 *
 *  Created on: Apr 14, 2015
 *      Author: sam
 */

#ifndef IEEECLUSTERING_H_
#define IEEECLUSTERING_H_

#include <iostream>
#include <unordered_map>

class IEEEClustering
{
  private:
    int windowLength;
    int halfWindowLength;

  public:
    IEEEClustering(const int& windowLength);
    void process(const std::string& inFile, const std::string outFile);
    void process(const std::unordered_map<std::string,std::string>& dataDescriptors);
};

#endif /* IEEECLUSTERING_H_ */
