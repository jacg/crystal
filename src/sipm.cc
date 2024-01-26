#include "sipm.hh"

#include <pet-materials.hh>

#include <n4-constants.hh>
#include <n4-sequences.hh>

using petmat::OPTPHOT_MIN_WL;
using petmat::OPTPHOT_MAX_WL;

std::pair<std::vector<double>, std::vector<double>> sipm_pde() {
  auto energies = n4::const_over(c4::hc/nm, {OPTPHOT_MAX_WL/nm, 809.722, 675.000, 587.500, 494.444, 455.556, 422.222, 395.833,
                                                       366.667, 344.444, 311.111, 293.056, 288.889, 279.167, OPTPHOT_MIN_WL/nm});
  auto pde      = n4::scale_by  (0.01     , {                0,   0.87 ,  19.2  ,  31.1  ,  46.7  ,  51.1  ,  50.2  ,  46.9  ,
                                                        40.6  ,  39.3  ,  32.4  ,  18.0  ,   4.8  ,   2.0  ,                 0});

  return {energies, pde};
}
