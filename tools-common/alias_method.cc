// (C) (or copyright) 2021. Triad National Security, LLC. All rights reserved.
//
// This program was produced under U.S. Government contract 89233218CNA000001
// for Los Alamos National Laboratory (LANL), which is operated by Triad
// National Security, LLC for the U.S. Department of Energy/National Nuclear
// Security Administration. All rights in the program are reserved by Triad
// National Security, LLC, and the U.S. Department of Energy/National Nuclear
// Security Administration. The Government is granted for itself and others
// acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license
// in this material to reproduce, prepare derivative works, distribute copies to
// the public, perform publicly and display publicly, and to permit others to do
// so.
//========================================================================================
//! \file alias_method.cpp
//  \brief Construct tables used to sample from a discrete probability
//  distribution using the alias method

#include "alias_method.h"

#include <numeric>
#include <queue>

namespace amr {

AliasMethod::AliasMethod(const std::vector<double>& probabilities)
    : prob_table(probabilities.size()), alias_table(probabilities.size()) {
  // make probability table and alias table, this implementation is based on
  // https://gist.github.com/Liam0205/0b5786e9bfc73e75eb8180b5400cd1f8
  //  auto prob_table = Kokkos::create_mirror_view(prob_table);
  //  auto alias_table = Kokkos::create_mirror_view(alias_table);

  // explicitly using double for sum and casting result to Real
  double prob_sum = std::accumulate(probabilities.begin(), probabilities.end(),
                                    static_cast<double>(0.0));

  std::queue<int> under_full, over_full;
  for (std::size_t i = 0; i < probabilities.size(); ++i) {
    alias_table[i] = -1;

    prob_table[i] = (probabilities.size()) * probabilities[i] / prob_sum;
    (prob_table[i] < 1.0 ? under_full : over_full).push(i);
  }

  while (!under_full.empty() && !over_full.empty()) {
    int under_idx = under_full.front();
    under_full.pop();
    int over_idx = over_full.front();
    over_full.pop();

    alias_table[under_idx] = over_idx;
    prob_table[over_idx] = (prob_table[over_idx] + prob_table[under_idx]) - 1.0;
    (prob_table[over_idx] < 1.0 ? under_full : over_full).push(over_idx);
  }

  while (!over_full.empty()) {
    int idx = over_full.front();
    over_full.pop();

    prob_table[idx] = 1.0;
    alias_table[idx] = idx;
  }

  while (!under_full.empty()) {
    int idx = under_full.front();
    under_full.pop();

    prob_table[idx] = 1.0;
    alias_table[idx] = idx;
  }

  //  Kokkos::deep_copy(prob_table, prob_table);
  //  Kokkos::deep_copy(alias_table, alias_table);
}

}  // namespace amr
