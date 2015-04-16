/*
 * IEEEMachine.h
 *
 *  Created on: Apr 10, 2015
 *      Author: sam
 */

#ifndef IEEEMACHINE_H_
#define IEEEMACHINE_H_

#include <iostream>
#include <vector>
#include <unordered_map>

class IEEEMachine
{
  public:
    void process(const std::string& inputName, const std::string& outputDir);
    void process(std::unordered_map<std::string, std::string>& in);

  private:
    void validateOutputDir(const std::string& outputDir) const;
    bool validateDouble(std::string const& s) const;
    void populateDeviceNames(std::vector<std::string>& deviceNames);
    void populateDeviceOutNames(const std::string& outputDir, std::vector<std::string>& deviceNames,
        std::vector<std::string>& deviceOutNames) const;
    void populateOutstreams(std::vector<std::string>& deviceNames,
        std::vector<std::string>& deviceOutNames,
        std::unordered_map<std::string, std::ofstream*>& outstreams) const;
    void removeOutputStreams(std::unordered_map<std::string, std::ofstream*>& outstreams) const;
};

#endif /* IEEEMACHINE_H_ */
