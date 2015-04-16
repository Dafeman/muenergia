/*
 * IEEEClustering.cpp
 *
 *  Created on: Apr 14, 2015
 *      Author: sam
 */

#include "IEEEClustering.h"
//
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <cassert>
//

IEEEClustering::IEEEClustering(const int& windowLength)
    : windowLength(windowLength), halfWindowLength(windowLength / 2)
{
}

void IEEEClustering::process(const std::string& inFile, const std::string outFile)
{
  std::ifstream in(inFile.c_str());

  std::vector<std::vector<double>> data;

  int minValue = std::numeric_limits<int>::max();
  int maxValue = std::numeric_limits<int>::min();

  if (in.is_open())
  {
    std::string str;
    std::vector<double> tokens;
    while (std::getline(in, str))
    {
      tokens.clear();
      std::istringstream iss(str);
      std::copy(std::istream_iterator<double>(iss), std::istream_iterator<double>(),
          std::back_inserter(tokens));
      data.push_back(tokens);

      minValue = std::min(minValue, static_cast<int>(tokens.size()));
      maxValue = std::max(maxValue, static_cast<int>(tokens.size()));
    }
  }
  else
  {
    std::cerr << "inFile: " << inFile << " not found!" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::cout << "data: " << data.size() << " min: " << minValue << " max: " << maxValue << std::endl;

  assert(minValue == maxValue);

  std::vector<std::vector<double>> pdata;

  for (size_t i = 0; i < data.size(); i += halfWindowLength)
  {
    std::vector<double> features(minValue, 0);
    const size_t iEnd = std::min(i + windowLength, data.size());
    for (size_t j = i; j < iEnd; ++j)
    {
      for (size_t k = 0; k < data[j].size(); ++k)
        features[k] += (data[j][k] / iEnd);
    }
    pdata.push_back(features);
  }

  std::cout << "pdata: " << pdata.size() << std::endl;

  std::ofstream out(outFile.c_str());

  for (size_t i = 0; i < pdata.size(); ++i)
  {
    for (size_t j = 0; j < pdata[i].size(); ++j)
      out << pdata[i][j] << " ";
    out << std::endl;
    out.flush();
  }

}

void IEEEClustering::process(const std::unordered_map<std::string, std::string>& dataDescriptors)
{
  for (std::unordered_map<std::string, std::string>::const_iterator iter = dataDescriptors.begin();
      iter != dataDescriptors.end(); ++iter)
  {
    std::cout << "starts: " << iter->first << " ends: " << iter->second << std::endl;
    process(iter->first, iter->second);
    std::cout << "ends  : " << iter->first << " ends: " << iter->second << std::endl;
  }

}

