#include <config.hh>
#include <geometry.hh>
#include <physics-list.hh>

#include <n4-all.hh>

#include <G4Step.hh>
#include <G4ThreeVector.hh>
#include <G4TrackStatus.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;


auto blue_light_towards_sipm() {
  auto optical_photon = n4::find_particle("opticalphoton");
  auto energy    = 2.5 * eV;
  auto isotropic = n4::random::direction{};
  auto random_source_pos = [] () -> G4ThreeVector {
    auto params = my.scint_params();
    return {
      n4::random::uniform_width(params.sipm_size),
      n4::random::uniform_width(params.sipm_size),
      -1 * um // almost zero distance to avoid optical absorption in scintillator
    };
  };

  return [energy, random_source_pos, optical_photon, isotropic] (G4Event* event) {
    auto particle = new G4PrimaryParticle{optical_photon, 0, 0, energy}; // along z axis
    particle -> SetPolarization(isotropic.get());
    auto vertex   = new G4PrimaryVertex{random_source_pos(), 0};
    vertex -> SetPrimary(particle);
    event  -> AddPrimaryVertex(vertex);
  };
}

TEST_CASE("crystal sipm sensitive", "[sipm][sensitive]") {
  unsigned n_shot     = 100000;
  run_stats stats;
  n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry([&] { return crystal_geometry(stats); })
    .actions(new n4::actions{blue_light_towards_sipm()})
    .run(n_shot);

  auto fraction_detected = static_cast<double>(stats.n_detected_evt) / n_shot;
  CHECK_THAT(fraction_detected, WithinAbs(1, 1e-6));
}
