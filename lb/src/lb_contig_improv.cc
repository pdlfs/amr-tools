//
// Created by Ankush J on 5/22/23.
//

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <vector>

#include "lb-common/lb_policies.h"
#include "tools-common/logging.h"

namespace {
double GetTimeMs() {
  // wrapper over clock_monotonic
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double time_ms = ts.tv_sec * 1e3 + ts.tv_nsec * 1e-6;
  return time_ms;
}

void GetRollingSum(std::vector<double> const& v, std::vector<double>& sum,
                   int k) {
  double rolling_sum = 0;

  for (int i = 0; i < v.size(); i++) {
    rolling_sum += v[i];

    if (i >= k - 1) {
      sum.push_back(rolling_sum);
      rolling_sum -= v[i - k + 1];
    }
  }

  double rolling_max = *std::max_element(sum.begin(), sum.end());
  double rolling_min = *std::min_element(sum.begin(), sum.end());

  MLOG(MLOG_DBG2, "K: %d, Rolling Max: %.2lf, Rolling Min: %.2lf", k,
       rolling_max, rolling_min);
}

bool IsRangeAvailable(std::vector<int> const& ranklist, int start, int end) {
  for (int i = start; i <= end; i++) {
    if (ranklist[i] != -1) {
      return false;
    }
  }
  return true;
}

bool MarkRange(std::vector<int>& ranklist, int start, int end, int flag) {
  MLOG(MLOG_DBG3, "MarkRange marking [%d, %d] with %d", start, end, flag);

  for (int i = start; i <= end; i++) {
    ranklist[i] = flag;
  }
  return true;
}

double GetSumRange(std::vector<double> const& cum_sum, int a, int b) {
  if (a > 0) {
    return cum_sum[b] - cum_sum[a - 1];
  } else {
    return cum_sum[b];
  }
}

#define LOG_TIME(evt) \
  MLOG(MLOG_DBG0, "Time taken until %s: %.2lf ms", evt, GetTimeMs() - _ts_beg);

int AssignBlocksDP(std::vector<double> const& costlist,
                   std::vector<int>& ranklist, int nranks) {
  double _ts_beg = GetTimeMs();

  double cost_total = std::accumulate(costlist.begin(), costlist.end(), 0.0);
  double cost_target = cost_total / nranks;
  MLOG(MLOG_DBG2, "Target Cost: %.2lf", cost_target);

  std::vector<double> cum_costlist(costlist);
  int nblocks = costlist.size();
  int n_a = std::floor(nblocks * 1.0 / nranks);
  int n_b = std::ceil(nblocks * 1.0 / nranks);
  int nalloc_b = nblocks % nranks;
  int nalloc_a = nranks - nalloc_b;

  MLOG(MLOG_DBG0, "nalloc_a: %d, nalloc_b: %d", nalloc_a, nalloc_b);

  for (int i = 1; i < nblocks; i++) {
    cum_costlist[i] += cum_costlist[i - 1];
  }

  const double kBigDouble = cost_total * 1e3;
  //  std::vector<double> dp(nblocks + 1, kBigDouble);
  //  double dp[nalloc_a + 1][nalloc_b + 1];
  std::vector<std::vector<double>> dp(nalloc_a + 1,
                                      std::vector<double>(nalloc_b + 1));
  for (int i = 0; i <= nalloc_a; i++) {
    for (int j = 0; j <= nalloc_b; j++) {
      dp[i][j] = kBigDouble;
    }
  }

  LOG_TIME("INIT");

  dp[0][0] = 0;
  for (int i = 0; i <= nalloc_a; i++) {
    for (int j = 0; j <= nalloc_b; j++) {
      int l = i * n_a + j * n_b;

      // option 1. we add an n_a chunk ending at l - 1
      // chunk range: [l - n_a, l - 1]
      double opt1_cost = kBigDouble;
      if (l >= n_a and i > 0) {
        double opt1_cost_a = GetSumRange(cum_costlist, l - n_a, l - 1);
        double opt1_cost_b = dp[i - 1][j];
        opt1_cost = std::max(opt1_cost_a, opt1_cost_b);
      }

      // option 2. we add an n_b chunk ending at l - 1
      // chunk range: [l - n_b, l - 1]
      double opt2_cost = kBigDouble;
      if (l >= n_b and j > 0) {
        double opt2_cost_a = GetSumRange(cum_costlist, l - n_b, l - 1);
        double opt2_cost_b = dp[i][j - 1];
        opt2_cost = std::max(opt2_cost_a, opt2_cost_b);
      }

      double dp_cost = std::min(opt1_cost, opt2_cost);
      dp_cost = std::min(dp_cost, kBigDouble);

      if (dp_cost != kBigDouble) {
        dp[i][j] = dp_cost;
        // MLOG(MLOG_DBG3, "DP[%d][%d]: %.2lf", i, j, dp[i][j]);
      }
    }
  }

  LOG_TIME("DP");
  MLOG(MLOG_DBG2, "DP Cost: %.2lf", dp[nalloc_a][nalloc_b]);

  int i = nalloc_a;
  int j = nalloc_b;
  int cur_rank = nranks - 1;
  while (i > 0 or j > 0) {
    int l = i * n_a + j * n_b;

    // option 1. we add an n_a chunk ending at l - 1
    // chunk range: [l - n_a, l - 1]
    double opt1_cost = kBigDouble;
    if (l >= n_a and i > 0) {
      double opt1_cost_a = GetSumRange(cum_costlist, l - n_a, l - 1);
      double opt1_cost_b = dp[i - 1][j];
      opt1_cost = std::max(opt1_cost_a, opt1_cost_b);
    }

    // option 2. we add an n_b chunk ending at l - 1
    // chunk range: [l - n_b, l - 1]
    double opt2_cost = kBigDouble;
    if (l >= n_b and j > 0) {
      double opt2_cost_a = GetSumRange(cum_costlist, l - n_b, l - 1);
      double opt2_cost_b = dp[i][j - 1];
      opt2_cost = std::max(opt2_cost_a, opt2_cost_b);
    }

    double dp_cost = std::min(opt1_cost, opt2_cost);
    // should not encounter invalid solutions while backtracking
    assert(dp_cost != kBigDouble);
    if (dp_cost == opt1_cost) {
      MLOG(MLOG_DBG3, "Backtracking [%d][%d]->[%d][%d]", i, j, i - 1, j);
      MarkRange(ranklist, l - n_a, l - 1, cur_rank);
      i--;
    } else {
      MLOG(MLOG_DBG3, "Backtracking [%d][%d]->[%d][%d]", i, j, i, j - 1);
      MarkRange(ranklist, l - n_b, l - 1, cur_rank);
      j--;
    }

    cur_rank--;
  }

  LOG_TIME("BACKTRACK");

  return 0;
}
}  // namespace

namespace amr {
int LoadBalancePolicies::AssignBlocksContigImproved(
    std::vector<double> const& costlist, std::vector<int>& ranklist,
    int nranks) {
  int nblocks = costlist.size();
  if (nblocks % nranks == 0) {
    MLOG(MLOG_DBG0,
         "Blocks evenly divisible by nranks_, using AssignBlocksContiguous");
    return AssignBlocksContiguous(costlist, ranklist, nranks);
  } else {
    return ::AssignBlocksDP(costlist, ranklist, nranks);
  }
}
}  // namespace amr
