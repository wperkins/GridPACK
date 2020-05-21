/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
#include <vector>

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>

#include "mpi.h"
#include <macdecls.h>
#include "gridpack/network/base_network.hpp"

#define XDIM 20
#define YDIM 20


class TestBus
  : public gridpack::component::BaseBusComponent {
  public:

  TestBus(void) {
  }

  ~TestBus(void) {
  }
};

BOOST_CLASS_EXPORT(TestBus)

class TestBranch
  : public gridpack::component::BaseBranchComponent {
  public:

  TestBranch(void) {
  }

  ~TestBranch(void) {
  }
};

BOOST_CLASS_EXPORT(TestBranch)

void factor_grid(int nproc, int xsize, int ysize, int *pdx, int *pdy)
{
  int i,j,it,ip,ifac,pmax,prime[1000], chk;
  int idx, idy;
  double xtmp, ytmp;
  int fac[1000];

  ip = nproc;

  /* Factor nproc completely. First, find all prime numbers less than or equal
   * to nproc. */
  pmax = 0;
  for (i=2; i<=nproc; i++) {
    chk = 0;
    for (j=0; j<pmax; j++) {
      if (i%prime[j] == 0) {
        chk = 1;
        break;
      }
    }
    if (!chk) {
      prime[pmax] = i;
      pmax++;
    }
  }

  /* Find all prime factors of nproc */
  ifac = 0;
  for (i=0; i<pmax; i++) {
    while(ip%prime[i] == 0 && ip > 1) {
      ifac++;
      fac[ifac] = prime[i];
      ip = ip / prime[i];
    }
  }
  /* Find three factors of nproc such that the resulting three dimensions of
     the simulation cell on each processor are as close as possible to being
     the same size */
  xtmp = xsize;
  ytmp = ysize;
  idx = 1;
  idy = 1;
  for (i=ifac; i>0; i--) {
    if (xtmp >= ytmp) {
      idx = fac[i]*idx;
      xtmp /= fac[i];
    } else {
      idy = fac[i]*idy;
      ytmp /= fac[i];
    }
  }
  *pdx = idx;
  *pdy = idy;
}

BOOST_AUTO_TEST_SUITE ( TestNetwork )

