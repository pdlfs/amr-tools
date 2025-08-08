//
// Created by Ankush J on 4/10/23.
//

#include <gtest/gtest.h>

#include "lb-common/lb_policies.h"
#include "tools-common/common.h"
#include "tools-common/logging.h"

namespace amr {
class LoadBalancingPoliciesTest : public ::testing::Test {
protected:
  template <typename T>
  static void AssertEqual(const std::vector<T> a, const std::vector<T> b) {
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size(); i++) {
      ASSERT_EQ(a[i], b[i]);
    }
  }

  void AssignBlocksSPT(std::vector<double> const &costlist,
                       std::vector<int> &ranklist, int nranks) {
    LoadBalancePolicies::AssignBlocksSPT(costlist, ranklist, nranks);
  }

  void AssignBlocksLPT(std::vector<double> const &costlist,
                       std::vector<int> &ranklist, int nranks) {
    LoadBalancePolicies::AssignBlocksLPT(costlist, ranklist, nranks);
  }

  void AssignBlocksContigImproved(std::vector<double> const &costlist,
                                  std::vector<int> &ranklist, int nranks) {
    LoadBalancePolicies::AssignBlocksContigImproved(costlist, ranklist, nranks);
  }

  void AssignBlocksContigImproved2(std::vector<double> const &costlist,
                                   std::vector<int> &ranklist, int nranks) {
    LoadBalancePolicies::AssignBlocksContigImproved2(costlist, ranklist,
                                                     nranks);
  }
};

TEST_F(LoadBalancingPoliciesTest, SPTTest1) {
  MLOG(MLOG_INFO, "SPT Test 1");

  std::vector<double> costlist = {1, 2, 3, 4};
  std::vector<int> ranklist;
  ranklist.resize(costlist.size());

  int nranks = 2;
  AssignBlocksSPT(costlist, ranklist, nranks);

  std::string ranklist_str = SerializeVector(ranklist);
  MLOG(MLOG_INFO, "Assignment: %s", ranklist_str.c_str());

  ASSERT_EQ(ranklist[0], ranklist[2]);
  ASSERT_EQ(ranklist[1], ranklist[3]);
}

TEST_F(LoadBalancingPoliciesTest, LPTTest1) {
  MLOG(MLOG_INFO, "LPT Test 1");

  std::vector<double> costlist = {1, 2, 3, 4};
  std::vector<int> ranklist;
  ranklist.resize(costlist.size());

  int nranks = 2;
  AssignBlocksLPT(costlist, ranklist, nranks);

  std::string ranklist_str = SerializeVector(ranklist);
  MLOG(MLOG_INFO, "Assignment: %s", ranklist_str.c_str());

  ASSERT_EQ(ranklist[0], ranklist[3]);
  ASSERT_EQ(ranklist[1], ranklist[2]);
}

TEST_F(LoadBalancingPoliciesTest, LPTTest2) {
  MLOG(MLOG_INFO, "LPT Test 2");

  std::vector<double> costlist = {1, 2, 3, 4, 5, 6, 8, 11};
  std::vector<int> ranklist;
  ranklist.resize(costlist.size());

  int nranks = 3;
  AssignBlocksLPT(costlist, ranklist, nranks);

  std::string ranklist_str = SerializeVector(ranklist);
  MLOG(MLOG_INFO, "Assignment: %s", ranklist_str.c_str());

  AssertEqual(ranklist, {1, 2, 0, 1, 2, 2, 1, 0});
}

TEST_F(LoadBalancingPoliciesTest, CDPTest1) {
  MLOG(MLOG_INFO, "CDP Test 1");
  /*std::vector<double> costlist = {2, 3, 2, 3, 2, 3, 2, 3, 2, 3};*/
  std::vector<double> costlist = {1, 2, 3, 4, 1, 2, 10};
  std::vector<int> ranklist;
  ranklist.resize(costlist.size());

  int nranks = 3;
  AssignBlocksContigImproved(costlist, ranklist, nranks);
  AssignBlocksContigImproved2(costlist, ranklist, nranks);
}
} // namespace amr
