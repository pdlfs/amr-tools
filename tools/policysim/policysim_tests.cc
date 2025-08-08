//
// Created by Ankush J on 4/11/23.
//

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <pdlfs-common/env.h>

#include "bin_readers.h"
#include "block_alloc_sim.h"
#include "tools-common/distributions.h"
#include "prof_set_reader.h"

namespace amr {
class MiscTest : public ::testing::Test {
 protected:
  std::string GetLogPath(const char* output_dir, const char* policy_name) {
    return PolicyUtils::GetLogPath(output_dir, policy_name, ".csv");
  }

  void AssertApproxEqual(std::vector<double> const& a,
                         std::vector<double> const& b) {
    ASSERT_EQ(a.size(), b.size());
    for (int i = 0; i < a.size(); i++) {
      ASSERT_NEAR(a[i], b[i], 0.0001);
    }
  }

  static const LBPolicyWithOpts GenHybrid(const std::string& policy_str) {
    LBPolicyWithOpts policy = amr::PolicyUtils::GenHybrid(policy_str);
    return policy;
  }
};

TEST_F(MiscTest, PolicyOptsHybridTest) {
  std::string policy_str = "hybrid25";
  auto policy_wopts = GenHybrid(policy_str);
  ASSERT_EQ(policy_wopts.hcf_opts.lpt_frac, 25 / 100.0);
  ASSERT_EQ(policy_wopts.hcf_opts.alt_solncnt_max, 0);

  policy_str = "hybrid99alt0";
  policy_wopts = GenHybrid(policy_str);
  ASSERT_EQ(policy_wopts.hcf_opts.lpt_frac, 99 / 100.0);
  ASSERT_EQ(policy_wopts.hcf_opts.alt_solncnt_max, 0);

  policy_str = "hybrid99alt10";
  policy_wopts = GenHybrid(policy_str);
  ASSERT_EQ(policy_wopts.hcf_opts.lpt_frac, 99 / 100.0);
  ASSERT_EQ(policy_wopts.hcf_opts.alt_solncnt_max, 10);
}

TEST_F(MiscTest, OutputFileTest) {
  std::string policy_name = "RoundRobin_Actual-Cost";
  std::string fname = GetLogPath("/a/b/c", policy_name.c_str());
  ASSERT_EQ(fname, "/a/b/c/roundrobin_actual_cost.csv");
}

TEST_F(MiscTest, RefReaderTest) {
  std::string fpath =
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini";
  RefinementReader reader(fpath);
  int ts, sub_ts = 0;
  int rv;
  std::vector<int> refs, derefs;

  do {
    rv = reader.ReadTimestep(ts, sub_ts, refs, derefs);
    MLOG(MLOG_DBG0,
         "rv: %d, ts: %d, sub_ts: %d, refs: %s, derefs: %s", rv, ts, sub_ts,
         SerializeVector(refs, 10).c_str(),
         SerializeVector(derefs, 10).c_str());
    sub_ts++;

    if (sub_ts == 1000) break;
  } while (rv);
}

TEST_F(MiscTest, AssignReaderTest) {
  std::string fpath =
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini";
  AssignmentReader reader(fpath);
  int ts, sub_ts = 0;
  int rv;
  std::vector<int> blocks;

  do {
    rv = reader.ReadTimestep(ts, sub_ts, blocks);
    MLOG(MLOG_DBG0, "rv: %d, ts: %d, sub_ts: %d, blocks: %s", rv,
         ts, sub_ts, SerializeVector(blocks, 10).c_str());
    sub_ts++;

    if (sub_ts == 1000) break;
  } while (rv);
}

TEST_F(MiscTest, BlockAllocSimTest) {
  BlockSimulatorOpts opts{};
  opts.nranks = 512;
  opts.nblocks = 512;
  opts.prof_dir = "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini";
  opts.output_dir = opts.prof_dir + "/output";
  opts.env = pdlfs::Env::Default();

  BlockSimulator sim(opts);
  sim.Run();
}

TEST_F(MiscTest, prof_reader_test) {
  int rv;

  std::vector<std::string> all_profs = {
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini/"
      "prof.merged.evt0.csv",
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini/"
      "prof.merged.evt1.csv"};

  CSVProfileReader reader(all_profs[1].c_str(), ProfTimeCombinePolicy::kAdd);

  std::vector<int> times;
  int nlines_read;

  times.resize(0);
  rv = reader.ReadTimestep(-1, times, nlines_read);
  MLOG(MLOG_DBG0, "RV: %d, Times: %s", rv,
       SerializeVector(times, 10).c_str());

  times.resize(0);
  rv = reader.ReadTimestep(0, times, nlines_read);
  MLOG(MLOG_DBG0, "RV: %d, Times: %s", rv,
       SerializeVector(times, 10).c_str());

  times.resize(0);
  rv = reader.ReadTimestep(1, times, nlines_read);
  MLOG(MLOG_DBG0, "RV: %d, Times: %s", rv,
       SerializeVector(times, 10).c_str());

  times.resize(0);
  rv = reader.ReadTimestep(2, times, nlines_read);
  MLOG(MLOG_DBG0, "RV: %d, Times: %s", rv,
       SerializeVector(times, 10).c_str());

  times.resize(0);
  rv = reader.ReadTimestep(3, times, nlines_read);
  MLOG(MLOG_DBG0, "RV: %d, Times: %s", rv,
       SerializeVector(times, 10).c_str());
}

TEST_F(MiscTest, prof_set_reader_test) {
  std::vector<std::string> all_profs = {
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini/"
      "prof.merged.evt0.csv",
      "/Users/schwifty/Repos/amr-data/20230424-prof-tags/ref-mini/"
      "prof.merged.evt1.csv"};

  ProfSetReader reader(all_profs, ProfTimeCombinePolicy::kAdd);
  std::vector<int> times;
  int sub_ts = -1, rv;

  do {
    rv = reader.ReadTimestep(sub_ts, times);
    MLOG(MLOG_DBG0, "[PSRTest] TS: %d, RV: %d, Times: %s", sub_ts,
         rv, SerializeVector(times, 10).c_str());
    sub_ts++;
  } while (rv);
}

TEST_F(MiscTest, ExtrapolateCosts1) {
  std::vector<double> costs_prev = {1.0};
  std::vector<int> refs = {0};
  std::vector<int> derefs = {};
  std::vector<double> costs_cur;

  PolicyUtils::ExtrapolateCosts3D(costs_prev, refs, derefs, costs_cur);
  MLOG(MLOG_DBG0, "Costs Prev: %s",
       SerializeVector(costs_prev, 10).c_str());
  MLOG(MLOG_DBG0, "Costs Cur: %s",
       SerializeVector(costs_cur, 10).c_str());
  AssertApproxEqual(costs_cur, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
}

TEST_F(MiscTest, ExtrapolateCosts2) {
  std::vector<double> costs_prev = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  std::vector<int> refs = {};
  std::vector<int> derefs = {0, 1, 2, 3, 4, 5, 6, 7};
  std::vector<double> costs_cur;

  PolicyUtils::ExtrapolateCosts3D(costs_prev, refs, derefs, costs_cur);
  MLOG(MLOG_DBG0, "Costs Prev: %s",
       SerializeVector(costs_prev, 10).c_str());
  MLOG(MLOG_DBG0, "Costs Cur: %s",
       SerializeVector(costs_cur, 10).c_str());
  AssertApproxEqual(costs_cur, {4.5});
}

TEST_F(MiscTest, ExtrapolateCosts3) {
  std::vector<double> costs_prev = {1.0, 2.0, 3.0, 4.0, 5.0,
                                    6.0, 7.0, 8.0, 9.0};
  std::vector<int> refs = {0};
  std::vector<int> derefs = {1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<double> costs_cur;

  PolicyUtils::ExtrapolateCosts3D(costs_prev, refs, derefs, costs_cur);
  MLOG(MLOG_DBG0, "Costs Prev: %s",
       SerializeVector(costs_prev, 10).c_str());
  MLOG(MLOG_DBG0, "Costs Cur: %s",
       SerializeVector(costs_cur, 10).c_str());
  AssertApproxEqual(costs_cur, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 5.5});
}

TEST_F(MiscTest, BinProfReaderTest) {
  BinProfileReader bpreader("/Users/schwifty/CRAP/tmp.bin",
                            ProfTimeCombinePolicy::kAdd);
  std::vector<int> times;
  while (true) {
    int nread = bpreader.ReadNextTimestep(times);
    if (nread == -1) break;
    MLOG(MLOG_INFO, "Times: %s",
         SerializeVector(times, 10).c_str());
  }
}

TEST_F(MiscTest, PolicyUtilsTest1) {
  std::string policy_name = "cdpc512";
  auto policy = PolicyUtils::GetPolicy(policy_name.c_str());
  ASSERT_EQ(policy.policy, LoadBalancePolicy::kPolicyCDPChunked);
  ASSERT_EQ(policy.chunked_opts.chunk_size, 512);
  ASSERT_EQ(policy.chunked_opts.parallelism, 1);

  policy_name = "cdpc512par1";
  policy = PolicyUtils::GetPolicy(policy_name.c_str());
  ASSERT_EQ(policy.policy, LoadBalancePolicy::kPolicyCDPChunked);
  ASSERT_EQ(policy.chunked_opts.chunk_size, 512);
  ASSERT_EQ(policy.chunked_opts.parallelism, 1);

  policy_name = "cdpc256par32";
  policy = PolicyUtils::GetPolicy(policy_name.c_str());
  ASSERT_EQ(policy.policy, LoadBalancePolicy::kPolicyCDPChunked);
  ASSERT_EQ(policy.chunked_opts.chunk_size, 256);
  ASSERT_EQ(policy.chunked_opts.parallelism, 32);
}

TEST_F(MiscTest, DistribGenTest) {
  const char* fname = "/tmp/test.txt";
  const char* fdata = "1.0 2.0 3.0 44.0";
  FILE* f = fopen(fname, "w");
  fprintf(f, "%s", fdata);
  fclose(f);

  std::vector<double> costs;
  int nblocks = 4;
  DistributionUtils::GenFromFile(costs, nblocks, fname);

  EXPECT_EQ(costs.size(), nblocks);
  EXPECT_NEAR(costs[0], 1.0, 0.0001);
  EXPECT_NEAR(costs[1], 2.0, 0.0001);
  EXPECT_NEAR(costs[2], 3.0, 0.0001);
  EXPECT_NEAR(costs[3], 44.0, 0.0001);
}
}  // namespace amr