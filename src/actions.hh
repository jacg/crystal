#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

std::function<void(G4Event*)> gammas_from_afar();
std::function<void(G4Event*)> photoelectric_electrons();
std::function<void(G4Event*)> pointlike_photon_source(unsigned nphot);

n4::actions* create_actions(run_stats& data);
