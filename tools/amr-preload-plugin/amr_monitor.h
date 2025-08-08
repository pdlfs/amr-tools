#pragma once

#include "amr_opts.h"
#include "common.h"
#include "detailed_logger.h"
#include "metric.h"
#include "metric_util.h"
#include "mpi_async.h"
#include "p2p.h"
#include "print_utils.h"
#include "tools-common/logging.h"
#include "tracer.h"
#include "types.h"

#include <cinttypes>
#include <glog/logging.h>
#include <pdlfs-common/env.h>

namespace amr {

#define TSWISE_FILE_ARG AMROptUtils::GetTswiseOutputFile(amr_opts, env_, rank_)
#define TRACER_FILE_ARG AMROptUtils::GetTracerOutputFile(amr_opts, env_, rank_)

class AMRMonitor {
public:
  AMRMonitor(pdlfs::Env *env, int rank, int nranks)
      : env_(env), rank_(rank), nranks_(nranks),
        tswise_logger_(TSWISE_FILE_ARG, rank), tracer_(TRACER_FILE_ARG, rank) {
    if (rank == 0) {
      MLOG(MLOG_INFO, "AMRMonitor initializing.");
      AMROptUtils::LogOpts(amr_opts);

      if (amr_opts.tswise_enabled) {
        MLOG(MLOG_WARN,
             "Timestep-wise logging is enabled! This may produce lots of "
             "data!!");
      }

      if (amr_opts.tracing_enabled) {
        MLOG(MLOG_WARN, "Tracing is enabled! This may produce lots of data!!");
      }
    }

    LogInitInfo();
  }

  ~AMRMonitor() {
    if (amr_opts.tswise_enabled) {
      tswise_logger_.FinalFlush();
    }

    LogMetrics();
    MLOGIF(!rank_, MLOG_INFO, "AMRMonitor destroyed on rank %d", rank_);
  }

  void LogInitInfo() {
    std::stringstream ss;
    ss << "rank:" << rank_;

    pid_t pid = getpid();
    ss << "|pid:" << pid;

    char hostname[1024];
    gethostname(hostname, 1024);
    ss << "|host:" << hostname;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t init_time = ts.tv_sec * 1000000000 + ts.tv_nsec;
    ss << "|inittime:" << init_time;

    MLOG(MLOG_INFO, "amrmonitor|%s", ss.str().c_str());
  }

  uint64_t Now() const {
    auto now = env_->NowMicros();
    return now;
  }

  void LogMPICollectiveBegin(const char *type) {
    tswise_logger_.LogBegin(type);
    tracer_.LogFuncBeginCoalesced(type);
  }

  void LogMPICollectiveEnd(const char *type, uint64_t time) {
    tswise_logger_.LogEnd(type, time);
    LogKey(times_us_, type, time);
    tracer_.LogFuncEndCoalesced(type);
  }

  void LogStackBegin(const char *type, const char *key) {
    if (stack_map_.find(type) == stack_map_.end()) {
      stack_map_[type] = ProfStack();
    }

    auto &s = stack_map_[type];
    s.push(StackOpenPair(key, Now()));
    tswise_logger_.LogBegin(key);
    tracer_.LogFuncBeginCoalesced(key);
  }

  void LogStackEnd(const char *type) {
    if (stack_map_.find(type) == stack_map_.end()) {
      MLOG(MLOG_WARN, "type %s not found in stack_map_.", type);
      return;
    }

    auto &s = stack_map_[type];
    if (s.empty()) {
      MLOG(MLOG_WARN, "Rank %d: stack %s is empty.", rank_, type);
      sleep(1000);
      return;
    }

    auto key = s.top().first;
    auto end_time = Now();
    auto begin_time = s.top().second;
    auto elapsed_time = end_time - begin_time;

    LogKey(times_us_, key.c_str(), elapsed_time);
    tswise_logger_.LogEnd(key.c_str(), elapsed_time);

    s.pop();

    tracer_.LogFuncEndCoalesced(key.c_str());

    return;
  }

  inline int GetMPITypeSizeCached(MPI_Datatype datatype) {
    if (mpi_datatype_sizes_.find(datatype) == mpi_datatype_sizes_.end()) {
      int size;
      int rv = PMPI_Type_size(datatype, &size);
      if (rv != MPI_SUCCESS) {
        MLOG(MLOG_WARN, "PMPI_Type_size failed");
      }

      mpi_datatype_sizes_[datatype] = size;
    }

    return mpi_datatype_sizes_[datatype];
  }

  void LogMPISend(int dest_rank, MPI_Datatype datatype, int count) {
    auto size = count * GetMPITypeSizeCached(datatype);
    p2p_comm_.LogSend(dest_rank, size);
  }

