/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   pf_components.hpp
 * @author Bruce Palmer
 * @date   2013-10-07 10:04:43 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _pf_components_h_
#define _pf_components_h_

/**
 * Some preprocessor string declarations. These will need to be put in an
 * include file someplace else. Just declare them here for the time being.
 */

/*
#define BRANCH_REACTANCE   "branch_reactance"
#define BRANCH_RESISTANCE  "branch_resistance"
#define BRANCH_TAP_RATIO   "branch_tap_ratio"
#define BRANCH_PHASE_SHIFT "branch_phase_shift"
#define BRANCH_CHARGING    "branch_charging"
#define BUS_SHUNT_GS    "branch_shunt_gs"
#define BUS_SHUNT_BS    "branch_shunt_bs"
*/

/* These are defined in dictionary.hpp. */

/* #define BRANCH_SHUNT_ADMTTNC_G1 "branch_shunt_admttnc_g1" */
/* #define BRANCH_SHUNT_ADMTTNC_B1 "branch_shunt_admttnc_b1" */
/* #define BRANCH_SHUNT_ADMTTNC_G2 "branch_shunt_admttnc_g2" */
/* #define BRANCH_SHUNT_ADMTTNC_B2 "branch_shunt_admttnc_b2" */

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/utilities/complex.hpp"
#include "gridpack/component/base_component.hpp"
#include "gridpack/component/data_collection.hpp"
#include "gridpack/network/base_network.hpp"

