#include <config.hh>
#include <geometry.hh>
#include <materials.hh>
#include <physics-list.hh>

#include <n4-all.hh>
#include <n4-will-become-external-lib.hh>

#include <G4ClassificationOfNewTrack.hh>
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



auto blue_light_towards_teflon() {
  auto particle_type = n4::find_particle("opticalphoton");
  auto energy        = 2.5 * eV;
  auto [x, y, z] = n4::unpack(my.scint_size());
  auto source_z  = -z/2;

  // Avoid the SiPM face which is not covered by teflon
  auto min_theta = std::atan(std::hypot(x,y) / -source_z);
  auto towards_teflon = n4::random::direction{}.min_theta(min_theta);
  auto isotropic      = n4::random::direction{};

  return [energy, source_z, particle_type, towards_teflon, isotropic] (G4Event* event) {
    auto r        = towards_teflon.get() * energy;
    auto particle = new G4PrimaryParticle{particle_type, r.x(), r.y(), r.z()};
    particle -> SetPolarization(isotropic.get());
    auto vertex   = new G4PrimaryVertex{{0,0,source_z}, 0};
    vertex -> SetPrimary(particle);
    event  -> AddPrimaryVertex(vertex);
  };
}

// Check whether step exists and if so, whether its value is as specified
bool step_matches(std::vector<G4LogicalVolume*>& step_volumes, size_t pos, G4LogicalVolume* value) {
    return step_volumes.size() > pos && step_volumes[pos] == value;
  };

// Shoot photons from within crystal in random directions towards teflon
// reflector (avoiding the SiPM face which is not covered by teflon). Count
// events in which the photon reaches the teflon in the first step. Count how
// many of those result in the photon returning into the crystal in the next
// step. Check that the ratio of these counts is equal to the expected
// reflectivity.
TEST_CASE("csi teflon reflectivity fraction", "[csi][teflon][reflectivity]") {
  unsigned count_incoming = 0, count_reflected = 0;

  std::vector<G4LogicalVolume*> step_volumes; // Space for storing the first and second volumes to be stepped through

  // Stepping action: record the volumes visited in the steps
  auto record_step_volumes = [&step_volumes] (const G4Step* step) {
    auto track = step -> GetTrack();
    auto next_volume = track -> GetNextVolume() -> GetLogicalVolume();
    step_volumes.push_back(next_volume);
    if (step_volumes.size() >= 2) { track -> SetTrackStatus(fStopAndKill); } // The first 2 steps tell us all we need to know
  };

  // Begin of event action: forget volumes visited in the previous event
  auto reset_step_list = [&step_volumes] (const G4Event*) { step_volumes.clear(); };

  // End of event action: count events where
  // 1. reflector was reached in first step
  // 2. second step resulted in reflection
  auto classify_events = [&] (const G4Event*) {
    auto reflector = n4::find_logical("reflector");
    auto crystal   = n4::find_logical("crystal");
    if (step_matches(step_volumes, 0, reflector)) { count_incoming ++; } else { return; }
    if (step_matches(step_volumes, 1, crystal  )) { count_reflected++; }
  };

  // Gather together all the above actions
  auto test_action = [&] {
    return (new n4::actions{blue_light_towards_teflon()})
      -> set((new n4::event_action)
             -> begin(reset_step_list)
             ->   end(classify_events))
      -> set(new n4::stepping_action{record_step_volumes})
      ;
  };

  unsigned dummy=0;
  n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry([&] {return crystal_geometry(dummy);})
    .actions(test_action)
    .run(100000);

  auto teflon_reflectivity_percentage   = 98;
  auto measured_reflectivity_percentage = 100.0 * count_reflected / count_incoming;

  std::cout << "\n\n-------------------- PHOTONS HITTING BOUNDARY    : " << count_incoming
            << "   reflected: " << count_reflected << "   ratio: " <<  measured_reflectivity_percentage << " %" << std::endl;

  CHECK_THAT(measured_reflectivity_percentage, WithinRel(teflon_reflectivity_percentage, 1e-3));
}

