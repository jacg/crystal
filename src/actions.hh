#pragma once

#include <n4-mandatory.hh>

auto my_generator();

n4::actions* create_actions(unsigned& n_event, unsigned& n_detected_evt, std::vector<unsigned>& n_detected_run);