BOOST_AUTO_TEST_CASE( TestNetworkTopology )
{

  gridpack::parallel::Communicator comm;
  MPI_Comm mpi_world = static_cast<MPI_Comm>(comm);
  int ierr;
  int me;
  ierr = MPI_Comm_rank(mpi_world, &me);
  int nprocs;
  ierr = MPI_Comm_size(mpi_world, &nprocs);

  // Create network
  gridpack::parallel::Communicator world;
  gridpack::network::BaseNetwork<TestBus, TestBranch> network(world);

  // Factor processors into a processor grid
  int ipx, ipy, pdx, pdy;
  factor_grid(nprocs, XDIM, YDIM, &pdx, &pdy);
  if (me == 0) {
    printf("\nProcessor configuration is %d X %d\n",pdx,pdy);
  }
  ipx = me%pdx;
  ipy = (me-ipx)/pdx;

  int ixmin, ixmax, iymin, iymax; // bounds of locally owned nodes
  int iaxmin, iaxmax, iaymin, iaymax; // bounds of locally held nodes
  ixmin = static_cast<int>((static_cast<double>(ipx*XDIM))/(static_cast<double>(pdx)));
  ixmax = static_cast<int>((static_cast<double>((ipx+1)*XDIM))/(static_cast<double>(pdx)))-1;
  iymin = static_cast<int>((static_cast<double>(ipy*YDIM))/(static_cast<double>(pdy)));
  iymax = static_cast<int>((static_cast<double>((ipy+1)*YDIM))/(static_cast<double>(pdy)))-1;

  iaxmin = ixmin - 1;
  if (ixmin == 0) iaxmin = 0;
  iaxmax = ixmax + 1;
  if (ixmax == XDIM - 1) iaxmax = XDIM - 1;

  iaymin = iymin - 1;
  if (iymin == 0) iaymin = 0;
  iaymax = iymax + 1;
  if (iymax == YDIM - 1) iaymax = YDIM - 1;

  // Add buses to network
  int ncnt, n, ix, iy, nx, ny, i, j;
  ncnt = 0;
  nx = iaxmax - iaxmin + 1;
  ny = iaymax - iaymin + 1;
  for (j=0; j<ny; j++) {
    iy = j + iaymin;
    for (i=0; i<nx; i++) {
      ix = i + iaxmin;
      n = iy*XDIM + ix;
      n = 2*n;  // Provide original index that is not equal to global index 
      network.addBus(n);
      // Set active flag for network buses
      if (ix >= ixmin && ix <= ixmax && iy >= iymin && iy <= iymax) {
        network.setActiveBus(ncnt, true);
      } else {
        network.setActiveBus(ncnt, false);
      }
      n = n/2;
      network.setGlobalBusIndex(ncnt, n);
      if (ix == 0 && iy == 0) {
        network.setReferenceBus(ncnt);
      }
      ncnt++;
    }
  }

  // Add branches to network. Start with branches connecting buses in the
  // i-direction
  int n1, n2, lx, ly, bridx(0);
  ncnt = 0;
  nx = iaxmax - iaxmin;
  ny = iymax - iymin + 1;
  for (j=0; j<ny; j++) {
    iy = j + iymin;
    for (i=0; i<nx; i++) {
      ix = i + iaxmin;
      n1 = iy*XDIM+ix;
      n1 = 2*n1;
      n2 = iy*XDIM+ix+1;
      n2 = 2*n2;
      network.addBranch(n1, n2);
      n1 = n1/2;
      n2 = n2/2;
      network.setGlobalBusIndex1(ncnt, n1);
      network.setGlobalBusIndex2(ncnt, n2);
      n = iy*(XDIM-1) + ix;
      network.setGlobalBranchIndex(ncnt,n);
      // Figure out local indices of buses
      lx = ix - iaxmin;
      ly = iy - iaymin;
      n1 = ly*(iaxmax-iaxmin+1) + lx;
      n2 = ly*(iaxmax-iaxmin+1) + lx + 1;
      network.setLocalBusIndex1(ncnt,n1);
      network.setLocalBusIndex2(ncnt,n2);
      // Determine which branches are locally held. Use the rule that if bus 1
      // is local, then branch belongs to this processor
      if (ix >= ixmin && ix <= ixmax && iy >= iymin && iy <= iymax) {
        network.setActiveBranch(ncnt, true);
      } else {
        network.setActiveBranch(ncnt, false);
      }
      ncnt++;
    }
  }
  // Add branches connecting buses in the j-direction
  nx = ixmax - ixmin + 1;
  ny = iaymax - iaymin;
  for (j=0; j<ny; j++) {
    iy = j + iaymin;
    for (i=0; i<nx; i++) {
      ix = i + ixmin;
      n1 = iy*XDIM+ix;
      n1 = 2*n1;
      n2 = (iy+1)*XDIM+ix;
      n2 = 2*n2;
      network.addBranch(n1, n2);
      n1 = n1/2;
      n2 = n2/2;
      network.setGlobalBusIndex1(ncnt, n1);
      network.setGlobalBusIndex2(ncnt, n2);
      n = iy*XDIM + ix + (XDIM-1)*YDIM;
      network.setGlobalBranchIndex(ncnt,n);
      // Figure out local indices of buses
      lx = ix - iaxmin;
      ly = iy - iaymin;
      n1 = ly*(iaxmax-iaxmin+1) + lx;
      n2 = (ly+1)*(iaxmax-iaxmin+1) + lx;
      network.setLocalBusIndex1(ncnt,n1);
      network.setLocalBusIndex2(ncnt,n2);
      // Determine which branches are locally held. Use the rule that if bus 1
      // is local, then branch belongs to this processor
      if (ix >= ixmin && ix <= ixmax && iy >= iymin && iy <= iymax) {
        network.setActiveBranch(ncnt, true);
      } else {
        network.setActiveBranch(ncnt, false);
      }
      ncnt++;
    }
  }

  // Check that number of buses and branches match expected number of buses and
  // branches
  n = (iaxmax-iaxmin+1)*(iaymax-iaymin+1);
  int oks, okr;
  bool ok = true;
  if (network.numBuses() != n) {
    printf("p[%d] Number of buses: %d expected: %d\n",me,network.numBuses(),n);
    ok = false;
  } 
  BOOST_CHECK_EQUAL(network.numBuses(), n);
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nNumber of buses on each process ok\n");
  }
  BOOST_CHECK(ok);
  ok = true;
  n = network.totalBuses();
  ncnt = XDIM*YDIM;
  if (n != ncnt) {
    printf("p[%d] Total number of buses: %d expected: %d\n",me,n,ncnt);
    ok = false;
  }
  BOOST_CHECK_EQUAL(network.totalBuses(), n);
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nTotal number of buses ok\n");
  }
  BOOST_CHECK(ok);
  ok = true;
  n = (iaxmax-iaxmin)*(iymax-iymin+1)+(ixmax-ixmin+1)*(iaymax-iaymin);
  if (network.numBranches() != n) {
    printf("p[%d] Number of branches: %d expected: %d\n",me,network.numBranches(),n);
    ok = false;
  }
  BOOST_CHECK_EQUAL(network.numBranches(), n);
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nNumber of branches on each process ok\n");
  }
  BOOST_CHECK(ok);
  ok = true;
  n = network.totalBranches();
  ncnt = (XDIM-1)*YDIM+XDIM*(YDIM-1);
  if (n != ncnt) {
    printf("p[%d] Total number of branches: %d expected: %d\n",me,n,ncnt);
    ok = false;
  }
  BOOST_CHECK_EQUAL(n, ncnt);
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nTotal number of branches ok\n");
  }
  BOOST_CHECK(ok);

  // Test location of reference bus
  n = network.getReferenceBus();
  if (!(me == 0 && n == 0) && !(me != 0 && n == -1)) {
    printf("p[%d] Reference bus error: %d\n",me,n);
  } else if (me == 0 && n == 0) {
    printf("\nReference bus ok\n");
  }
  BOOST_CHECK((me == 0 && n == 0) || (me != 0 && n == -1));

  // Set up number of branches attached to bus
  int nbus = network.numBuses();
  for (i=0; i<nbus; i++) {
    ok = network.clearBranchNeighbors(i);
    if (!ok) {
      printf("p[%d] clearBranchNeighbors failed for bus %d\n",me,i);
    }
    BOOST_CHECK(ok);
  }
  // Loop over all branches
  int nbranch = network.numBranches();
  for (i=0; i<nbranch; i++) {
    network.getBranchEndpoints(i, &n1, &n2);
    if (network.getActiveBus(n1) || network.getActiveBus(n2)) {
      ok = network.addBranchNeighbor(n1, i);
      if (!ok) {
	printf("p[%d] addBranchNeighbor failed for bus %d\n",me,n1);
      }
      BOOST_CHECK(ok);
      ok = network.addBranchNeighbor(n2, i);
      if (!ok) {
	printf("p[%d] addBranchNeighbor failed for bus %d\n",me,n2);
      }
      BOOST_CHECK(ok);
    }
  }

  // Test active buses
  int ldx = iaxmax-iaxmin+1;
  ok = true;
  for (i=0; i<nbus; i++) {
    ix = i%ldx;
    iy = (i-ix)/ldx;
    ix = ix + iaxmin;
    iy = iy + iaymin;
    if (ix >= ixmin && ix <= ixmax && iy >= iymin && iy <= iymax) {
      if (!network.getActiveBus(i)) {
        printf("p[%d] inactive bus error %d\n",me,i);
        ok = false;
      }
    } else {
      if (network.getActiveBus(i)) {
        printf("p[%d] active bus error %d\n",me,i);
        ok = false;
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nActive bus settings ok\n");
  }
  BOOST_CHECK(ok);

  // Test active branches
  ok = true;
  for (i=0; i<nbranch; i++) {
    network.getBranchEndpoints(i, &n1, &n2);
    ix = n1%ldx;
    iy = (n1-ix)/ldx;
    ix = ix + iaxmin;
    iy = iy + iaymin;
    // If bus 1 is local, then branch is local
    if (ix >= ixmin && ix <= ixmax && iy >= iymin && iy <= iymax) {
      if (!network.getActiveBranch(i)) {
        printf("p[%d] inactive branch error %d\n",me,i);
        ok = false;
      }
    } else {
      if (network.getActiveBranch(i)) {
        printf("p[%d] active branch error %d\n",me,i);
        ok = false;
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nActive branch settings ok\n");
  }
  BOOST_CHECK(ok);
  
  // check neighbors of buses
  ok = true;
  for (i=0; i<nbus; i++) {
    if (network.getActiveBus(i)) {
      ix = i%ldx;
      iy = (i-ix)/ldx;
      ix = ix + iaxmin;
      iy = iy + iaymin;
      n = 0;
      if (ix > iaxmin) n++;
      if (ix < iaxmax) n++;
      if (iy > iaymin) n++;
      if (iy < iaymax) n++;
      std::vector<int> branches = network.getConnectedBranches(i);
      std::map<int,int> checkBuses;
      if (n != branches.size()) {
        printf("p[%d] incorrect neighbor branches on bus %d\n",me,i);
        ok = false;
      }
      n = branches.size();
      for (j=0; j<n; j++) {
        network.getBranchEndpoints(branches[j], &n1, &n2);
        if (n1 != i && n2 != i) {
          printf("p[%d] incorrectly assigned branches on bus %d\n",me,i);
          ok = false;
        }
        if (n1 != i) {
          checkBuses.insert(std::pair<int, int>(n1,j));
        } else {
          checkBuses.insert(std::pair<int, int>(n2,j));
        }
      }
      std::vector<int> buses = network.getConnectedBuses(i);
      if (buses.size() != branches.size()) {
        printf("p[%d] incorrect neighbor buses on bus %d\n",me,i);
        ok = false;
      }
      n = buses.size();
      for (j=0; j<n; j++) {
        if (checkBuses.find(buses[j]) == checkBuses.end()) {
          printf("p[%d] incorrectly assigned buses on bus %d\n",me,i);
          ok = false;
        }
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nBus neighbors are ok\n");
  }
  BOOST_CHECK(ok);

  // Test clone operation
  bool test_clone = true;
  if (test_clone) {
    boost::shared_ptr<gridpack::network::BaseNetwork<TestBus, TestBranch> >
      network2(new gridpack::network::BaseNetwork<TestBus, TestBranch> (world));
    network.clone<TestBus, TestBranch>(network2);
    ok = true;
    n = network2->numBuses();
    if (n != nbus) {
      printf("number of buses in clone %d does not match original %d\n",n,nbus);
      ok = false;
    }
    for (i=0; i<nbus; i++) {
      if (network.getOriginalBusIndex(i) != network2->getOriginalBusIndex(i))
        ok = false;
      if (network.getGlobalBusIndex(i) != network2->getGlobalBusIndex(i))
        ok = false;
      std::vector<int> buses = network.getConnectedBuses(i);
      std::vector<int> buses2 = network2->getConnectedBuses(i);
      if (network.getActiveBus(i) != network2->getActiveBus(i)) ok = false;
      if (buses.size() != buses2.size()) ok = false;
      for (j=0; j<buses.size(); j++) {
        if (buses[j] != buses2[j]) ok = false;
      }
    }
    if (network.getReferenceBus() != network2->getReferenceBus()) ok = false;
    n = network2->numBranches();
    if (n != nbranch) {
      printf("number of branches in clone %d does not match original %d\n",
          n,nbranch);
      ok = false;
    }
    for (i=0; i<nbranch; i++) {
      if (network.getGlobalBranchIndex(i) != network2->getGlobalBranchIndex(i))
        ok = false;
      int idx1, idx2, jdx1, jdx2;
      network.getOriginalBranchEndpoints(i,&idx1,&jdx1);
      network2->getOriginalBranchEndpoints(i,&idx2,&jdx2);
      if (idx1 != idx2 || jdx1 != jdx2) ok = false;
      if (network.getGlobalBranchIndex(i) != network2->getGlobalBranchIndex(i))
        ok = false;
      if (network.getActiveBranch(i) != network2->getActiveBranch(i)) ok = false;
      network.getBranchEndpoints(i,&idx1,&jdx1);
      network2->getBranchEndpoints(i,&idx2,&jdx2);
      if (idx1 != idx2 || jdx1 != jdx2) ok = false;
      if (network.getGlobalBusIndex(idx1)
          != network2->getGlobalBusIndex(idx2)) ok = false;
      if (network.getGlobalBusIndex(jdx1)
          != network2->getGlobalBusIndex(jdx2)) ok = false;
    }
    oks = (int)ok;
    ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
    ok = (bool)okr;
    if (me == 0 && ok) {
      printf("\nNetwork clone function is ok\n");
    } else if (me == 0) {
      printf("\nNetwork clone function failed\n");
    }
    BOOST_CHECK(ok);
  }
  
  // Test map functions
  network.setMap();
  ok = true;
  std::vector<int> originalBus;
  for (i=0; i<nbus; i++) {
    originalBus.push_back(network.getOriginalBusIndex(i));
  }
  for (i=0; i<nbus; i++) {
    std::vector<int> localBus;
    localBus = network.getLocalBusIndices(originalBus[i]);
    bool found = false;
    for (j=0; j<localBus.size(); j++) {
      if (localBus[j] == i) found = true;
    }
    if (!found) ok = false;
  }
  std::vector<std::pair<int,int> > originalBranch;
  
  for (i=0; i<nbranch; i++) {
    int idx1, idx2;
    network.getOriginalBranchEndpoints(i,&idx1,&idx2);
    originalBranch.push_back(std::pair<int,int>(idx1,idx2));
  }
  for (i=0; i<nbranch; i++) {
    std::vector<int> localBranch;
    localBranch = network.getLocalBranchIndices(originalBranch[i].first,
        originalBranch[i].second);
    bool found = false;
    for (j=0; j<localBranch.size(); j++) {
      if (localBranch[j] == i) found = true;
    }
    if (!found) ok = false;
  }

  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nMap functions are ok\n");
  }
  BOOST_CHECK(ok);

  // Test ghost update operations. Start by allocating exchange buffers and
  // assigning values to them
  network.allocXCBus(sizeof(int));
  network.allocXCBranch(sizeof(int));
  int *iptr;

  for (i=0; i<nbus; i++) {
    iptr = (int*)network.getXCBusBuffer(i);
    if (iptr) {
      if (network.getActiveBus(i)) {
        *iptr = network.getGlobalBusIndex(i);
      } else {
        *iptr = -1;
      }
    } else {
      printf("p[%d] null iptr at 1: %d\n",me,i);
    }
    // printf("p[%d] iptr1: %d\n",me,*iptr);
  }
  for (i=0; i<nbranch; i++) {
    iptr = (int*)network.getXCBranchBuffer(i);
    if (network.getActiveBranch(i)) {
      *iptr = network.getGlobalBranchIndex(i);
    } else {
      *iptr = -1;
    }
  }
  network.initBusUpdate();
  network.initBranchUpdate();

  network.updateBuses();
  network.updateBranches();

  ok = true;
  for (i=0; i<nbus; i++) {
    iptr = (int*)network.getXCBusBuffer(i);
  //  printf("p[%d] iptr2: %d global: %d\n",me,*iptr,network.getGlobalBusIndex(i));
    if (!network.getActiveBus(i)) {
      if (*iptr != network.getGlobalBusIndex(i)) {
        ok = false;
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nBus update ok\n");
  } else if (!ok) {
    printf("\nMismatched bus update on %d\n",me);
  }
  BOOST_CHECK(ok);
  
  ok = true;
  for (i=0; i<nbranch; i++) {
    iptr = (int*)network.getXCBranchBuffer(i);
    if (!network.getActiveBranch(i)) {
      if (*iptr != network.getGlobalBranchIndex(i)) {
        ok = false;
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nBranch update ok\n");
  } else if (!ok) {
    printf("\nMismatched branch update on %d\n",me);
  }
  BOOST_CHECK(ok);

  network.freeXCBus();
  network.freeXCBranch();

  // Test clean function
  network.clean();
  // Check that total number of remaining buses and branches are as expected
  n = (ixmax-ixmin+1)*(iymax-iymin+1);
  ok = true;
  if (network.numBuses() != n) {
    printf("p[%d] Number of buses after clean: %d expected: %d\n",me,network.numBuses(),n);
    ok = false;
  } 
  oks = n;
  ierr = MPI_Allreduce(&oks, &n, 1, MPI_INT, MPI_SUM, mpi_world);
  if (n != XDIM*YDIM) {
    ok = false;
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nNumber of buses after clean ok\n");
  }
  BOOST_CHECK(ok);
  ok = true;
  n = (iaxmax-ixmin)*(iymax-iymin+1)+(ixmax-ixmin+1)*(iaymax-iymin);
  if (network.numBranches() != n) {
    printf("p[%d] Number of branches after clean: %d expected: %d\n",me,network.numBranches(),n);
    ok = false;
  }
  oks = n;
  ierr = MPI_Allreduce(&oks, &n, 1, MPI_INT, MPI_SUM, mpi_world);
  if (n != (XDIM-1)*YDIM+XDIM*(YDIM-1)) {
    ok = false;
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nNumber of branches after clean ok\n");
  }
  BOOST_CHECK(ok);

  // check neighbors of buses after performing clean operation
  ok = true;
  nbus = network.numBuses();
  ldx = ixmax - ixmin + 1;
  for (i=0; i<nbus; i++) {
    if (network.getActiveBus(i)) {
      ix = i%ldx;
      iy = (i-ix)/ldx;
      ix = ix + ixmin;
      iy = iy + iymin;
      n = 0;
      if (ix > ixmin) n++;
      if (ix < iaxmax) n++;
      if (iy > iymin) n++;
      if (iy < iaymax) n++;
      std::vector<int> branches = network.getConnectedBranches(i);
      std::map<int,int> checkBuses;
      if (n != branches.size()) {
        printf("p[%d] incorrect neighbor branches expected: %d actual: %d  on bus %d\n",
               me,n,static_cast<int>(branches.size()),i);
        ok = false;
      }
      n = branches.size();
      for (j=0; j<n; j++) {
        network.getBranchEndpoints(branches[j], &n1, &n2);
        if (n1 != i && n2 != i) {
          printf("p[%d] incorrectly assigned branches on bus %d\n",me,i);
          ok = false;
        }
        if (n1 != i) {
          checkBuses.insert(std::pair<int, int>(n1,j));
        } else {
          checkBuses.insert(std::pair<int, int>(n2,j));
        }
      }
      std::vector<int> buses = network.getConnectedBuses(i);
      if (buses.size() != branches.size()) {
        printf("p[%d] incorrect neighbor buses on bus %d\n",me,i);
        ok = false;
      }
      n = buses.size();
      for (j=0; j<n; j++) {
        if (checkBuses.find(buses[j]) == checkBuses.end()) {
          printf("p[%d] incorrectly assigned buses on bus %d\n",me,i);
          ok = false;
        }
      }
    }
  }
  oks = (int)ok;
  ierr = MPI_Allreduce(&oks, &okr, 1, MPI_INT, MPI_PROD, mpi_world);
  ok = (bool)okr;
  if (me == 0 && ok) {
    printf("\nBuses and branches are ok after clean operation\n");
  }
  BOOST_CHECK(ok);
  
#if 0
  // Partition network using partitioner
  network.partition();
  if (me == 0 && ok) {
    printf("\nCompleted partitioning of network\n");
  }
  network.writeGraph("test.dot");
#endif
}

BOOST_AUTO_TEST_SUITE_END( )

bool init_function(void)
{
  return true;
}

int main (int argc, char **argv) {

  gridpack::Environment env(argc, argv);
  gridpack::parallel::Communicator world;

  int me = world.rank();
  if (me == 0) {
    printf("Testing Network Module\n");
    printf("\nTest Network is %d X %d\n",XDIM,YDIM);
  }

  int result = ::boost::unit_test::unit_test_main( &init_function, argc, argv );
  return result;
}
