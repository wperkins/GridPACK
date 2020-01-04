/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   genrou.hpp
 * @author Shuangshuang Jin 
 * @Last modified:   07/25/19
 * 
 * @brief  
 * 
 * 
 */

#ifndef _genrou_h_
#define _genrou_h_

#include <base_gen_model.hpp>
#include <gridpack/include/gridpack.hpp>
#include "boost/smart_ptr/shared_ptr.hpp"

class GenrouGen: public BaseGenModel
{
   public:
  /**
     * Basic constructor
     */
    GenrouGen();

    /**
     * Basic destructor
     */
    ~GenrouGen();

    /**
     * Load parameters from DataCollection object into generator model
     * @param data collection of generator parameters from input files
     * @param index of generator on bus
     * TODO: might want to move this functionality to BaseGeneratorModel
     */
    void load(const boost::shared_ptr<gridpack::component::DataCollection>
        data, int idx);

    /**
     * Initialize generator model before calculation
     * @param [output] values - array where initialized generator variables should be set
     */
  void init(gridpack::ComplexType *values);

    /**
     * Write output from generators to a string.
     * @param string (output) string with information to be printed out
     * @param bufsize size of string buffer in bytes
     * @param signal an optional character string to signal to this
     * routine what about kind of information to write
     * @return true if bus is contributing string to output, false otherwise
     */
    bool serialWrite(char *string, const int bufsize,
        const char *signal);

    double getAngle();

    /**
     * Write out generator state
     * @param signal character string used to determine behavior
     * @param string buffer that contains output
     */
    void write(const char* signal, char* string);

    /**
     *  Set the number of variables for this generator model
     *  @param [output] number of variables for this model
     */
    bool vectorSize(int *nvar) const;

    /**
     * Set the internal values of the voltage magnitude and phase angle. Need this
     * function to push values from vectors back onto generators
     * @param values array containing generator state variables
     */
     void setValues(gridpack::ComplexType*);

    /**
     * Return the values of the generator vector block
     * @param values: pointer to vector values
     * @return: false if generator does not contribute
     *        vector element
     */
    bool vectorValues(gridpack::ComplexType *values);

    /**
     * Return the generator current injection (in rectangular form) 
     * @param [output] IGD - real part of the generator current
     * @param [output] IGQ - imaginary part of the generator current
     */
    void getCurrent(double *IGD, double *IGQ);

    /**
     * Return the matrix entries
     * @param [output] nval - number of values set
     * @param [output] row - row indices for matrix entries
     * @param [output] col - col indices for matrix entries
     * @param [output] values - matrix entries
     * return true when matrix entries set
     */
    bool matrixDiagEntries(int *nval,int *row, int *col, gridpack::ComplexType *values);

    /* return field current */
    double getFieldCurrent();

    /* return rotor speed deviation */
    double getRotorSpeedDeviation();
  private:
    // Machine parameters
    double H, D, Ra, Xd, Xq, Xdp, Xdpp, Xl, Xqp, Xqpp;
    double Tdop, Tdopp, Tqopp, S10, S12, Tqop;

    // Internal constants
    double Efd; // Field voltage
    double LadIfd; // Field current
    double Pmech; // Mechanical power
  
    // Generator variables and their derivatives
    double delta, dw, Eqp, Psidp, Psiqp, Edp; 
    double ddelta, ddw, dEqp, dPsidp, dPsiqp, dEdp;
    double Id, Iq; // Generator current on d and q axis

    // previous step values of the variables
    double deltaprev, dwprev, Eqpprev, Psidpprev, Psiqpprev, Edpprev; 

    double B, G;


    double sat_A, sat_B; // Saturation constant
    int bid;

    /**
     * Saturation function
     * @ param x
     */
    double Sat(double psidpp,double psiqpp); 

};

#endif
