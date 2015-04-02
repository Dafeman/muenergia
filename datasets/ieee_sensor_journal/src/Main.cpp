//============================================================================
// Name        : Main.cpp
// Author      : Sam Abeyruwan
// Version     :
// Copyright   : Your copyright notice
// Description : IEEE journal paper sesnor pre-processing
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iterator>
//
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Process data
 */
class IEEEMachine
{
  public:
    IEEEMachine()
    {
    }

    void process(const std::string& inputName, const std::string& outputDir)
    {
      std::cout << "inputName: " << inputName << " outputDir: " << outputDir << std::endl;

      std::ifstream in(inputName.c_str());

      validateOutputDir(outputDir);

      std::vector<std::string> deviceNames;
      populateDeviceNames(deviceNames);

      std::vector<std::string> deviceOutNames;
      populateDeviceOutNames(outputDir, deviceNames, deviceOutNames);

      std::unordered_map<std::string, std::ofstream*> outstreams;
      populateOutstreams(deviceNames, deviceOutNames, outstreams);

      if (in.is_open())
      {
        std::string line;
        std::vector<std::string> tokens;
        int lineNumber = 0;
        while (std::getline(in, line))
        {
          tokens.clear();
          std::istringstream iss(line);
          std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
              std::back_inserter(tokens));

          std::unordered_map<std::string, std::ofstream*>::iterator iter = outstreams.find(
              *tokens.begin());
          if (iter != outstreams.end())
          {
            if ((tokens.size() > 10) || (tokens.size() < 9))
            {
              std::cerr << "Line: " << lineNumber << " str: " << line << std::endl;
              continue;
            }

            bool validTokens = true;
            for (size_t i = 1; i < 8; i++)
            {
              if (!validateDouble(tokens[i]))
              {
                validTokens = false;
                std::cout << "Skip: " << lineNumber << " checkForDouble: " << tokens[i]
                    << std::endl;
                break;
              }
            }

            if (validTokens)
            {
              for (size_t i = 1; i < 8; i++)
                (*iter->second) << tokens[i] << " ";
              (*iter->second) << std::endl;
              iter->second->flush();
            }
          }

          ++lineNumber;
        }
      }
      else
      {
        std::cerr << "Error: file: " << inputName << " missing" << std::endl;
      }

      removeOutputStreams(outstreams);

    }

    void process(std::unordered_map<std::string, std::string>& in)
    {
      for (std::unordered_map<std::string, std::string>::const_iterator iter = in.begin();
          iter != in.end(); ++iter)
        process(iter->first, iter->second);
    }

  private:

    void validateOutputDir(const std::string& outputDir) const
    {
      struct stat st =
      { 0 };
      if (stat(outputDir.c_str(), &st) == -1)
      {
        mkdir(outputDir.c_str(), 0700);
        std::cout << "dir: " << outputDir << " has been created." << std::endl;
      }
      else
        std::cout << "dir: " << outputDir << " exists." << std::endl;
    }

    bool validateDouble(std::string const& s) const
    {
      std::istringstream ss(s);
      double d;
      return (ss >> d) && (ss >> std::ws).eof();
    }

    void populateDeviceNames(std::vector<std::string>& deviceNames)
    {
      deviceNames.push_back("7ef");
      deviceNames.push_back("7cd");
      deviceNames.push_back("8d4");
    }

    void populateDeviceOutNames(const std::string& outputDir, std::vector<std::string>& deviceNames,
        std::vector<std::string>& deviceOutNames) const
    {
      for (std::vector<std::string>::const_iterator iter = deviceNames.begin();
          iter != deviceNames.end(); ++iter)
        deviceOutNames.push_back(outputDir + (*iter));
    }

    void populateOutstreams(std::vector<std::string>& deviceNames,
        std::vector<std::string>& deviceOutNames,
        std::unordered_map<std::string, std::ofstream*>& outstreams) const
    {
      for (std::vector<std::string>::size_type i = 0; i < deviceNames.size(); ++i)
        outstreams.emplace(deviceNames[i], new std::ofstream(deviceOutNames[i].c_str()));
    }

    void removeOutputStreams(std::unordered_map<std::string, std::ofstream*>& outstreams) const
    {
      for (std::unordered_map<std::string, std::ofstream*>::iterator iter = outstreams.begin();
          iter != outstreams.end(); ++iter)
      {
        iter->second->close();
        delete iter->second;
      }

      outstreams.clear();
    }
};

void ieeeMachineSingleProcess()
{
  const std::string inputName = "201541-18-8-24data-ub506-people22.txt";
  const std::string outputDir = "04_01_2015/";
  IEEEMachine ieeeMachine;
  ieeeMachine.process(inputName, outputDir);
}

void ieeeMachineMultipleProcess()
{
  std::unordered_map<std::string, std::string> in;
  std::string base = "floor-data-march-26/";

  in.emplace(base + "2015326-16-48-181st-floor-before-swap.txt", "03_26_2015_1f1h/");
  in.emplace(base + "2015326-16-57-511st-floor-after-swap.txt", "03_26_2015_1f2h/");

  in.emplace(base + "2015326-17-5-92nd-floor-before-swap.txt", "03_26_2015_2f1h/");
  in.emplace(base + "2015326-17-15-282nd-floor-after-swap.txt", "03_26_2015_2f2h/");

  in.emplace(base + "2015326-17-27-03rd-floor-before-swap.txt", "03_26_2015_3f1h/");
  in.emplace(base + "2015326-17-36-233rd-floor-after-swap.txt", "03_26_2015_3f2h/");

  in.emplace(base + "2015326-17-47-114th-floor-before-swap.txt", "03_26_2015_4f1h/");
  in.emplace(base + "2015326-17-56-334th-floor-after-swap.txt", "03_26_2015_4f2h/");

  in.emplace(base + "2015326-18-4-425th-floor-before-swap.txt", "03_26_2015_5f1h/");
  in.emplace(base + "2015326-18-14-315th-floor-after-swap.txt", "03_26_2015_5f2h/");

  IEEEMachine ieeeMachine;
  ieeeMachine.process(in);
}

int main()
{
  std::cout << "*** starts ***" << std::endl;
  ieeeMachineSingleProcess();
//  ieeeMachineMultipleProcess();
  std::cout << "*** ends   ***" << std::endl;
  return 0;
}
