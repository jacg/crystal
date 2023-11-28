#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

auto my_generator();

n4::actions* create_actions(run_stats& data);
