#include "single_ts_trace_reader.h"

#include "tools-common/common.h"
#include "tools-common/logging.h"

namespace topo {
Status SingleTimestepTraceReader::Read(int rank) {
  Status s = Status::OK;

  MLOGIFR0(MLOG_INFO, "[TimelessTraceReader] Reading trace file: %s",
           trace_file_.c_str());

  if (trace_file_ == "") {
    MLOGIFR0(MLOG_ERRO, "[TimelessTraceReader] No trace file provided");
    return Status::Error;
  }

  if (file_read_) {
    return Status::OK;
  }

  MLOGIFR0(MLOG_DBG0, "[TimelessTraceReader] Reading %s\n",
           trace_file_.c_str());

  FILE *f = fopen(trace_file_.c_str(), "r");
  if (f == nullptr) {
    MLOGIFR0(MLOG_ERRO, "[TimelessTraceReader] Read Failed: %s",
             strerror(errno));
    s = Status::Error;
    return s;
  }

  file_read_ = true;

  size_t buf_sz = 4096;
  char buf[buf_sz];

  /* scan header */
  int ret = fscanf(f, "%4095[^\n]\n", buf);
  while (!feof(f)) {
    ret = fscanf(f, "%4095[^\n]\n", buf);
    if (ret == EOF or ret == 0) break;

    s = ParseLine(buf, ret, rank);
    if (s != Status::OK) return s;
  }

  fclose(f);
  PrintSummary();

  return s;
}

Status SingleTimestepTraceReader::ParseLine(char *buf, size_t buf_sz,
                                            const int rank) {
  Status s = Status::OK;

  MLOG(MLOG_DBG3, "[TimelessTraceReader] Parsing line: %s", buf);

  /* blk_id,blk_rank,nbr_id,nbr_rank,msgsz,isflx */
  int blk_id, blk_rank, nbr_id, nbr_rank, msgsz, isflx;

  int ret = sscanf(buf, "%d,%d,%d,%d,%d,%d", &blk_id, &blk_rank, &nbr_id,
                   &nbr_rank, &msgsz, &isflx);

  if (ret != 6) {
    return Status::Error;
  }

  MLOG(MLOG_DBG3, "[TimelessTraceReader] Msg (%d -> %d), msgsz: %dB", blk_rank,
       nbr_rank, msgsz);

  if (isflx) {
    return s;
  }

  if (blk_rank == rank) {
    send_map_.push_back({blk_id, nbr_rank, msgsz});
  } else if (nbr_rank == rank) {
    recv_map_.push_back({blk_id, blk_rank, msgsz});
  }

  return s;
}
}  // namespace topo