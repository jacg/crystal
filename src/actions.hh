#pragma once

#include <n4-mandatory.hh>

auto my_generator();

struct run_stats {
  unsigned n_detected_evt   = 0;
  unsigned n_over_threshold = 0;
  unsigned n_detected_total = 0;
  float over_threshold_fraction();
};

n4::actions* create_actions(run_stats& data);