  void LogMPIRecv(int src_rank, MPI_Datatype datatype, int count) {
    auto size = count * GetMPITypeSizeCached(datatype);
    p2p_comm_.LogRecv(src_rank, size);
  }

  void LogKey(MetricMap &map, const char *key, uint64_t val) {
    MLOGIF(!rank_, MLOG_DBG2, "Rank %d: key %s, val: %" PRIu64 "\n", rank_, key,
           val);
    // must use iterators because Metric class has const variables,
    // and therefore can not be assigned to and all
    auto it = map.find(key);
    if (it == map.end()) {
      map.insert({key, Metric(key, rank_)});
      it = map.find(key);
    }

    it->second.LogInstance(val);
  }

  StringVec GetCommonMetrics() {
    MLOGIF(!rank_, MLOG_DBG0, "Entering GetCommonMetrics.");

    StringVec local_metrics;
    for (auto &kv : times_us_) {
      local_metrics.push_back(kv.first);
    }

    auto intersection_computer = CommonComputer(local_metrics, rank_, nranks_);
    auto intersection = intersection_computer.Compute();

    MLOGIF(!rank_, MLOG_DBG0, "Exiting GetCommonMetrics.");
    return intersection;
  }

  void LogMetrics() {
    MLOGIF(!rank_, MLOG_DBG0, "Entering LogMetrics.");

    // First, need to get metrics that are logged on all ranks
    // as collectives will block on ranks that are missing a given metric
    StringVec common_metrics = GetCommonMetrics();

    auto compute_metric_stats =
        Metric::CollectMetrics(common_metrics, times_us_);
    auto compute_metric_str = MetricPrintUtils::SortAndSerialize(
        compute_metric_stats, amr_opts.print_topk);

    if (rank_ == 0) {
      fprintf(stderr, "%s\n\n", compute_metric_str.c_str());
    }

    if (amr_opts.rankwise_enabled) {
      CollectMetricsDetailed(common_metrics);
    }

    if (amr_opts.p2p_enable_matrix_put) {
      MLOGIF(!rank_, MLOG_DBG0, "Collecting P2P matrix with RMA PUT.");

      auto p2p_matrix_str = p2p_comm_.CollectAndAnalyze(rank_, nranks_, true);
      if (rank_ == 0) {
        fprintf(stderr, "%s\n", p2p_matrix_str.c_str());
      }
    }

    if (amr_opts.p2p_enable_matrix_reduce) {
      MLOGIF(!rank_, MLOG_DBG0, "Collecting P2P matrix with RMA PUT.");

      auto p2p_matrix_str = p2p_comm_.CollectAndAnalyze(rank_, nranks_, false);
      if (rank_ == 0) {
        fprintf(stderr, "%s\n", p2p_matrix_str.c_str());
      }
    }

    MLOGIF(!rank_, MLOG_DBG0, "Exiting LogMetrics.");
  }

  void CollectMetricsDetailed(StringVec const &metrics) {
    pdlfs::WritableFile *f;
    pdlfs::Status s =
        env_->NewWritableFile(amr_opts.rankwise_fpath.c_str(), &f);
    if (!s.ok()) {
      MLOG(MLOG_WARN, "Failed to open file %s",
           amr_opts.rankwise_fpath.c_str());
      return;
    }

    for (auto &m : metrics) {
      auto it = times_us_.find(m);
      if (it != times_us_.end()) {
        auto metric_str = it->second.GetMetricRankwise(nranks_);
        if (rank_ == 0) {
          f->Append(pdlfs::Slice(metric_str.c_str(), metric_str.size()));
        }
      }
    }
  }

  void LogAsyncMPIRequest(const char *key, MPI_Request *request) {
    async_assist_.LogAsyncBegin(key, request);
  }

  //
  // elapsed is used as a signalling mechanism for the MPI_Async tracker
  // currently we track only MPI_Test as that's what Phoebus/Parthenon use
  // for async collectives. This logic is incorrect if they change to also
  // using MPI_Wait, or if we start tracking collectives other than MPI
  // IAllgather for the "MPI_Async" bucket
  //
  void LogMPITestEnd(int flag, MPI_Request *request) {
    double elapsed_ms = 0;
    int rv = async_assist_.LogMPITestEnd(flag, request, &elapsed_ms);

    if (rv) {
      LogKey(times_us_, "MPI_Iallgather_Async", elapsed_ms * 1000);
    }
  }

private:
  MetricMap times_us_;
  StackMap stack_map_;

  P2PCommCollector p2p_comm_;

  std::unordered_map<MPI_Datatype, uint32_t> mpi_datatype_sizes_;

  std::unordered_map<std::string, uint64_t> begin_times_us_;

  pdlfs::Env *const env_;
  const int rank_;
  const int nranks_;
  TimestepwiseLogger tswise_logger_;

  MPIAsync async_assist_;

public:
  Tracer tracer_;
};
} // namespace amr
