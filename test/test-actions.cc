#include "n4-sequences.hh"
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
#include <unordered_map>

using Catch::Matchers::WithinULP;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("gamma generator", "[generator][gamma]") {
  n4::test::default_run_manager().run(0);
  auto generator       = gammas_from_outside_crystal();
  auto is_given_energy = WithinULP(my.particle_energy(), 1);
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

G4ThreeVector range_of_vectors(auto positions) {
  auto [min_x, max_x] = n4::stats::min_max(n4::map<double>([] (G4ThreeVector v) {return v.x();}, positions)).value();
  auto [min_y, max_y] = n4::stats::min_max(n4::map<double>([] (G4ThreeVector v) {return v.y();}, positions)).value();
  auto [min_z, max_z] = n4::stats::min_max(n4::map<double>([] (G4ThreeVector v) {return v.z();}, positions)).value();
  return {
    max_x - min_x,
    max_y - min_y,
    max_z - min_z
  };
}

// Check particle type, energy, isotropic directions, and distribution of positions
TEST_CASE("electron generator", "[generator][electron]") {
  n4::test::default_run_manager().run(0);
  auto generator       = photoelectric_electrons();
  auto is_given_energy = WithinULP(my.particle_energy() - xe_kshell_binding_energy, 5);
  auto electron        = n4::find_particle("e-");
  auto [sx, sy, sz]    = n4::unpack(my.scint_size());

  G4ThreeVector              average_mom{};
  std::vector<G4ThreeVector> positions{};
  auto n_events = 10'000;
  for (auto i=0; i<n_events; i++) {
    G4Event event{};
    generator(&event);
    // Check that each event generates correct particle (e-) with correct energy
    REQUIRE(event  .  GetNumberOfPrimaryVertex() == 1); auto vertex = event.GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle     () == 1); auto particle = vertex -> GetPrimary(0);
    CHECK     (particle -> GetParticleDefinition()  == electron    );
    CHECK_THAT(particle -> GetKineticEnergy()     , is_given_energy);

    // Check isotropy of generated particles
    average_mom += particle -> GetMomentumDirection();
    positions.push_back(vertex -> GetPosition());
  }
  average_mom /= n_events;

  auto range = range_of_vectors(positions);

  auto is_close_to_zero = WithinAbs(0, 2e-2);
  // Isotropic directions should average out to zero
  CHECK_THAT(average_mom.x(), is_close_to_zero);
  CHECK_THAT(average_mom.y(), is_close_to_zero);
  CHECK_THAT(average_mom.z(), is_close_to_zero);
  // Uniform distribution within crystal should cover most of crystal size
  auto tol = 0.1;
  CHECK(range.x() > sx * (1 - tol));
  CHECK(range.y() > sy * (1 - tol));
  CHECK(range.z() > sz * (1 - tol));
}

