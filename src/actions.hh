#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

n4::generator::function gammas_from_outside_crystal();
n4::generator::function photoelectric_electrons();
n4::generator::function pointlike_photon_source();

std::function<n4::generator::function((void))> select_generator();

n4::actions* create_actions(run_stats& data);

extern const double xe_kshell_binding_energy;