// Shoot photons from within crystal in random directions towards teflon
// reflector (avoiding the SiPM face which is not covered by teflon). Ignore
// events in which the photon does not reach the teflon in the first step or
// does not get reflected in the second. Record the angle of incidence and angle
// of reflection. Calculate the correlation between these angles and check that
// it is ZERO.
//
// To see the test detecting non-Lambertian reflection, change
//
// - teflon_surface -> SetFinish(groundfrontpainted);    // Lambertian
// + teflon_surface -> SetFinish(polishedfrontpainted);  // Specular
//
// in geometry.cc.
TEST_CASE("teflon reflectivity lambertian", "[teflon][reflectivity]") {
  std::vector<G4LogicalVolume*> step_volumes;
  std::vector<G4double> step_thetas;
  std::vector<G4double> thetas_in, thetas_out;

  // The normal to the surface at which the photon is reflected
  auto find_normal = [] (const auto& pos) {
    auto [sx, sy, sz] = n4::unpack(my.scint_size());
    return WithinRel(-sz  , 1e-6).match(pos.z()) ? G4ThreeVector{ 0,  0,  1} :
           WithinRel(-sx/2, 1e-6).match(pos.x()) ? G4ThreeVector{ 1,  0,  0} :
           WithinRel( sx/2, 1e-6).match(pos.x()) ? G4ThreeVector{-1,  0,  0} :
           WithinRel(-sy/2, 1e-6).match(pos.y()) ? G4ThreeVector{ 0,  1,  0} :
           WithinRel( sy/2, 1e-6).match(pos.y()) ? G4ThreeVector{ 0, -1,  0} :
                                                   G4ThreeVector{ 0,  0, -1} ; // Meaningless
  };

  auto record_data = [&step_volumes, &step_thetas, &find_normal] (const G4Step* step) {
    auto track = step -> GetTrack();
    auto step_number = track -> GetCurrentStepNumber();
    if ( step_number < 3 ) {
      auto next_volume = track -> GetNextVolume() -> GetLogicalVolume();
      step_volumes.push_back(next_volume);
      auto point = step_number == 1 ?
        step -> GetPostStepPoint() :
        step ->  GetPreStepPoint() ;
      auto pos = point -> GetPosition();
      auto n = find_normal(pos);
      auto p = step -> GetPreStepPoint() -> GetMomentumDirection();
      auto theta = std::acos(p.dot(n));
      step_thetas.push_back(theta);
    }
    else {
      step -> GetTrack() -> SetTrackStatus(fStopAndKill);
    }
  };

  auto reset_data = [&step_volumes, &step_thetas] (const G4Event*) {
    step_volumes.clear();
    step_thetas .clear();
  };

  auto check_validity = [&step_volumes, &step_thetas, &thetas_in, &thetas_out] (const G4Event* event) {
    auto crystal   = n4::find_logical("crystal");
    auto reflector = n4::find_logical("reflector");
    if (step_matches(step_volumes, 0, reflector) &&
        step_matches(step_volumes, 1, crystal)) {
      thetas_in .push_back(step_thetas[0]);
      thetas_out.push_back(step_thetas[1]);
    }
  };

  auto test_action = [&] {
    return (  new n4::actions{blue_light_towards_teflon()})
      -> set((new n4::   event_action{}) -> begin(reset_data) -> end(check_validity))
      -> set( new n4::stepping_action{record_data})
      ;
  };

  unsigned dummy=0;
  n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry([&] {return crystal_geometry(dummy);})
    .actions(test_action)
    .run(100'000);

  auto corr = n4::stats::correlation(thetas_in, thetas_out).value();
  std::cerr << "------------------------------ CORRELATION: " << corr << std::endl;
  CHECK_THAT(corr, WithinAbs(0, 1e-2));
}

TEST_CASE("CsI abslength", "[material][csi][abslength]") {
  auto csi = csi_with_properties();
  CHECK_THAT( csi -> GetDensity() / (g / cm3), WithinRel(4.51, 1e-6));

  auto abs_lengths = measure_abslength(test_config{ .physics         = physics_list()
                                                  , .material        = csi
                                                  , .particle_name   = "gamma"
                                                  , .particle_energy = 511 * keV
                                                  , .distances       = n4::scale_by(mm, {5, 10, 15, 20, 25, 30, 35, 40, 45, 50})});

  auto expected_abs_length = 24.9 * mm;
  for (auto abs_length : abs_lengths) {
    CHECK_THAT(abs_length, WithinRel(expected_abs_length, 0.05));
  }
}

TEST_CASE("BGO abslength", "[material][bgo][abslength]") {
  auto bgo = bgo_with_properties();
  CHECK_THAT( bgo -> GetDensity() / (g / cm3), WithinRel(7.13, 1e-6));

  auto abs_lengths = measure_abslength(test_config{ .physics         = physics_list()
                                                  , .material        = bgo
                                                  , .particle_name   = "gamma"
                                                  , .particle_energy = 511 * keV
                                                  , .distances       = n4::scale_by(mm, {5, 10, 15, 20, 25, 30, 35, 40, 45, 50})});

  auto expected_abs_length = 11.4 * mm;
  for (auto abs_length : abs_lengths) {
    CHECK_THAT(abs_length, WithinRel(expected_abs_length, 0.05));
  }
}

TEST_CASE("LYSO abslength", "[material][lyso][abslength]") {
  auto lyso = lyso_with_properties();
  CHECK_THAT( lyso -> GetDensity() / (g / cm3), WithinRel(7.1, 1e-6));

  auto abs_lengths = measure_abslength(test_config{ .physics         = physics_list()
                                                  , .material        = lyso
                                                  , .particle_name   = "gamma"
                                                  , .particle_energy = 511 * keV
                                                  , .distances       = n4::scale_by(mm, {5, 10, 15, 20, 25, 30, 35, 40, 45, 50})});

  auto expected_abs_length = 13.0 * mm;
  for (auto abs_length : abs_lengths) {
    CHECK_THAT(abs_length, WithinRel(expected_abs_length, 0.05));
  }
}

TEST_CASE("csi interaction process fractions", "[csi][interaction]") {
  auto fractions = calculate_interaction_process_fractions(csi_with_properties(), physics_list());
  CHECK_THAT(fractions.photoelectric, WithinRel(0.207, 1e-2));
  CHECK_THAT(fractions.compton      , WithinRel(0.740, 1e-2));
  CHECK_THAT(fractions.rayleigh     , WithinRel(0.053, 1e-2));
}

TEST_CASE("bgo interaction process fractions", "[bgo][interaction]") {
  auto fractions = calculate_interaction_process_fractions(bgo_with_properties(), physics_list());
  CHECK_THAT(fractions.photoelectric, WithinRel(0.414, 1e-2));
  CHECK_THAT(fractions.compton      , WithinRel(0.532, 1e-2));
  CHECK_THAT(fractions.rayleigh     , WithinRel(0.054, 1e-2));
}

TEST_CASE("lyso interaction process fractions", "[lyso][interaction]") {
  auto fractions = calculate_interaction_process_fractions(lyso_with_properties(), physics_list());
  CHECK_THAT(fractions.photoelectric, WithinRel(0.311, 1e-2));
  CHECK_THAT(fractions.compton      , WithinRel(0.637, 1e-2));
  CHECK_THAT(fractions.rayleigh     , WithinRel(0.052, 1e-2));
}
