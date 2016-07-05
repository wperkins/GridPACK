/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   ca_factory.hpp
 * @author Yousu Chen 
 * @date   Feb 11, 2014 
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _ca_factory_h_
#define _ca_factory_h_

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/factory/base_factory.hpp"
#include "ca_components.hpp"
#include "ca_driver.hpp"
#include "gridpack/math/matrix.hpp"

typedef gridpack::network::BaseNetwork< gridpack::contingency_analysis::CABus,
        gridpack::contingency_analysis::CABranch > CANetwork;

namespace gridpack {
namespace contingency_analysis {

class CAFactory
  : public gridpack::factory::BaseFactory<CANetwork> {
  public:
    /**
     * Basic constructor
     * @param network: network associated with factory
     */
    CAFactory(NetworkPtr network);

    /**
     * Basic destructor
     */
    ~CAFactory();

    /**
     * Reset voltages to initial values on all buses
     */
    void resetVoltage(void);

    /**
     * Create the admittance (Y-Bus) matrix
     */
    void setYBus(void);

    /**
     * Make SBus vector 
     */
    void setSBus(void);

    /**
     * Create the PQ 
     */
    void setPQ(void);

    /**
     * Set contingency
     * @param contingency the contigency that is to be set
     */
    void setContingency(gridpack::contingency_analysis::Contingency contingency);

    /**
     * Clear contingency and set branch to its pre-contingency state
     */
    void clearContingency(gridpack::contingency_analysis::Contingency
        contingency);
    
    /**
     * Check for lone buses in the system. Do this by looking for buses that
     * have no branches attached to them or for whom all the branches attached
     * to the bus have all transmission elements with status false (the element
     * is off). Set status of bus to isolated so that it does not contribute to
     * powerflow matrix
     * @param stream optional stream pointer that can be used to print out IDs
     * of isolated buses
     * @return false if there is an isolated bus in the network
     */
    bool checkLoneBus(std::ofstream *stream = NULL);

    /**
     * Set lone buses back to their original status.
     */
    void clearLoneBus();

    /**
     * Check to see if there any violations on the network
     * @param minV maximum voltage limit
     * @param maxV maximum voltage limit
     * @param bus_ok return true if no violations on buses
     * @param branch_ok return true if no violations on branches
     */
    void  checkContingencies(double minV, double maxV,
        bool *bus_ok, bool *branch_ok);


  private:

    NetworkPtr p_network;
    std::vector<bool> p_saveStatus;
    std::vector<bool> p_saveIsolatedStatus;
};

} // contingency_analysis
} // gridpack
#endif
