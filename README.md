# AMR Tools

This repository contains artifacts developed during our work on AMR load-balancing and placement policies: the load-balancing policies and various simulators and benchmarks.

## Placement Policies

Contained in `/lb`.

Valid placement policy names:
- "actual": noop placement, used in sim to obtain real placement from trace
- "baseline": contiguous placement, assuming unit cost
- "lpt": Longest Processing Time
- "cdp": Contiguous-DP
- "cdpi50": CDP + iterative improvements, not used in final runs
- "cdpi250": CDP + iterative improvements, not used in final runs
- "hybrid<X>": CPLX, internal name (X in 0-100). E.g. hybrid50
- "cdpc<C>par<P>": CDP-Chunked for higher parallelism
  C: chunk size (512), P: parallelism (8 for 4096 ranks)

See kPolicyMap in `src/policy_utils.cc` for more

## Tools

- `amr-preload-plugin`: a custom plugin to collect AMR telemetry from Parthenon-based codes, via Kokkos/MPI profiling interfaces.

- `amr-tau-plugin`: an older plugin extending [TAU](https://www.cs.uoregon.edu/research/tau/home.php) for AMR telemetry.

- `commbench`: a benchmark for AMR boundary exchange using MPI P2P calls. `commbench` simulates a real AMR mesh and uses geometric face/edge/vertex neighbors to perform exhcnages, under different placement policies.

- `ember-benchmarks`: refactored but functionally identical version of the `halo3d-26` benchmark [here](https://github.com/sstsimulator/ember). Now superseded by `commbench.

- `policybench`: Basic utility to execute policies and measure basic properties under synthetic data distributions.

- `policysim`: Replays cost data collected from Parthenon via `amr-preload-plugin` under different placement policies, and measures their impact on the compute load balance of the workload.

- `scalebench`: Utility to evaluate the computation cost of different placement policies at different scales on synthetically generated cost distributions.
