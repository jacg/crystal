#include <materials.hh>
#include <physics-list.hh>

#include <n4-all.hh>

#include <G4Step.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("csi teflon reflectivity", "[csi][teflon][reflectivity]") {

  auto geometry = [] {
    auto teflon = teflon_with_properties();
    auto csi    = csi_with_properties();
    auto air    = n4::material("G4_AIR");

    auto world = n4::box("world") .cube(10*mm).place(air   )          .now();
    auto outer = n4::box("teflon").cube( 9*mm).place(teflon).in(world).now();
    auto inner = n4::box("csi")   .cube( 8*mm).place(csi   ).in(outer).now();
    return world;
  };

  auto generator = [] (G4Event* event) {

  };

  auto check_step = [] (const G4Step* step) {

  };

  auto test_action = [&] {
    auto actions = (new n4::actions{generator})
      -> set(new n4::stepping_action{check_step})

  };

  auto rm = n4::run_manager::create()
    .fake_ui()
    .physics(physics_list)
    .geometry(geometry)
    .actions(test_action)
    .run();

}
