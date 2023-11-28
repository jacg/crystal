#pragma once

#include <cstddef>
#include <unordered_map>

struct run_stats {
  unsigned n_detected_evt   = 0;
  unsigned n_over_threshold = 0;
  unsigned n_detected_total = 0;
  float n_events_over_threshold_fraction() const;
  std::unordered_map<size_t, size_t> n_detected_at_sipm;
  size_t n_sipms_over_threshold(size_t threshold) const;
};
