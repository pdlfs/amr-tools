//
// Created by Ankush J on 4/11/22.
//

#pragma once

#include <mpi.h>

#include "comm_mesh.h"
#include "mesh_gen.h"
#include "tools-common/globals.h"
#include "tools-common/logging.h"

namespace topo {
class Driver {
 public:
  Driver(const DriverOpts &opts) : opts_(opts) {
    // amr::Globals::driver_opts = opts;
  }

  Status Setup(int argc, char *argv[]) {
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
      return Status::MPIError;
    }

    if (MPI_Comm_rank(MPI_COMM_WORLD, &(Globals::my_rank)) != MPI_SUCCESS) {
      return Status::MPIError;
    }

    if (MPI_Comm_size(MPI_COMM_WORLD, &Globals::nranks) != MPI_SUCCESS) {
      return Status::MPIError;
    }

    return Status::OK;
  }

  Status Destroy() {
    MPI_Finalize();
    return Status::OK;
  }

  void PrintOpts() {
    if (opts_.meshgen_method != MeshGenMethod::FromSingleTSTrace and
        opts_.meshgen_method != MeshGenMethod::FromMultiTSTrace) {
      MLOGIFR0(MLOG_INFO, "[Blocks Per Rank] %zu\n", opts_.blocks_per_rank);
      MLOGIFR0(MLOG_INFO, "[Size Per Msg] %zu\n", opts_.size_per_msg);
    }

    MLOGIFR0(MLOG_INFO, "[Comm Rounds] %d\n", opts_.comm_rounds);
    MLOGIFR0(MLOG_INFO, "[MeshGenMethod] %s\n",
             MeshGenMethodToStr(opts_.meshgen_method).c_str());
    MLOGIFR0(MLOG_INFO, "[Job dir] %s\n", opts_.job_dir);
    MLOGIFR0(MLOG_INFO, "[Log output] %s\n", opts_.bench_log);
  }

  void Run(int argc, char *argv[]) {
    Setup(argc, argv);
    PrintOpts();

    if (opts_.meshgen_method == MeshGenMethod::FromSingleTSTrace or
        opts_.meshgen_method == MeshGenMethod::FromMultiTSTrace) {
      RunInternalTrace();
    } else {
      RunInternalNonTrace();
    }

    ExtraMetricVec extra_metrics;
    mesh_.GenerateStats(extra_metrics, opts_.bench_log);
    Destroy();
  }

  void RunInternalNonTrace() {
    auto mesh_gen = MeshGenerator::Create(opts_);

    int nrounds = mesh_gen->GetNumTimesteps();

    for (int rnum = 0; rnum < nrounds; rnum++) {
      mesh_gen->GenerateMesh(mesh_, rnum);
      mesh_.AllocateBvars();
      mesh_.PrintConfig();
      mesh_.DoCommunicationRound();
      mesh_.ResetBvarsAndBlocks();
    }
  }

  int GetNumTimestepsToRunForTrace(int nts_trace) const {
    if (opts_.meshgen_method == MeshGenMethod::FromSingleTSTrace) {
      return 1;
    } else if (opts_.meshgen_method == MeshGenMethod::FromMultiTSTrace) {
      if (opts_.comm_nts > 1) {
        return std::min(opts_.comm_nts, nts_trace);
      } else {
        // since we skip the first ts, make sure to run at least one
        return std::min(2, nts_trace);
      }
    } else {
      ABORT("Invalid usage");
    }

    // unreachable
    return -1;
  }

  void RunInternalTrace() {
    auto mesh_gen = MeshGenerator::Create(opts_);
    const int nts_trace = mesh_gen->GetNumTimesteps();
    const int nts_to_run = GetNumTimestepsToRunForTrace(nts_trace);
    // number of times to repeat a timestep
    const int nrounds = opts_.comm_rounds;

    MLOGIFR0(MLOG_INFO,
             "num_ts found: %d, num_ts to run: %d, rounds per ts: %d\n"
             "(will skip first ts)",
             nts_trace, nts_to_run, nrounds);

    const int ts_beg =
        (opts_.meshgen_method == MeshGenMethod::FromMultiTSTrace) ? 1 : 0;

    for (int ts = ts_beg; ts < nts_to_run; ts++) {
      mesh_gen->GenerateMesh(mesh_, ts);
      MPI_Barrier(MPI_COMM_WORLD);

      mesh_.AllocateBvars();
      mesh_.PrintConfig();

      for (int rnum = 0; rnum < nrounds; rnum++) {
        mesh_.DoCommunicationRound();
        MPI_Barrier(MPI_COMM_WORLD);
      }

      mesh_.ResetBvarsAndBlocks();
    }
  }

 private:
  CommMesh mesh_;
  const DriverOpts opts_;
};
}  // namespace topo
