#include "run_stats.hh"

#include <n4-inspect.hh>

float run_stats::over_threshold_fraction() {
  return 100.0 * n_over_threshold / (n4::event_number() + 1);
}
