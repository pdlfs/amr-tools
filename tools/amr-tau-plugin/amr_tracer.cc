#include "amr_tracer.h"

#include "tau_types.h"

namespace {

#define BUFSZ 65536

std::string JoinVec(std::vector<double> const& v) {
  char buf[BUFSZ];
  size_t ptr = 0;
  size_t rem = BUFSZ;

  for (double k : v) {
    int cur = snprintf(buf + ptr, rem, "%.2lf,", k);
    if (cur > rem) break;
    ptr += cur;
    rem -= cur;
  }

  return buf;
}

std::string JoinVec(std::vector<int> const& v) {
  char buf[BUFSZ];
  size_t ptr = 0;
  size_t rem = BUFSZ;

  for (int k : v) {
    int cur = snprintf(buf + ptr, rem, "%d,", k);
    if (cur > rem) break;
    ptr += cur;
    rem -= cur;
  }

  return buf;
}
}  // namespace

namespace tau {

void AMRTracer::ProcessTriggerMsg(void* data) {
  TriggerMsg* msg = (TriggerMsg*)data;
  switch (msg->type) {
    case MsgType::kBlockAssignment:
      ProcessTriggerMsgBlockAssignment(msg->data);
      break;
    case MsgType::kTargetCost:
      ProcessTriggerMsgTargetCost(msg->data);
      break;
    case MsgType::kTsEnd:
      msglog_->Flush(ts_sim_);
      ts_sim_++;
      break;
    case MsgType::kBlockEvent:
      ProcessTriggerMsgBlockEvent(msg->data);
      break;
    case MsgType::kCommChannel:
      ProcessTriggerMsgCommChannel(msg->data);
      break;
    case MsgType::kMsgSend:
      ProcessTriggerMsgLogSend(msg->data);
      break;
    default:
      MLOG(MLOG_ERRO, "Unknown trigger msg type!");
      break;
  }

  return;
}

void AMRTracer::ProcessTriggerMsgBlockAssignment(void* data) {
  MsgBlockAssignment* msg = (MsgBlockAssignment*)data;

  // std::string clstr = JoinVec(*(msg->costlist));
  // std::string rlstr = JoinVec(*(msg->ranklist));

  // MLOG(MLOG_DBG2, "[Rank %d: CL] %s\n", rank_, clstr.c_str());
  // MLOG(MLOG_DBG2, "[Rank %d: RL] %s\n", rank_, rlstr.c_str());

  // statelog_->LogKV(timestep_, "CL", clstr.c_str());
  // statelog_->LogKV(timestep_, "RL", rlstr.c_str());
}

void AMRTracer::ProcessTriggerMsgTargetCost(void* data) {
  double target_cost = *(double*)data;
  std::string cost_str = std::to_string(target_cost);
  // statelog_->LogKV(timestep_, "TC", cost_str.c_str());
}

void AMRTracer::ProcessTriggerMsgBlockEvent(void* data) {
  MsgBlockEvent* msg = (MsgBlockEvent*)data;
  proflog_->LogEvent(ts_sim_, msg->block_id, msg->opcode, msg->data);
}

void AMRTracer::ProcessTriggerMsgCommChannel(void* data) {
  MsgCommChannel* msg = (MsgCommChannel*)data;
  msglog_->LogChannel(msg->ptr, msg->block_id, msg->block_rank, msg->nbr_id,
                      msg->nbr_rank, msg->tag, msg->is_flux);
}

void AMRTracer::ProcessTriggerMsgLogSend(void* data) {
  MsgSend* msg = (MsgSend*)data;
  msglog_->LogSend(msg->ptr, msg->buf_sz, msg->recv_rank, msg->tag,
                   msg->timestamp);
}
}  // namespace tau
