#include <actions.hh>
#include <config.hh>
#include <geometry.hh>

#include <n4-all.hh>

#include <G4LogicalVolume.hh>
#include <G4UImanager.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

using Catch::Matchers::WithinULP;
using Catch::Matchers::WithinAbs;

TEST_CASE("gamma generator", "[generator][gamma]") {
  n4::test::default_run_manager().run(0);
  auto generator       = gammas_from_afar();
  auto is_given_energy = WithinULP(my.particle_energy, 1);
  auto gamma           = n4::find_particle("gamma");

  for (auto i=0; i<1'000; i++) {
    G4Event event{};
    generator(&event);
    REQUIRE(event.GetNumberOfPrimaryVertex() == 1);
    auto vertex = event.GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle() == 1);
    auto particle = vertex -> GetPrimary(0);
    CHECK     (particle -> GetParticleDefinition() == gamma);
    CHECK_THAT(particle -> GetKineticEnergy()      , is_given_energy);
    CHECK_THAT(particle -> GetTotalEnergy  ()      , is_given_energy);
    CHECK_THAT(particle -> GetMomentum     ().mag(), is_given_energy);
  }
}


TEST_CASE("electron generator", "[generator][electron]") {
  n4::test::default_run_manager().run(0);
  auto generator       = photoelectric_electrons();
  auto is_given_energy = WithinULP(my.particle_energy - 34.56*keV, 5);
  auto electron        = n4::find_particle("e-");

  G4ThreeVector average_p{};
  auto n_events = 10'000;
  for (auto i=0; i<n_events; i++) {
    G4Event event{};
    generator(&event);
    REQUIRE(event.GetNumberOfPrimaryVertex() == 1);
    auto vertex = event.GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle() == 1);
    auto particle = vertex -> GetPrimary(0);
    CHECK     (particle -> GetParticleDefinition()  == electron    );
    CHECK_THAT(particle -> GetKineticEnergy()     , is_given_energy);

    average_p += particle -> GetMomentumDirection();
  }
  average_p /= n_events;

  auto is_close_to_zero = WithinAbs(0, 2e-2);
  CHECK_THAT(average_p.x(), is_close_to_zero);
  CHECK_THAT(average_p.y(), is_close_to_zero);
  CHECK_THAT(average_p.z(), is_close_to_zero);
}


TEST_CASE("pointlike photon source generator", "[generator][photon][pointlike]") {
  n4::test::default_run_manager().run(0);

  // override particle energy
  my.particle_energy    = 1.234 * eV;
  auto n_phot_per_event = 9876;
  auto generator        = pointlike_photon_source();
  auto optical_photon   = n4::find_particle("opticalphoton");
  auto is_given_energy  = WithinULP(my.particle_energy, 2);
  auto is_close_to_zero = WithinAbs(0, 2e-2);

  G4UImanager::GetUIpointer() -> ApplyCommand("/source/nphotons " + std::to_string(n_phot_per_event));

  auto n_events = 10;
  for (auto i=0; i<n_events; i++) {
    G4ThreeVector average_p{};
    G4Event event{};
    generator(&event);
    REQUIRE(event.GetNumberOfPrimaryVertex() == 1);
    auto vertex = event.GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle() == n_phot_per_event);
    for (auto j=0; j<n_phot_per_event; j++) {
      auto particle = vertex -> GetPrimary(j);
      CHECK     (particle -> GetParticleDefinition() == optical_photon);
      CHECK_THAT(particle -> GetKineticEnergy()      , is_given_energy);
      CHECK_THAT(particle -> GetTotalEnergy  ()      , is_given_energy);
      CHECK_THAT(particle -> GetMomentum     ().mag(), is_given_energy);
      average_p += particle -> GetMomentumDirection();
    }
    average_p /= n_phot_per_event;

    CHECK_THAT(average_p.x(), is_close_to_zero);
    CHECK_THAT(average_p.y(), is_close_to_zero);
    CHECK_THAT(average_p.z(), is_close_to_zero);
  }
}
