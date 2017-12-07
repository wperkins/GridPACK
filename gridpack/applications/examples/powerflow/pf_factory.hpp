/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   pf_factory.hpp
 * @author Bruce Palmer
 * @date   2014-01-28 11:33:42 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _pf_factory_h_
#define _pf_factory_h_

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/include/gridpack.hpp"
#include "gridpack/applications/components/pf_matrix/pf_components.hpp"

namespace gridpack {
namespace powerflow {

// Define the type of network used in the powerflow application
typedef network::BaseNetwork<PFBus, PFBranch > PFNetwork;

class PFFactory
  : public gridpack::factory::BaseFactory<PFNetwork> {
  public:
    /**
     * Basic constructor
     * @param network: network associated with factory
     */
    PFFactory(NetworkPtr network);

    /**
     * Basic destructor
     */
    ~PFFactory();

    /**
     * Create the admittance (Y-Bus) matrix.
     */
    void setYBus(void);

    /**
     * Make SBus vector 
     */
    void setSBus(void);

  private:

    NetworkPtr p_network;
};

} // powerflow
} // gridpack
#endif
