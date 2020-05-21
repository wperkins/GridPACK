/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/**
 * @file   graph_partitioner.hpp
 * @author William A. Perkins
 * @date   2013-09-06 13:35:16 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _graph_partitioner_hpp_
#define _graph_partitioner_hpp_

#include <boost/scoped_ptr.hpp>
#include <gridpack/partition/graph_partitioner_implementation.hpp>

namespace gridpack {
namespace network {

// -------------------------------------------------------------
//  class GraphPartitioner
// -------------------------------------------------------------
/// A class that serves as an interface to a graph partitioning library
class GraphPartitioner 
  : public parallel::WrappedDistributed,
    private utility::Uncopyable 
{
public:

  typedef GraphPartitionerImplementation::Index Index;
  typedef GraphPartitionerImplementation::IndexVector IndexVector;
  typedef GraphPartitionerImplementation::MultiIndexVector MultiIndexVector;

  /// Default constructor.
  GraphPartitioner(const parallel::Communicator& comm);

  /// Construct w/ known local sizes  (guesses to size containers, maybe)
  GraphPartitioner(const parallel::Communicator& comm,
                   const int& local_nodes, const int& local_edges);

  /// Destructor
  ~GraphPartitioner(void);

  /// Add the global index of a local node and the original index of local node
  void add_node(const Index& global_index, const Index& original_index)
  {
    p_impl->add_node(global_index, original_index);
  }
  
  /// Add the global index of a local edge and what it connects using the original
  /// indices of the buses at either end of the node 
  void add_edge(const Index& edge_index, 
                const Index& node_index_1,
                const Index& node_index_2)
  {
    p_impl->add_edge(edge_index, node_index_1, node_index_2);
  }

  /// Get the global indices of the buses at either end of a branch
  void get_global_edge_ids(int idx, Index *node_index_1, Index *node_index_2) const
  {
    p_impl->get_global_edge_ids(idx, node_index_1, node_index_2);
  }

  /// Get the number of local nodes
  size_t nodes(void) const
  {
    return p_impl->nodes();
  }

  /// Get the global node index given a local index
  Index node_index(const int& local_index) const
  {
    return p_impl->node_index(local_index);
  }

  /// Get the number of local edges
  size_t edges(void) const
  {
    return p_impl->edges();
  }

  /// Get the global edge index given a local index
  Index edge_index(const int& local_index) const
  {
    return p_impl->edge_index(local_index);
  }

  /// Partition the graph
  void partition(void)
  {
    p_impl->partition();
  }

  /// Get the node destinations
  void node_destinations(IndexVector& dest) const
  {
    p_impl->node_destinations(dest);
  }

  /// Get the edge destinations
  void edge_destinations(IndexVector& dest) const
  {
    p_impl->edge_destinations(dest);
  }

  /// Get the destinations of ghosted nodes
  void ghost_node_destinations(MultiIndexVector& dest) const
  {
    p_impl->ghost_node_destinations(dest);
  }

  /// Get the destinations of ghosted edge
  void ghost_edge_destinations(IndexVector& dest) const
  {
    p_impl->ghost_edge_destinations(dest);
  }

protected:

  /// The actual implementation
  boost::scoped_ptr<GraphPartitionerImplementation> p_impl;
  
};


} // namespace network
} // namespace gridpack


#endif
