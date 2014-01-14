/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   pf_app.cpp
 * @author Bruce Palmer
 * @date   2014-01-10 09:50:57 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include <gridpack/math/matrix.hpp>
#include <gridpack/math/vector.hpp>
#include <gridpack/math/linear_solver.hpp>
#include <gridpack/math/newton_raphson_solver.hpp>
#include <gridpack/math/nonlinear_solver.hpp>
#include <gridpack/parser/PTI23_parser.hpp>
#include <gridpack/configuration/configuration.hpp>
#include <gridpack/mapper/bus_vector_map.hpp>
#include <gridpack/mapper/full_map.hpp>
#include <gridpack/parser/PTI23_parser.hpp>
#include <gridpack/serial_io/serial_io.hpp>
#include <gridpack/timer/coarse_timer.hpp>

#include "pf_app.hpp"
#include "pf_factory.hpp"

// Calling program for powerflow application

/**
 * Basic constructor
 */
gridpack::powerflow::PFApp::PFApp(void)
{
}

/**
 * Basic destructor
 */
gridpack::powerflow::PFApp::~PFApp(void)
{
}

/**
 * Execute application
 */
void gridpack::powerflow::PFApp::execute(void)
{
  // load input file
  gridpack::utility::CoarseTimer *timer =
    gridpack::utility::CoarseTimer::instance();
  int t_total = timer->createCategory("Total Application");
  timer->start(t_total);
  gridpack::parallel::Communicator world;
  boost::shared_ptr<PFNetwork> network(new PFNetwork(world));

  // read configuration file
  gridpack::utility::Configuration *config = gridpack::utility::Configuration::configuration();
  config->open("input.xml",world);
  gridpack::utility::Configuration::CursorPtr cursor;
  cursor = config->getCursor("Configuration.Powerflow");
  std::string filename = cursor->get("networkConfiguration",
      "No network configuration specified");

  int t_pti = timer->createCategory("PTI Parser");
  timer->start(t_pti);
  gridpack::parser::PTI23_parser<PFNetwork> parser(network);
  parser.parse(filename.c_str());
  timer->stop(t_pti);

//  std::string unpartout(cursor->get("networkUnpartitionedGraph", ""));
//  std::string partout(cursor->get("networkPartitionedGraph", ""));

//  if (!unpartout.empty()) {
//    network->writeGraph(unpartout);
//  }

  // partition network
  int t_part = timer->createCategory("Partition");
  timer->start(t_part);
  network->partition();
  timer->stop(t_part);

//  if (!partout.empty()) {
//    network->writeGraph(partout);
//  }

  // create factory
  gridpack::powerflow::PFFactory factory(network);
  int t_fact = timer->createCategory("Factory");
  timer->start(t_fact);
  factory.load();

  // set network components using factory
  factory.setComponents();

  // Set up bus data exchange buffers. Need to decide what data needs to be
  // exchanged
  factory.setExchange();
  timer->stop(t_fact);

  // Create bus data exchange
  int t_updt = timer->createCategory("Bus Update");
  timer->start(t_updt);
  network->initBusUpdate();
  timer->stop(t_updt);

  // set YBus components so that you can create Y matrix
  timer->start(t_fact);
  factory.setYBus();
  timer->stop(t_fact);

  // Create serial IO object to export data from buses
  gridpack::serial_io::SerialBusIO<PFNetwork> busIO(128,network);
  char ioBuf[128];

  int t_cmap = timer->createCategory("Create Mappers");
  timer->start(t_cmap);
  factory.setMode(YBus); 
  gridpack::mapper::FullMatrixMap<PFNetwork> mMap(network);
  timer->stop(t_cmap);
  int t_mmap = timer->createCategory("Map to Matrix");
  timer->start(t_mmap);
  boost::shared_ptr<gridpack::math::Matrix> Y = mMap.mapToMatrix();
  timer->stop(t_mmap);
//  busIO.header("\nY-matrix values\n");
//  Y->print();
//  Y->save("Ybus.m");

  timer->start(t_fact);
  factory.setMode(S_Cal);
  timer->stop(t_fact);
  timer->start(t_cmap);
  gridpack::mapper::BusVectorMap<PFNetwork> vvMap(network);
  timer->stop(t_cmap);
  int t_vmap = timer->createCategory("Map to Vector");
  timer->start(t_vmap);
  boost::shared_ptr<gridpack::math::Vector> vv = vvMap.mapToVector();
  timer->stop(t_vmap);
//  vv->save("vv.m");

  boost::shared_ptr<gridpack::math::Vector> Yvv(multiply(*Y, *vv)); 
//  Yvv->save("Yvv.m");


  // make Sbus components to create S vector
  timer->start(t_fact);
  factory.setSBus();
  timer->stop(t_fact);
  busIO.header("\nIteration 0\n");

  // Set PQ
  timer->start(t_cmap);
  factory.setMode(RHS); 
  gridpack::mapper::BusVectorMap<PFNetwork> vMap(network);
  timer->stop(t_cmap);
  timer->start(t_vmap);
  boost::shared_ptr<gridpack::math::Vector> PQ = vMap.mapToVector();
  timer->stop(t_vmap);
//  busIO.header("\nPQ values\n");
//  PQ->print();
//  busIO.header("\n   Elements of PQ vector\n");
//  busIO.header("\n   Bus Number           P                   Q      Neighbors\n");
//  busIO.write("pq");
//  PQ->save("PQ.m");
#if 1
  timer->start(t_cmap);
  factory.setMode(Jacobian);
  gridpack::mapper::FullMatrixMap<PFNetwork> jMap(network);
  timer->stop(t_cmap);
  timer->start(t_mmap);
  boost::shared_ptr<gridpack::math::Matrix> J = jMap.mapToMatrix();
  timer->stop(t_mmap);
  busIO.header("\nJacobian values\n");
//  J->print(); 
//  J->save("J.m");


#if 1
  boost::shared_ptr<gridpack::math::Vector> X(PQ->clone());

  // Convergence and iteration parameters
  double tolerance;
  int max_iteration;
  ComplexType tol;

  // These need to eventually be set using configuration file
  tolerance = 1.0e-5;
  max_iteration = 50;

  // Create linear solver
  int t_csolv = timer->createCategory("Create Linear Solver");
  timer->start(t_csolv);
  gridpack::math::LinearSolver isolver(*J);
  isolver.configure(cursor);
  timer->stop(t_csolv);

  tol = 2.0*tolerance;
  int iter = 0;

  // First iteration
  X->zero(); //might not need to do this
  busIO.header("\nCalling solver\n");
  int t_lsolv = timer->createCategory("Solve Linear Equation");
  timer->start(t_lsolv);
  isolver.solve(*PQ, *X);
  timer->stop(t_lsolv);
  tol = PQ->normInfinity();
//  busIO.header("\nX values\n");
//  X->save("X.m");

  // Create timer for map to bus
  int t_bmap = timer->createCategory("Map to Bus");

  while (real(tol) > tolerance && iter < max_iteration) {
    // Push current values in X vector back into network components
    // Need to implement setValues method in PFBus class in order for this to
    // work
    timer->start(t_bmap);
    factory.setMode(RHS);
    vMap.mapToBus(X);
    timer->stop(t_bmap);

    // Exchange data between ghost buses (I don't think we need to exchange data
    // between branches)
    timer->start(t_updt);
    network->updateBuses();
    timer->stop(t_updt);

    // Create new versions of Jacobian and PQ vector
    timer->start(t_vmap);
    vMap.mapToVector(PQ);
    timer->stop(t_vmap);
//    sprintf(ioBuf,"\nIteration %d Print PQ\n",iter+1);
//    busIO.header(ioBuf);
//    PQ->print();
    timer->start(t_mmap);
    factory.setMode(Jacobian);
    jMap.mapToMatrix(J);
    timer->stop(t_mmap);

    // Create linear solver
    timer->start(t_lsolv);
    gridpack::math::LinearSolver solver(*J);
    solver.configure(cursor);
//    sprintf(ioBuf,"\nIteration %d Print X\n",iter+1);
//    busIO.header(ioBuf);
    X->zero(); //might not need to do this
    solver.solve(*PQ, *X);
    timer->stop(t_lsolv);
//    X->print();

    tol = PQ->normInfinity();
    sprintf(ioBuf,"\nIteration %d Tol: %12.6e\n",iter+1,real(tol));
    busIO.header(ioBuf);
    iter++;
  }
#endif
  // Push final result back onto buses
  timer->start(t_bmap);
  factory.setMode(RHS);
  vMap.mapToBus(X);
  timer->stop(t_bmap);

  gridpack::serial_io::SerialBranchIO<PFNetwork> branchIO(128,network);
  branchIO.header("\n   Branch Power Flow\n");
  branchIO.header("\n        Bus 1       Bus 2            P"
                  "                    Q\n");
  branchIO.write();


  busIO.header("\n   Bus Voltages and Phase Angles\n");
  busIO.header("\n   Bus Number      Phase Angle      Voltage Magnitude\n");
  busIO.write();
#endif
  timer->stop(t_total);
  timer->dump();

}
