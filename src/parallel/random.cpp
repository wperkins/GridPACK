// Emacs Mode Line: -*- Mode:c++;-*-
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   random.cpp
 * @author Bruce Palmer
 * @date   2016-07-01 10:37:20 d3g096
 * 
 * @brief  
 * This is a wrapper for a random number generator. The current implementation
 * relies on the standard random number generator in C++ and all the caveats
 * that apply to default random number generators should be noted.
 * 
 */

// -------------------------------------------------------------

#include <cstdlib>
#include <unistd.h>
#include <cmath>

#include "gridpack/parallel/random.hpp"

// -------------------------------------------------------------
//  class Random
// -------------------------------------------------------------

namespace gridpack {
namespace random {
/**
 * Default constructor
 */
Random::Random()
{
  p_iset = 0;
  p_gset = 0.0;
  p_rand_max_i = 1.0/static_cast<double>(RAND_MAX);
}

/**
 * Initialize random number generator with a seed
 * @param seed random number generator initialization
 */
Random::Random(int seed)
{
  p_iset = 0;
  p_gset = 0.0;
  p_rand_max_i = 1.0/static_cast<double>(RAND_MAX);
  if (seed < 0) seed = -seed;
  seed += ::getpid();
  srand(static_cast<unsigned int>(seed));
}

/**
 * Default destructor
 */
Random::~Random(void)
{
}

/**
 * Reinitialize random number generator with a new seed
 * @param seed random number generator initialization
 */
void Random::Random::seed(int seed)
{
  if (seed < 0) seed = -seed;
  seed += ::getpid();
  srand(static_cast<unsigned int>(seed));
}

/**
 * Return a double precision random number in the range [0,1]
 */
double Random::Random::drand(void)
{
  return rand()*p_rand_max_i;
}

/**
 * Return a double precision random number from a gaussian distribution with
 * unit variance (p(x) = exp(-x*x/2)/sqrt(2*pi))
 */
double Random::Random::grand(void)
{
  if (p_iset == 0) {
    double r = 2.0;
    double x, y;
    while (r >= 1.0) {
      x = 2.0*p_rand_max_i*rand() - 1.0;
      y = 2.0*p_rand_max_i*rand() - 1.0;
      r = x*x + y*y;
    }
    double logr = sqrt(-2.0*log(r)/r);
    p_gset = x * logr;
    p_iset = 1;
    return y * logr;
  } else {
    p_iset = 0;
    return p_gset;
  }
  return 0.0;
}

}  // random
}  // gridpack
