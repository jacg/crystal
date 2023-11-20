#include <cmath>
#include <config.hh>
#include <geometry.hh>
#include <materials.hh>
#include <physics-list.hh>

#include <n4-all.hh>

#include <G4PVPlacement.hh>
#include <G4Step.hh>
#include <G4TrackStatus.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// TODO test that teflon reflectivity is Lambertian not specular

TEST_CASE("csi teflon reflectivity fraction", "[csi][teflon][reflectivity]") {

  unsigned count_incoming = 0, count_reflected = 0;

  auto particle_name  = "opticalphoton";
  auto critical_angle = std::asin(1.35/1.79);
  auto energy         = 2.5 * eV;

  auto make_generator = [&] {
    auto particle_type    = n4::find_particle(particle_name);

    auto [x, y, z] = unpack(my.scint_size);

    auto min_theta = std::atan(std::hypot(x,y) / z);
    auto random_direction = n4::random::direction{}.min_theta(min_theta);

    return [&, z, particle_type, random_direction] (G4Event* event) {
      auto r = random_direction.get();
      auto particle = new G4PrimaryParticle{particle_type, r.x(), r.y(), r.z(), energy};
      particle -> SetPolarization(random_direction.get());
      auto vertex   = new G4PrimaryVertex{{0,0,-z/2}, 0};
      vertex -> SetPrimary(particle);
      event  -> AddPrimaryVertex(vertex);
    };
  };

  auto check_step = [&count_incoming, &count_reflected] (const G4Step* step) {
    auto track = step -> GetTrack();
    auto reflector = n4::find_logical("reflector");
    auto crystal   = n4::find_logical("crystal");
    if (track -> GetVolume() -> GetLogicalVolume() == reflector) {
      count_incoming++;
      if (track -> GetNextVolume() -> GetLogicalVolume() == crystal) { count_reflected++; }
      track -> SetTrackStatus(fStopAndKill);
    }
  };

  auto test_action = [&] {
    return (new n4::actions{make_generator()})
      -> set(new n4::stepping_action{check_step})
      ;
  };

  auto rm = n4::run_manager::create()
    .fake_ui()
    // .apply_command("/tracking/verbose 2")
    // .apply_command("/event/verbose 2")
    .physics(physics_list)
    .geometry(crystal_geometry)
    .actions(test_action)
    .run(100000);

  std::cout << "\n\n-------------------- PHOTONS HITTING BOUNDARY: " << count_incoming << "   reflected: " << count_reflected << std::endl;
  float teflon_reflectivity_percentage = 90; // TODO find correct value
  CHECK(100.0 * count_reflected / count_incoming > teflon_reflectivity_percentage);
}
