/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   adjacency_list.cpp
 * @author William A. Perkins
 * @date   2014-02-26 12:56:16 d3g096
 * 
 * @brief  Implementation of AdjacencyList
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created June 17, 2013 by William A. Perkins
// Last Change: 2013-05-03 12:23:12 d3g096
// -------------------------------------------------------------

#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <boost/assert.hpp>
#include <boost/mpi/collectives.hpp>
#include <ga.h>
#include "adjacency_list.hpp"
#include "gridpack/utilities/exception.hpp"

namespace gridpack {
namespace network {

// -------------------------------------------------------------
//  class AdjacencyList
// -------------------------------------------------------------

// -------------------------------------------------------------
// AdjacencyList static members
// -------------------------------------------------------------
const AdjacencyList::Index AdjacencyList::bogus(-1);

// -------------------------------------------------------------
// AdjacencyList:: constructors / destructor
// -------------------------------------------------------------
AdjacencyList::AdjacencyList(const parallel::Communicator& comm)
  : parallel::Distributed(comm),
    utility::Uncopyable(),
    p_global_nodes(), p_original_nodes(), p_edges(), p_adjacency()
{
  // empty
}

AdjacencyList::AdjacencyList(const parallel::Communicator& comm,
                             const int& local_nodes, const int& local_edges)
  : parallel::Distributed(comm),
    utility::Uncopyable(),
    p_global_nodes(), p_original_nodes(), p_edges(), p_adjacency()
{
  p_global_nodes.reserve(local_nodes);
  p_original_nodes.reserve(local_nodes);
  p_edges.reserve(local_edges);
  p_adjacency.reserve(local_nodes);
}

AdjacencyList::~AdjacencyList(void)
{
  // empty
}

// -------------------------------------------------------------
// AdjacencyList::node_index
// -------------------------------------------------------------
AdjacencyList::Index 
AdjacencyList::node_index(const int& local_index) const
{
  BOOST_ASSERT(local_index < this->nodes());
  return p_global_nodes[local_index];
}

// -------------------------------------------------------------
// AdjacencyList::edge_index
// -------------------------------------------------------------
AdjacencyList::Index 
AdjacencyList::edge_index(const int& local_index) const
{
  BOOST_ASSERT(local_index < this->edges());
  return p_edges[local_index].index;
}

// -------------------------------------------------------------
// AdjacencyList::edge
// -------------------------------------------------------------
void
AdjacencyList::edge(const int& local_index, Index& node1, Index& node2) const
{
  BOOST_ASSERT(local_index < this->edges());
  node1 = p_edges[local_index].global_conn.first;
  node2 = p_edges[local_index].global_conn.second;
}

// -------------------------------------------------------------
// AdjacencyList::ready
// -------------------------------------------------------------
void
AdjacencyList::ready(void)
{
#if 1
  int grp = this->communicator().getGroup();
  int me = GA_Pgroup_nodeid(grp);
  int nprocs = GA_Pgroup_nnodes(grp);
  p_adjacency.clear();
  p_adjacency.resize(p_global_nodes.size());

  // Find total number of nodes and edges. Assume no duplicates
  int nedges = p_edges.size();
  int total_edges = nedges;
  char plus[2];
  strcpy(plus,"+");
  GA_Pgroup_igop(grp,&total_edges, 1, plus);
  int nnodes = p_original_nodes.size();
  int total_nodes = nnodes;
  GA_Pgroup_igop(grp,&total_nodes, 1, plus);

  // Create a global array containing original indices of all nodes and indexed
  // by the global index of the node
  int i, p;
  std::vector<int> dist(nprocs);
  for (p=0; p<nprocs; p++) {
    dist[p] = 0;
  }
  dist[me] = nnodes;
  GA_Pgroup_igop(grp,&dist[0],nprocs,plus);
  int *mapc = new int[nprocs+1];
  mapc[0] = 0;
  for (p=1; p<nprocs; p++) {
    mapc[p] = mapc[p-1] + dist[p-1];
  }
  mapc[nprocs] = total_nodes;
  int g_nodes = GA_Create_handle();
  int dims = total_nodes;
  NGA_Set_data(g_nodes,1,&dims,C_INT);
  NGA_Set_pgroup(g_nodes, grp);
  if (!GA_Allocate(g_nodes)) {
    char buf[256];
    sprintf(buf,"AdjacencyList::ready: Unable to allocate distributed array"
        " for bus indices\n");
    printf("%s",buf);
    throw gridpack::Exception(buf);
  }
  int lo, hi;
  lo = mapc[me];
  hi = mapc[me+1]-1;
  int size = hi - lo + 1;
  std::vector<int> o_idx(size), g_idx(size);
  for (i=0; i<size; i++) o_idx[i] = p_original_nodes[i]; 
  for (i=0; i<size; i++) g_idx[i] = p_global_nodes[i]; 
  int **indices= new int*[size];
  int *iptr = &g_idx[0];
  for (i=0; i<size; i++) {
    indices[i] = iptr;
    iptr++;
  }
  if (size > 0) NGA_Scatter(g_nodes,&o_idx[0],indices,size);
  GA_Pgroup_sync(grp);
  delete [] indices;
  delete [] mapc;

  // Cycle through all nodes and match them up with nodes at end of edges.
  for (p=0; p<nprocs; p++) {
    int iproc = (me+p)%nprocs;
    // Get node data from process iproc
    NGA_Distribution(g_nodes,iproc,&lo,&hi);
    size = hi - lo + 1;
    if (size <= 0) continue;
    int *buf = new int[size];
    int ld = 1;
    NGA_Get(g_nodes,&lo,&hi,buf,&ld);
    // Create a map of the nodes from process p
    std::map<int,int> nmap;
    std::map<int,int>::iterator it;
    std::pair<int,int> pr;
    for (i=lo; i<=hi; i++){
      pr = std::pair<int,int>(buf[i-lo],i);
      nmap.insert(pr);
    }
    delete [] buf;
    // scan through the edges looking for matches. If there is a match, set the
    // global index
    int idx;
    for (i=0; i<nedges; i++) {
      idx = static_cast<int>(p_edges[i].original_conn.first);
      it = nmap.find(idx);
      if (it != nmap.end()) {
        p_edges[i].global_conn.first = static_cast<Index>(it->second);
      }
      idx = static_cast<int>(p_edges[i].original_conn.second);
      it = nmap.find(idx);
      if (it != nmap.end()) {
        p_edges[i].global_conn.second = static_cast<Index>(it->second);
      }
    }
  }
  GA_Destroy(g_nodes);

  // All edges now have global indices assigned to them. Begin constructing
  // adjacency list. Start by creating a global array containing all edges
  dist[0] = 0;
  for (p=1; p<nprocs; p++) {
    double max = static_cast<double>(total_edges);
    max = (static_cast<double>(p))*(max/(static_cast<double>(nprocs)));
    dist[p] = 2*(static_cast<int>(max));
  }
  int g_edges = GA_Create_handle();
  dims = 2*total_edges;
  NGA_Set_data(g_edges,1,&dims,C_INT);
  NGA_Set_irreg_distr(g_edges,&dist[0],&nprocs);
  NGA_Set_pgroup(g_edges, grp);
  if (!GA_Allocate(g_edges)) {
    char buf[256];
    sprintf(buf,"AdjacencyList::ready: Unable to allocate distributed array"
        " for branch indices\n");
    printf("%s",buf);
    throw gridpack::Exception(buf);
  }

  // Add edge information to global array. Start by figuring out how much data
  // is associated with each process
  for (p=0; p<nprocs; p++) {
    dist[p] = 0;
  }
  dist[me] = nedges;
  GA_Pgroup_igop(grp,&dist[0], nprocs, plus);
  std::vector<int> offset(nprocs);
  offset[0] = 0;
  for (p=1; p<nprocs; p++) {
    offset[p] = offset[p-1] + 2*dist[p-1];
  }
  // Figure out where local data goes in GA and then copy it to GA
  lo = offset[me];
  hi = lo + 2*nedges - 1;
  std::vector<int> edge_ids(2*nedges);
  for (i=0; i<nedges; i++) {
    edge_ids[2*i] = static_cast<int>(p_edges[i].global_conn.first);
    edge_ids[2*i+1] = static_cast<int>(p_edges[i].global_conn.second);
  }
  if (lo <= hi) {
    int ld = 1;
    NGA_Put(g_edges,&lo,&hi,&edge_ids[0],&ld);
  }
  GA_Pgroup_sync(grp);

  // Cycle through all edges and find out how many are attached to the nodes on
  // your process. Start by creating a map between the global node indices and
  // the local node indices
  std::map<int,int> gmap;
  std::map<int,int>::iterator it;
  std::pair<int,int> pr;
  for (i=0; i<nnodes; i++){
    pr = std::pair<int,int>(static_cast<int>(p_global_nodes[i]),i);
    gmap.insert(pr);
  }
  // Cycle through edge information on each processor
  for (p=0; p<nprocs; p++) {
    int iproc = (me+p)%nprocs;
    NGA_Distribution(g_edges,iproc,&lo,&hi);
    int size = hi - lo + 1;
    int *buf = new int[size];
    int ld = 1;
    NGA_Get(g_edges,&lo,&hi,buf,&ld);
    BOOST_ASSERT(size%2 == 0);
    size = size/2;
    int idx1, idx2;
    Index idx;
    for (i=0; i<size; i++) {
      idx1 = buf[2*i];
      idx2 = buf[2*i+1];
      it = gmap.find(idx1);
      if (it != gmap.end()) {
        idx = static_cast<Index>(idx2);
        p_adjacency[it->second].push_back(idx);
      }
      it = gmap.find(idx2);
      if (it != gmap.end()) {
        idx = static_cast<Index>(idx1);
        p_adjacency[it->second].push_back(idx);
      }
    }
    delete [] buf;
  }
  GA_Destroy(g_edges);
  GA_Pgroup_sync(grp);
#else
  int me(this->processor_rank());
  int nproc(this->processor_size());

  p_adjacency.clear();
  p_adjacency.resize(p_nodes.size());

  IndexVector current_indexes;
  IndexVector connected_indexes;

  for (int p = 0; p < nproc; ++p) {

    // broadcast the node indexes owned by process p to all processes,
    // all processes work on these at once

    current_indexes.clear();
    if (me == p) {
      std::copy(p_nodes.begin(), p_nodes.end(), 
	  std::back_inserter(current_indexes));
      // std::cout << me << ": node indexes: ";
      // std::copy(current_indexes.begin(), current_indexes.end(),
      //           std::ostream_iterator<Index>(std::cout, ","));
      // std::cout << std::endl;
    }
    boost::mpi::broadcast(this->communicator(), current_indexes, p);

    // make a copy of the local edges in a list (so it's easier to
    // remove those completely accounted for)
    std::list<p_Edge> tmpedges;
    std::copy(p_edges.begin(), p_edges.end(), 
	std::back_inserter(tmpedges));

    // loop over the process p's node index set

    int local_index(0);
    for (IndexVector::iterator n = current_indexes.begin(); 
	n != current_indexes.end(); ++n, ++local_index) {

      // determine the local edges that refer to the current node index

      connected_indexes.clear();
      std::list<p_Edge>::iterator e(tmpedges.begin());
      //      std::cout << me << ": current node index: " << *n 
      //                << ", edges: " << tmpedges.size() 
      //                << std::endl;

      while (e != tmpedges.end()) {
	if (*n == e->conn.first && e->conn.second != bogus) {
	  connected_indexes.push_back(e->conn.second);
	  e->found.first = true;
	  // std::cout << me << ": found connection: edge " << e->index
	  //           << " (" << e->conn.first << ", " << e->conn.second << ")"
	  //           << std::endl;
	}
	if (*n == e->conn.second && e->conn.first != bogus) {
	  connected_indexes.push_back(e->conn.first);
	  e->found.second = true;
	  // std::cout << me << ": found connection: edge " << e->index
	  //           << " (" << e->conn.first << ", " << e->conn.second << ")"
	  //           << std::endl;
	}

	if (e->found.first && e->found.second) {
	  e = tmpedges.erase(e);
	} else if (e->conn.first == bogus || 
	    e->conn.second == bogus) {
	  e = tmpedges.erase(e);
	} else {
	  ++e;
	}
      }

      // gather all connections for the current node index to the
      // node's owner process, we have to gather the vectors because
      // processes will have different numbers of connections

      if (me == p) {
	size_t allsize;
        boost::mpi::reduce(this->communicator(), 
                           connected_indexes.size(), allsize, std::plus<size_t>(), p);

	std::vector<IndexVector> all_connected_indexes;
        boost::mpi::gather(this->communicator(), 
                           connected_indexes, all_connected_indexes, p);
	p_adjacency[local_index].clear();
	for (std::vector<IndexVector>::iterator k = all_connected_indexes.begin();
	    k != all_connected_indexes.end(); ++k) {
	  std::copy(k->begin(), k->end(), 
	      std::back_inserter(p_adjacency[local_index]));
	}
      } else {
	boost::mpi::reduce(this->communicator(), 
                           connected_indexes.size(), std::plus<size_t>(), p);
	boost::mpi::gather(this->communicator(), connected_indexes, p);
      }
      this->communicator().barrier();
    }
    this->communicator().barrier();
  }
#endif
}

// -------------------------------------------------------------
// AdjacencyList::node_neighbors
// -------------------------------------------------------------
size_t 
AdjacencyList::node_neighbors(const int& local_index) const
{
  BOOST_ASSERT(local_index < p_adjacency.size());
  return p_adjacency[local_index].size();
}

void
AdjacencyList::node_neighbors(const int& local_index,
                              IndexVector& global_neighbor_indexes) const
{
  BOOST_ASSERT(local_index < p_adjacency.size());
  global_neighbor_indexes.clear();
  std::copy(p_adjacency[local_index].begin(), p_adjacency[local_index].end(),
            std::back_inserter(global_neighbor_indexes));

}


} // namespace network
} // namespace gridpack
