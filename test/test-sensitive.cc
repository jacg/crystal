#include <config.hh>
#include <geometry.hh>
#include <materials.hh>
#include <physics-list.hh>

#include <n4-all.hh>

#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4Step.hh>
#include <G4ThreeVector.hh>
#include <G4TrackStatus.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;



auto blue_light_towards_sipm() {
  auto particle_type = n4::find_particle("opticalphoton");
  auto energy    = 2.5 * eV;
  auto source_z  = -1*um; // zero distance to avoid optical absorption in scintillator
  auto isotropic = n4::random::direction{};
  return [energy, source_z, particle_type, isotropic] (G4Event* event) {
    auto particle = new G4PrimaryParticle{particle_type, 0, 0, energy}; // along z axis
    particle -> SetPolarization(isotropic.get());
    auto vertex   = new G4PrimaryVertex{{0,0,source_z}, 0};
    vertex -> SetPrimary(particle);
    event  -> AddPrimaryVertex(vertex);
  };
}

TEST_CASE("crystal sipm sensitive", "[sipm][sensitive]") {
  unsigned n_shot     = 100000;
  unsigned n_detected = 0;
  n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry([&] { return crystal_geometry(n_detected);})
    .actions(new n4::actions{blue_light_towards_sipm()})
    .run(n_shot);

  auto fraction_detected = static_cast<double>(n_detected) / n_shot;
  CHECK_THAT(fraction_detected, WithinAbs(1, 1e-6));
}