// Check particle type, energy, isotropic directions, and distribution of positions
TEST_CASE("pointlike photon source generator", "[generator][photon][pointlike]") {
  n4::test::default_run_manager().run(0);

  // override particle energy
  G4UImanager::GetUIpointer() -> ApplyCommand("/my/particle_energy 1.234 eV");
  auto n_phot_per_event = 9876;
  auto generator        = pointlike_photon_source();
  auto optical_photon   = n4::find_particle("opticalphoton");
  auto is_given_energy  = WithinULP(my.particle_energy(), 2);
  auto [sx, sy, sz]     = n4::unpack(my.scint_size());

  G4UImanager::GetUIpointer() -> ApplyCommand("/source/nphotons " + std::to_string(n_phot_per_event));

  std::vector<G4ThreeVector> positions{};

  // Matcher combination outside macro does not work
  auto is_zero = WithinULP(0., 1);

  auto n_events = 10;
  for (auto i=0; i<n_events; i++) {
    G4ThreeVector average_mom{};
    G4Event event{};
    generator(&event);
    REQUIRE(event.GetNumberOfPrimaryVertex() == 1);
    auto vertex = event.GetPrimaryVertex(0);
    REQUIRE(vertex -> GetNumberOfParticle() == n_phot_per_event);
    for (auto j=0; j<n_phot_per_event; j++) {
      auto particle = vertex -> GetPrimary(j);
      // Check that each event generates correct particle (opt photon) with correct energy etc.
      CHECK     (particle -> GetParticleDefinition() == optical_photon);
      CHECK_THAT(particle -> GetKineticEnergy()      , is_given_energy);
      CHECK_THAT(particle -> GetTotalEnergy  ()      , is_given_energy);
      CHECK_THAT(particle -> GetMomentum     ().mag(), is_given_energy);
      average_mom += particle -> GetMomentumDirection();
    }
    average_mom /= n_phot_per_event;

    auto is_close_to_zero = WithinAbs(0, 2e-2);
    // Isotropic directions of photons should average out to zero
    CHECK_THAT(average_mom.x(), is_close_to_zero);
    CHECK_THAT(average_mom.y(), is_close_to_zero);
    CHECK_THAT(average_mom.z(), is_close_to_zero);

    positions.push_back(vertex -> GetPosition());
  }

  auto range = range_of_vectors(positions);
  auto tol = 0.3; // Events are costly, and low event count requires high tolerance
  // Uniform distribution within crystal should cover most of crystal size
  CHECK(range.x() > sx * (1 - tol));
  CHECK(range.y() > sy * (1 - tol));
  CHECK(range.z() > sz * (1 - tol));
}

// Check particle energy distribution
TEST_CASE("pointlike photon source generator spectrum", "[generator][photon][pointlike]") {
  n4::test::default_run_manager().run(0);

  auto n_phot_per_event = 1'000;
  auto generator        = pointlike_photon_source();

  G4UImanager::GetUIpointer() -> ApplyCommand("/my/fixed_energy false");
  G4UImanager::GetUIpointer() -> ApplyCommand("/source/nphotons " + std::to_string(n_phot_per_event));

  auto energies = n4::vec_with_capacity<double>(n_phot_per_event);

  G4Event event{};
  generator(&event);
  REQUIRE(event.GetNumberOfPrimaryVertex() == 1);
  auto vertex = event.GetPrimaryVertex(0);
  REQUIRE(vertex -> GetNumberOfParticle() == n_phot_per_event);
  for (auto j=0; j<n_phot_per_event; j++) { energies.push_back(vertex -> GetPrimary(j) -> GetTotalEnergy()); }

  auto [e_min, e_max] = n4::stats::min_max(energies).value();
  auto e_mean = n4::stats::mean(energies).value();
  CHECK_THAT(e_mean / eV, WithinRel(3.73, 1e-2));
  CHECK_THAT(e_min  / eV, WithinRel(2.70, 1e-2));
  CHECK_THAT(e_max  / eV, WithinRel(4.76, 1e-2));
}

TEST_CASE("test selector", "[selector]") {
  using generator_type = std::function<n4::generator::function(void)>;
  std::unordered_map<std::string, generator_type> options {
    {"gammas_from_outside_crystal", gammas_from_outside_crystal},
    {"gammas"                     , gammas_from_outside_crystal},
    {"photoelectric_electrons"    , photoelectric_electrons},
    {"electrons"                  , photoelectric_electrons},
    {"pointlike_photon_source"    , pointlike_photon_source},
    {"photons"                    , pointlike_photon_source}
  };

  auto fn_equal = [] (generator_type got, generator_type expected) {
    typedef n4::generator::function (fn_type)(void);
    size_t      got_address = (size_t)      *got.target<fn_type*>();
    size_t expected_address = (size_t) *expected.target<fn_type*>();
    return got_address == expected_address;
  };

  for (auto& [name, expected_fn] : options) {
    my.generator = name;
    CHECK(fn_equal(select_generator(), expected_fn));
  }

}
