//============================================================================
// Name        : ieee_sensor_journal.cpp
// Author      : Sam Abeyruwan
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>

bool checkForDouble(std::string const& s)
{
  std::istringstream ss(s);
  double d;
  return (ss >> d) && (ss >> std::ws).eof();
}

int main()
{
  std::cout << "*** starts ***" << std::endl;

  const std::string inputName = "2015330-18-8-39data-talk-ub506-person10.txt";
  const std::string outputDir = "03_30_2015/";

  const std::string deviceOutName[] =
  { outputDir + "7ef", outputDir + "7cd", outputDir + "8d4" };

  std::ifstream in(inputName.c_str());

  std::map<std::string, std::ofstream*> outstreams;
  outstreams.insert(std::make_pair("7ef", new std::ofstream(deviceOutName[0].c_str())));
  outstreams.insert(std::make_pair("7cd", new std::ofstream(deviceOutName[1].c_str())));
  outstreams.insert(std::make_pair("8d4", new std::ofstream(deviceOutName[2].c_str())));

  if (in.is_open())
  {
    std::string line;
    std::vector<std::string> tokens;
    int lineNumber = 0;
    size_t maxValue = 0;
    size_t minValue = 100;
    while (std::getline(in, line))
    {
      tokens.clear();
      std::istringstream iss(line);
      std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
          std::back_inserter(tokens));

      maxValue = std::max(maxValue, tokens.size());
      minValue = std::min(minValue, tokens.size());

      //std::cout << "lineNumber: " << (++lineNumber) << " size: " << tokens.size() << " min: "
      //    << minValue << " max: " << maxValue << std::endl;

      std::map<std::string, std::ofstream*>::iterator iter = outstreams.find(*tokens.begin());
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
          if (!checkForDouble(tokens[i]))
          {
            validTokens = false;
            std::cout << "Skip: " << lineNumber << " checkForDouble: " << tokens[i] << std::endl;
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

  for (std::map<std::string, std::ofstream*>::iterator iter = outstreams.begin();
      iter != outstreams.end(); ++iter)
  {
    iter->second->close();
    delete iter->second;
  }

  std::cout << "*** ends   ***" << std::endl;
  return 0;
}
