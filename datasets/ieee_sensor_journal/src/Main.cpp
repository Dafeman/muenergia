//============================================================================
// Name        : Main.cpp
// Author      : Sam Abeyruwan
// Version     :
// Copyright   : Your copyright notice
// Description : IEEE journal paper sesnor pre-processing
//============================================================================

#include "IEEEMachine.h"
#include "IEEEClustering.h"

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

void ieeeClusteringSingleProcess()
{
  std::string inFile = "/home/sam/Projects/muenergia/datasets/human/data-fall-forward.txt";
  std::string outFile =
      "/home/sam/Projects/muenergia/datasets/ieee_sensor_journal/ieee_human/data-fall-forward.txt";
  IEEEClustering clustering(20);
  clustering.process(inFile, outFile);
}

void ieeeClusteringMultipleProcess()
{
  std::unordered_map<std::string, std::string> in;
  std::string inBase = "/home/sam/Projects/muenergia/datasets/human/";
  std::string outBase = "/home/sam/Projects/muenergia/datasets/ieee_sensor_journal/ieee_human/";

  in.emplace(inBase + "data-fall-backward.txt", outBase + "data-fall-backward.txt");
  in.emplace(inBase + "data-fall-forward.txt", outBase + "data-fall-forward.txt");
  in.emplace(inBase + "data-fall-left.txt", outBase + "data-fall-left.txt");
  in.emplace(inBase + "data-fall-right.txt", outBase + "data-fall-right.txt");
  in.emplace(inBase + "data-marching.txt", outBase + "data-marching.txt");
  in.emplace(inBase + "data-rotate-ccw.txt", outBase + "data-rotate-ccw.txt");
  in.emplace(inBase + "data-rotate-cw.txt", outBase + "data-rotate-cw.txt");
  in.emplace(inBase + "data-seat-from-stand.txt", outBase + "data-seat-from-stand.txt");
  in.emplace(inBase + "data-walk-backward.txt", outBase + "data-walk-backward.txt");
  in.emplace(inBase + "data-walk-forward.txt", outBase + "data-walk-forward.txt");
  in.emplace(inBase + "data-walk-left.txt", outBase + "data-walk-left.txt");
  in.emplace(inBase + "data-walk-right.txt", outBase + "data-walk-right.txt");

  IEEEClustering clustering(20);
  clustering.process(in);
}

int main()
{
  std::cout << "*** starts ***" << std::endl;
//  ieeeMachineSingleProcess();
//  ieeeMachineMultipleProcess();
  ieeeClusteringMultipleProcess();
  std::cout << "*** ends   ***" << std::endl;
  return 0;
}
