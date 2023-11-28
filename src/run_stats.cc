#include "run_stats.hh"

#include <algorithm>
#include <n4-inspect.hh>

float run_stats::n_events_over_threshold_fraction() const {
  return 100.0 * n_over_threshold / (n4::event_number() + 1);
}

size_t run_stats::n_sipms_over_threshold(size_t threshold) const {
  return std::count_if(
    cbegin(n_detected_at_sipm),
    cend  (n_detected_at_sipm),
    [threshold] (const auto& pair) { return pair.second > threshold; }
  );
}
