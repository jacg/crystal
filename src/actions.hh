#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

std::function<void(G4Event*)> gammas_from_afar();

n4::actions* create_actions(run_stats& data);
