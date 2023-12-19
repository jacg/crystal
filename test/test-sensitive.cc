#include <config.hh>
#include <geometry.hh>
#include <materials.hh>
#include <physics-list.hh>

#include <n4-all.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinRel;

auto photons_along_z(auto energy) {
  auto particle_type = n4::find_particle("opticalphoton");
  auto source_z      = my.gel_thickness / 2; // Generate photon inside gel to avoid crystal-gel interface
  auto isotropic     = n4::random::direction{};
  return [energy, source_z, particle_type, isotropic] (G4Event* event) {
    auto particle = new G4PrimaryParticle{particle_type, 0, 0, energy};
    auto vertex   = new G4PrimaryVertex{{0,0,source_z}, 0};
    particle -> SetPolarization(isotropic.get());
    vertex -> SetPrimary(particle);
    event  -> AddPrimaryVertex(vertex);
  };
}

void check_pde_at_energy(double energy) {
  my.gel_thickness = 1 * nm; // Prevent absorption in gel from skewing the statistics
  run_stats stats;
  auto N = 50'000;
  auto [pde_energies, pde_values] = sipm_pde();
  auto pde_at_energy = n4::interpolator(std::move(pde_energies), std::move(pde_values))(energy).value();
  n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry([&] {return crystal_geometry(stats);})
    .actions(new n4::actions{photons_along_z(energy)})
    .run(N);

  auto fraction_detected = static_cast<double>(stats.n_detected_at_sipm[0]) / N;
  CHECK_THAT(fraction_detected, WithinRel(pde_at_energy, 1e-2));
}

TEST_CASE("sipm_sensitive_pde 2.5", "[sipm][sensitive][pde]") { check_pde_at_energy(2.5 * eV); }
TEST_CASE("sipm_sensitive_pde 3.5", "[sipm][sensitive][pde]") { check_pde_at_energy(3.5 * eV); }
TEST_CASE("sipm_sensitive_pde 4.0", "[sipm][sensitive][pde]") { check_pde_at_energy(4.0 * eV); }
TEST_CASE("sipm_sensitive_pde 4.2", "[sipm][sensitive][pde]") { check_pde_at_energy(4.2 * eV); }
