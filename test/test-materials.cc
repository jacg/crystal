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

  auto teflon = teflon_with_properties();
  auto csi    = csi_with_properties();
  auto air    = air_with_properties();

  auto world     = n4::box("world")       .xy(100*mm).z(30*mm).place(air   )              .now();
  auto reflector = n4::box("reflector")   .xy( 90*mm).z(20*mm).place(teflon).in(world)    .now();
  auto scint     = n4::box("scintillator").xy( 80*mm).z(10*mm).place(csi   ).in(reflector).now();

  unsigned count_incoming = 0, count_reflected = 0;

  auto geometry = [teflon, world] {
    return world;
  };

  auto particle_name  = "opticalphoton";
  auto critical_angle = std::asin(1.35/1.79);
  auto energy         = 2.5 * eV;

  auto make_generator = [&] {
    auto particle_type    = n4::find_particle(particle_name);
    auto random_direction = n4::random::direction();
    return [&, particle_type, random_direction] (G4Event* event) {
      auto r = random_direction.get();
      auto particle = new G4PrimaryParticle{particle_type, r.x(), r.y(), r.z(), energy};
      particle -> SetPolarization(random_direction.get());
      auto vertex   = new G4PrimaryVertex{{}, 0};
      vertex -> SetPrimary(particle);
      event  -> AddPrimaryVertex(vertex);
    };
  };

  auto check_step = [&count_incoming, &count_reflected, reflector, scint] (const G4Step* step) {
    auto track = step -> GetTrack();
    if (dynamic_cast<G4PVPlacement*>(track -> GetVolume()) == reflector) {
      count_incoming++;
      if (dynamic_cast<G4PVPlacement*>(track->GetNextVolume()) == scint) { count_reflected++; }
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
    .geometry(geometry)
    .actions(test_action)
    .run(100000);

  std::cout << "photons hitting boundary: " << count_incoming << "   reflected: " << count_reflected << std::endl;
  float teflon_reflectivity_percentage = 90; // TODO find correct value
  CHECK(100.0 * count_reflected / count_incoming > teflon_reflectivity_percentage);
}
