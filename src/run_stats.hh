#pragma once

struct run_stats {
  unsigned n_detected_evt   = 0;
  unsigned n_over_threshold = 0;
  unsigned n_detected_total = 0;
  float over_threshold_fraction();
};
