/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   pf_components.cpp
 * @author Bruce Palmer
 * @date   2014-02-13 07:35:47 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include <vector>
#include <iostream>
#include <cstring>
#include <stdio.h>

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/utilities/complex.hpp"
#include "gridpack/component/base_component.hpp"
#include "gridpack/component/data_collection.hpp"
#include "ca_components.hpp"
#include "gridpack/parser/dictionary.hpp"

/**
 *  Simple constructor
 */
gridpack::contingency_analysis::CABus::CABus(void)
{
  p_vMin = 0.9;
  p_vMax = 1.1;
}

/**
 *  Simple destructor
 */
gridpack::contingency_analysis::CABus::~CABus(void)
{
}

/**
 * Set voltage limits on bus. Voltage magnitudes that are outside limits
 * represent a contingency violation
 * @param vMin minimum voltage
 * @param vMax maximum voltage
 */
void gridpack::contingency_analysis::CABus::setVoltageLimits(double vMin, double vMax)
{
  p_vMin = vMin;
  p_vMax = vMax;
}

/**
 * Write output from buses to standard out
 * @param string (output) string with information to be printed out
 * @param bufsize size of string buffer in bytes
 * @param signal an optional character string to signal to this
 * routine what about kind of information to write
 * @return true if bus is contributing string to output, false otherwise
 */
bool gridpack::contingency_analysis::CABus::serialWrite(char *string,
    const int bufsize, const char *signal)
{
  if (signal != NULL && !strcmp(signal,"violations_only")) {
    double V = getVoltage();
    if (V < p_vMin || V > p_vMax) {
      sprintf(string, "     %6d      %12.6f\n",
        getOriginalIndex(),V);
      return true;
    } else {
      return false;
    }
  } else {
    return PFBus::serialWrite(string, bufsize, signal);
  }
}

/**
 * Return the size of the buffer used in data exchanges on the network.
 * For this problem, the power flow data need to be exchanged plus data
 * for bool that keeps track of the isolated status of the bus
 * @return size of buffer
 */
int gridpack::contingency_analysis::CABus::getXCBufSize(void)
{
  return sizeof(bool) + PFBus::getXCBufSize();
}

/**
 * Assign pointers for powerflow data and isolated status variable
 */
void gridpack::contingency_analysis::CABus::setXCBuf(void *buf)
{
  p_isolated = static_cast<bool*>(buf);
  bool *ptr = p_isolated + 1;
  PFBus::setXCBuf(ptr);
}

/**
 *  Simple constructor
 */
gridpack::contingency_analysis::CABranch::CABranch(void)
{
}

/**
 *  Simple destructor
 */
gridpack::contingency_analysis::CABranch::~CABranch(void)
{
}

