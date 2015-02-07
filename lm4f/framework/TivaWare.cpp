/*
 * TivaWare.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: sam
 */

#include "TivaWare.h"

#if defined(EMBEDDED_MODE)

TivaWareController* TivaWareController::theInstance = 0;

TivaWareController::TivaWareController()
{
}

TivaWareController::~TivaWareController()
{
}

TivaWareController& TivaWareController::getInstance()
{
  if (!theInstance)
    theInstance = new TivaWareController;
  return *theInstance;
}

void TivaWareController::deleteInstance()
{
  if (theInstance)
    delete theInstance;
  theInstance = 0;
}

#endif

