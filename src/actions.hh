#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

n4::generator::function gammas_from_afar();
n4::generator::function photoelectric_electrons();
n4::generator::function pointlike_photon_source(unsigned);

n4::actions* create_actions(run_stats& data);
