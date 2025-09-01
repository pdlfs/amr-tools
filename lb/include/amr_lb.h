#pragma once

#include <mpi.h>

#include <string>
#include <vector>

namespace amr {
namespace lb {
//
// PlacementArgs: minimal placement inputs and outputs
// Valid placement policy names:
// - "actual": noop placement, used in sim to obtain real placement from trace
// - "baseline": contiguous placement, assuming unit cost
// - "lpt": Longest Processing Time
// - "cdp": Contiguous-DP
// - "cdpi50": CDP + iterative improvements, not used in final runs
// - "cdpi250": CDP + iterative improvements, not used in final runs
// - "hybrid<X>": CPLX, internal name (X in 0-100). E.g. hybrid50
// - "cdpc<C>par<P>": CDP-Chunked for higher parallelism
//   C: chunk size (512), P: parallelism (8 for 4096 ranks)
//
// See kPolicyMap in `src/policy_utils.cc` for more
//
struct PlacementArgs {
  std::string policy_name;             // preconfigured policy name
  std::vector<double> const &costlist; // size assumed to be nblocks
  std::vector<int> &ranklist;          // will be resized to nblocks
  int nranks;                          // number of policy ranks
};

//
// PlacementArgsMpi: PlacementArgs + MPI info for parallel placement
// (MPI  ranks here is separate from stdargs ranks, to decouple input and exec)
// (But this may not be implemented all the way)
//
struct PlacementArgsMpi {
  PlacementArgs stdargs; // std args
  int my_rank;           // MPI rank
  int nranks;            // number of MPI ranks
  MPI_Comm comm;         // MPI communicator
};

//
// LoadBalance: top-level interface for placement
//
class LoadBalance {
public:
  //
  // AssignBlocks: assign blocks to ranks using the given policy
  // Returns 0 on success, 1 on failure
  //
  static int AssignBlocks(PlacementArgs args);

  //
  // AssignBlocksMpi: assign blocks to ranks using the given policy in parallel
  // Returns 0 on success, 1 on failure
  //
  static int AssignBlocksMpi(PlacementArgsMpi args);
};
} // namespace lb
} // namespace amr
