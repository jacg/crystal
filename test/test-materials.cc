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

#include <cmath>

// TODO test that teflon reflectivity is Lambertian not specular

TEST_CASE("csi teflon reflectivity fraction", "[csi][teflon][reflectivity]") {

  unsigned count_incoming = 0, count_reflected = 0;

  auto particle_name  = "opticalphoton";
  auto energy         = 2.5 * eV;

  auto make_generator = [&] {
    auto particle_type    = n4::find_particle(particle_name);

    auto [x, y, z] = unpack(my.scint_size);

    auto min_theta = std::atan(std::hypot(x,y) / z);
    auto random_direction = n4::random::direction{}.min_theta(min_theta);

    return [energy, z, particle_type, random_direction] (G4Event* event) {
      auto r        = random_direction.get() * energy;
      auto particle = new G4PrimaryParticle{particle_type, r.x(), r.y(), r.z()};
      particle -> SetPolarization(random_direction.get());
      auto vertex   = new G4PrimaryVertex{{0,0,-z/2}, 0};
      vertex -> SetPrimary(particle);
      event  -> AddPrimaryVertex(vertex);
    };
  };

  std::vector<G4LogicalVolume*> step_volumes;

  auto record_step_volumes = [&step_volumes] (const G4Step* step) {
    auto track = step -> GetTrack();
    auto next_volume = track -> GetNextVolume() -> GetLogicalVolume();
    step_volumes.push_back(next_volume);
    if (step_volumes.size() >= 2) { track -> SetTrackStatus(fStopAndKill); } // The first 2 steps tell us all we need to know
  };

  // Check whether step exists and if so, whether its value is as specified
  auto step_matches = [&step_volumes] (size_t pos, G4LogicalVolume* value) {
    return step_volumes.size() > pos && step_volumes[pos] == value;
  };

  auto reset_step_list = [&step_volumes] (const G4Event*) { step_volumes.clear(); };
  auto classify_events = [&] (const G4Event*) {
    auto reflector = n4::find_logical("reflector");
    auto crystal   = n4::find_logical("crystal");
    if (step_matches(0, reflector)) { count_incoming ++; } else { return; }
    if (step_matches(1, crystal  )) { count_reflected++; }
  };

  auto test_action = [&] {
    return (new n4::actions{make_generator()})
      -> set((new n4::event_action)
             -> begin(reset_step_list)
             ->   end(classify_events))
      -> set(new n4::stepping_action{record_step_volumes})
      ;
  };

  n4::run_manager::create()
    .fake_ui()
    // .apply_command("/tracking/verbose 2")
    // .apply_command("/event/verbose 2")
    .physics(physics_list)
    .geometry(crystal_geometry)
    .actions(test_action)
    .run(100000);

  auto teflon_reflectivity_percentage   = 90; // TODO find correct value
  auto measured_reflectivity_percentage = 100.0 * count_reflected / count_incoming;

  std::cout << "\n\n-------------------- PHOTONS HITTING BOUNDARY    : " << count_incoming
            << "   reflected: " << count_reflected << "   ratio: " <<  measured_reflectivity_percentage << " %" << std::endl;

  CHECK(measured_reflectivity_percentage > teflon_reflectivity_percentage);
}
