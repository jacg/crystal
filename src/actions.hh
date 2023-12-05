#pragma once

#include "run_stats.hh"

#include <n4-mandatory.hh>

n4::generator::function gammas_from_outside_crystal();
n4::generator::function photoelectric_electrons();
n4::generator::function pointlike_photon_source();

std::function<n4::generator::function((void))> select_generator();

n4::actions* create_actions(run_stats& data);


class csv_writer {
public:
  csv_writer();
  void write(const G4ThreeVector& pos, const std::unordered_map<size_t, size_t>& counts);
private:
  std::ofstream out;
  size_t        number_of_sipms;
};
