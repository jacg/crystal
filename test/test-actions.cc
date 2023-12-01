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

TEST_CASE("gamma generator", "[generator][gamma]") {
  n4::test::default_run_manager().run(0);
  auto generator       = gammas_from_afar();
  auto is_given_energy = WithinULP(my.particle_energy, 1);
  auto gamma           = n4::find_particle("gamma");

  for (auto i=0; i<1'000; i++) {
    auto event = new G4Event();
    generator(event);
    REQUIRE(event -> GetNumberOfPrimaryVertex() == 1);
    auto vertex = event -> GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle() == 1);
    auto particle = vertex -> GetPrimary(0);
    CHECK     (particle -> GetParticleDefinition() == gamma);
    CHECK_THAT(particle -> GetKineticEnergy()      , is_given_energy);
    CHECK_THAT(particle -> GetTotalEnergy  ()      , is_given_energy);
    CHECK_THAT(particle -> GetMomentum     ().mag(), is_given_energy);
  }
}