namespace gridpack {
namespace powerflow {

enum PFMode{YBus, Jacobian, RHS, S_Cal};

class PFBus
  : public gridpack::component::BaseBusComponent {
  public:
    /**
     *  Simple constructor
     */
    PFBus(void);

    /**
     *  Simple destructor
     */
    ~PFBus(void);

    /**
     * Return size of matrix block on the diagonal contributed by
     * component
     * @param isize, jsize: number of rows and columns of matrix block
     * @return: false if network component does not contribute matrix
     *        element
     */
    bool matrixDiagSize(int *isize, int *jsize) const;

    /**
     * Return the values of for a diagonal matrix block. The values are
     * returned in row-major order
     * @param values: pointer to matrix block values
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixDiagValues(ComplexType *values);

    /**
     * Return size of vector block contributed by component
     * @param isize: number of vector elements
     * @return: false if network component does not contribute
     *        vector element
     */
    bool vectorSize(int *isize) const;

    /**
     * Return the values of the vector block
     * @param values: pointer to vector values
     * @return: false if network component does not contribute
     *        vector element
     */
    bool vectorValues(ComplexType *values);

    /**
     * Set the internal values of the voltage magnitude and phase angle. Need this
     * function to push values from vectors back onto buses 
     * @param values array containing voltage magnitude and angle
     */
    void setValues(gridpack::ComplexType *values);

    /**
     * Return the size of the buffer used in data exchanges on the network.
     * For this problem, the voltage magnitude and phase angle need to be exchanged
     * @return size of buffer
     */
    int getXCBufSize(void);

    /**
     * Assign pointers for voltage magnitude and phase angle
     */
    void setXCBuf(void *buf);

    /**
     * Set values of YBus matrix. These can then be used in subsequent
     * calculations
     */
    void setYBus(void);

    /**
     * Get values of YBus matrix. These can then be used in subsequent
     * calculations
     */
    gridpack::ComplexType getYBus(void);

    /**
     * Load values stored in DataCollection object into PFBus object. The
     * DataCollection object will have been filled when the network was created
     * from an external configuration file
     * @param data: DataCollection object contain parameters relevant to this
     *       bus that were read in when network was initialized
     */
    void load(const boost::shared_ptr<gridpack::component::DataCollection> &data);

    /**
     * Set the mode to control what matrices and vectors are built when using
     * the mapper
     * @param mode: enumerated constant for different modes
     */
    void setMode(int mode);

    /**
     * Return the value of the voltage magnitude on this bus
     * @return voltage magnitude
     */
    double getVoltage(void);

    /**
     * Return the complex voltage on this bus
     * @return the complex voltage
     */
    ComplexType getComplexVoltage(void);

    /**
     * Return the value of the phase angle on this bus
     * @return: phase angle
     */
    double getPhase(void);

    /**
     * Return whether or not the bus is a PV bus (V held fixed in powerflow
     * equations)
     * @return true if bus is PV bus
     */
    bool isPV(void);

    /**
     * Set voltage value
     */
    void setVoltage(void);

    /**
     * Set phase angle value
     */
    void setPhase(void);

    /**
     * setGBus
    */
    void setGBus(void);

    /**
     * setSBus
    BUS = (CG*(GEN(ON,PG) + J*GEN(ON,QG)-(PD+J*QD))/BASEMVA
    */
    void setSBus(void);

    /**
     * Write output from buses to standard out
     * @param string (output) string with information to be printed out
     * @param signal an optional character string to signal to this
     * routine what about kind of information to write
     * @return true if bus is contributing string to output, false otherwise
     */
    bool serialWrite(char *string, const char *signal = NULL);

  private:
    double p_shunt_gs;
    double p_shunt_bs;
    bool p_shunt;
    bool p_load;
    int p_mode;

    // p_v and p_a are initialized to p_voltage and p_angle respectively,
    // but may be subject to change during the NR iterations
    double p_v, p_a;
    double p_theta; //phase angle difference
    double p_ybusr, p_ybusi;
    double p_P0, p_Q0; //double p_sbusr, p_sbusi;
    double p_angle;   // initial bus angle read from parser
    double p_voltage; // initial bus voltage read from parser
    // newly added priavate variables:
    std::vector<double> p_pg, p_qg;
    std::vector<int> p_gstatus;
    double p_pl, p_ql;
    double p_sbase;
    double p_Pinj, p_Qinj;
    bool p_isPV;

    /**
     * Variables that are exchanged between buses
     */
    double* p_vMag_ptr;
    double* p_vAng_ptr;

private:


  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & boost::serialization::base_object<gridpack::component::BaseBusComponent>(*this)
      & p_shunt_gs
      & p_shunt_bs
      & p_shunt
      & p_load
      & p_mode
      & p_v & p_a & p_theta
      & p_ybusr & p_ybusi
      & p_P0 & p_Q0
      & p_angle & p_voltage
      & p_pg & p_qg
      & p_gstatus
      & p_pl & p_ql
      & p_sbase
      & p_Pinj & p_Qinj
      & p_isPV;
  }  

};

class PFBranch
  : public gridpack::component::BaseBranchComponent {
  public:
    /**
     *  Simple constructor
     */
    PFBranch(void);

    /**
     *  Simple destructor
     */
    ~PFBranch(void);

    /**
     * Return size of off-diagonal matrix block contributed by the component
     * for the forward/reverse directions
     * @param isize, jsize: number of rows and columns of matrix block
     * @return: false if network component does not contribute matrix element
     */
    bool matrixForwardSize(int *isize, int *jsize) const;
    bool matrixReverseSize(int *isize, int *jsize) const;

    /**
     * Return the values of the forward/reverse matrix block. The values are
     * returned in row-major order
     * @param values: pointer to matrix block values
     * @return: false if network component does not contribute matrix element
     */
    bool matrixForwardValues(ComplexType *values);
    bool matrixReverseValues(ComplexType *values);

    /**
     * Set values of YBus matrix. These can then be used in subsequent
     * calculations
     */
    void setYBus(void);

    /**
     * Get values of YBus matrix. These can then be used in subsequent
     * calculations
     */
    gridpack::ComplexType getYBus(void);

    /**
     * Load values stored in DataCollection object into PFBranch object. The
     * DataCollection object will have been filled when the network was created
     * from an external configuration file
     * @param data: DataCollection object contain parameters relevant to this
     *       branch that were read in when network was initialized
     */
    void load(const boost::shared_ptr<gridpack::component::DataCollection> &data);

    /**
     * Return the complex admittance of the branch
     * @return: complex addmittance of branch
     */
    gridpack::ComplexType getAdmittance(void);

    /**
     * Return transformer contribution from the branch to the calling
     * bus
     * @param bus: pointer to the bus making the call
     * @return: contribution from transformers to Y matrix
     */
    gridpack::ComplexType getTransformer(PFBus *bus);

    /**
     * Return the contribution to a bus from shunts
     * @param bus: pointer to the bus making the call
     * @return: contribution to Y matrix from shunts associated with branches
     */
    gridpack::ComplexType getShunt(PFBus *bus);

    /**
     * Return the contribution to the Jacobian for the powerflow equations from
     * a branch
     * @param bus: pointer to the bus making the call
     * @param values: an array of 4 doubles that holds return metrix elements
     * @return: contribution to Jacobian matrix from branch
     */
    void getJacobian(PFBus *bus, double *values);

    /**
     * Return contribution to constraints
     * @param p: real part of constraint
     * @param q: imaginary part of constraint
     */
    void getPQ(PFBus *bus, double *p, double *q);

    /**
     * Set the mode to control what matrices and vectors are built when using
     * the mapper
     * @param mode: enumerated constant for different modes
     */
    void setMode(int mode);

    /**
     * Write output from branches to standard out
     * @param string (output) string with information to be printed out
     * @param signal an optional character string to signal to this
     * routine what about kind of information to write
     * @return true if branch is contributing string to output, false otherwise
     */
    bool serialWrite(char *string, const char *signal = NULL);

  private:
    double p_reactance;
    double p_resistance;
    double p_tap_ratio;
    double p_phase_shift;
    double p_charging;
    double p_shunt_admt_g1;
    double p_shunt_admt_b1;
    double p_shunt_admt_g2;
    double p_shunt_admt_b2;
    bool p_xform, p_shunt;
    int p_mode;
    double p_ybusr_frwd, p_ybusi_frwd;
    double p_ybusr_rvrs, p_ybusi_rvrs;
    double p_theta;
    double p_sbase;

private:


  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & boost::serialization::base_object<gridpack::component::BaseBranchComponent>(*this)
      & p_reactance
      & p_resistance
      & p_tap_ratio
      & p_phase_shift
      & p_charging
      & p_shunt_admt_g1
      & p_shunt_admt_b1
      & p_shunt_admt_g2
      & p_shunt_admt_b2
      & p_xform & p_shunt
      & p_mode
      & p_ybusr_frwd & p_ybusi_frwd
      & p_ybusr_rvrs & p_ybusi_rvrs
      & p_theta
      & p_sbase;
  }  

};


/// The type of network used in the powerflow application
typedef network::BaseNetwork<PFBus, PFBranch > PFNetwork;


}     // powerflow
}     // gridpack

BOOST_CLASS_EXPORT_KEY(gridpack::powerflow::PFBus);
BOOST_CLASS_EXPORT_KEY(gridpack::powerflow::PFBranch);


#endif
