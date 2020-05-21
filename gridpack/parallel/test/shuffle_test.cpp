/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
/**
 * @file   shuffle_test.cpp
 * @author William A. Perkins
 * @date   2014-12-09 09:43:15 d3g096
 * 
 * @brief  A test of the Shuffler<> class
 * 
 * 
 */

#include <iostream>
#include <iterator>
#include <boost/lexical_cast.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>
#include <boost/serialization/string.hpp>

#include "printit.hpp"
#include "shuffler.hpp"
#include "ga_shuffler.hpp"

#include "gridpack/environment/environment.hpp"

// -------------------------------------------------------------
// struct Tester
// -------------------------------------------------------------
/// A more complicated thing to shuffle
struct Tester {
  int index;
  std::string label;
  explicit Tester(int i) 
    : index(i), label("unset") 
  {
    char c('A');
    c += i%26;
    label = std::string((i % 10) + 1, c);
  }
  Tester(void) 
    : index(-1), label("unset")
  {}
private: 
  friend class boost::serialization::access;
  /// Serialization method
  template<class Archive> void serialize(Archive &ar, const unsigned int)
  {
    ar & index & label;
  }
};

std::ostream& 
operator<<(std::ostream& out, const Tester& t)
{
  out << t.index << " (" << t.label << ")";
  return out;
}

BOOST_AUTO_TEST_SUITE ( shuffler ) 

BOOST_AUTO_TEST_CASE( int_shuffle )
{
  gridpack::parallel::Communicator comm;
  boost::mpi::communicator world(static_cast<MPI_Comm>(comm),
      boost::mpi::comm_duplicate);
  const int local_size(5);
  const int global_size(local_size*world.size());
  std::vector<int> things;
  std::vector<int> dest;
  
  if (world.rank() == 0) {
    things.resize(global_size);
    dest.resize(global_size);
    for (int i = 0; i < local_size*world.size(); ++i) {
      things[i] = i;
      dest[i] = i % world.size();
    }
  }

  printit(world, things, "Before:");

  gridpack::parallel::Shuffler<int> shuffle(world);
  shuffle(things, dest);

  printit(world, things, "After:");
}

BOOST_AUTO_TEST_CASE( string_shuffle )
{
  gridpack::parallel::Communicator comm;
  boost::mpi::communicator world(static_cast<MPI_Comm>(comm),
      boost::mpi::comm_duplicate);
  const int local_size(5);
  const int global_size(local_size*world.size());
  std::vector<std::string> things;
  std::vector<int> dest;
  
  if (world.rank() == 0) {
    things.resize(global_size);
    dest.resize(global_size);
    for (int i = 0; i < local_size*world.size(); ++i) {
      char c('A');
      c += i;
      things[i] = c;
      dest[i] = i % world.size();
    }
  }

  printit(world, things, "Before:");

  gridpack::parallel::Shuffler<std::string> shuffle(world);
  shuffle(things, dest);

  printit(world, things, "After:");
}

BOOST_AUTO_TEST_CASE( tester_shuffle )
{
  gridpack::parallel::Communicator comm;
  boost::mpi::communicator world(static_cast<MPI_Comm>(comm),
      boost::mpi::comm_duplicate);
  const int local_size(5);
  const int global_size(local_size*world.size());
  std::vector<Tester> things;
  std::vector<int> dest;
  
  if (world.rank() == 0) {
    things.reserve(global_size);
    dest.reserve(global_size);
    for (int i = 0; i < local_size*world.size(); ++i) {
      things.push_back(Tester(i));
      dest.push_back(i % world.size());
    }
  }

  printit(world, things, "Before:");

  gridpack::parallel::Shuffler<Tester> shuffle(world);
  shuffle(things, dest);

  printit(world, things, "After:");
}

BOOST_AUTO_TEST_CASE( multi_tester_shuffle )
{
  gridpack::parallel::Communicator comm;
  boost::mpi::communicator world(static_cast<MPI_Comm>(comm),
      boost::mpi::comm_duplicate);
  const int local_size(5);
  std::vector<Tester> things;
  std::vector<int> dest;

  things.reserve(local_size);
  dest.reserve(local_size);
  for (int i = 0; i < local_size; ++i) {
    things.push_back(Tester(i));
    dest.push_back(i % world.size());
  }

  printit(world, things, "Before:");
  printit(world, dest, "Destination:");

  gridpack::parallel::Shuffler<Tester> shuffle(world);
  shuffle(things, dest);

  printit(world, things, "After:");

  // move all to root process
  dest.clear();
  dest.resize(things.size(), 0);
  printit(world, dest, "Destination:");

  shuffle(things, dest);

  printit(world, things, "After:");
    
}

BOOST_AUTO_TEST_SUITE_END( )

BOOST_AUTO_TEST_SUITE( gaShufflerTest )

BOOST_AUTO_TEST_CASE( stringShuffle )
{
  gridpack::parallel::Communicator world;
  const int local_size(5);
  const int global_size(local_size*world.size());
  std::vector<std::string> things;
  std::vector<int> dest;
  
  if (world.rank() == 0) {
    things.resize(global_size);
    dest.resize(global_size);
    for (int i = 0; i < local_size*world.size(); ++i) {
      char c('A');
      c += i;
      things[i] = c;
      dest[i] = i % world.size();
    }
  }

  printit(world, things, "Before:");

  gridpack::parallel::gaShuffler<std::string, int> shuffler(world);

  shuffler(things, dest);

  printit(world, things, "After:");
}

BOOST_AUTO_TEST_CASE( testerShuffle )
{
  gridpack::parallel::Communicator comm;
  boost::mpi::communicator world(static_cast<MPI_Comm>(comm),
      boost::mpi::comm_duplicate);
  const int local_size(5);
  const int global_size(local_size*world.size());
  std::vector<Tester> things;
  std::vector<int> dest;
  
  if (world.rank() == 0) {
    things.reserve(global_size);
    dest.reserve(global_size);
    for (int i = 0; i < local_size*world.size(); ++i) {
      things.push_back(Tester(i));
      dest.push_back(i % world.size());
    }
  }

  printit(world, things, "Before:");

  gridpack::parallel::gaShuffler<Tester> shuffler(world);

  shuffler(things, dest);

  printit(world, things, "After:");
}


BOOST_AUTO_TEST_SUITE_END( )

bool init_function()
{
  return true;
}

// -------------------------------------------------------------
//  Main Program
// -------------------------------------------------------------
int
main(int argc, char **argv)
{
  gridpack::Environment env(argc, argv);
  return ::boost::unit_test::unit_test_main( &init_function, argc, argv );
}

