#include <materials.hh>

#include <n4-all.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("csi teflon reflectivity", "[csi][teflon][reflectivity]") {

  auto teflon = teflon_with_properties();
  auto csi    = csi_with_properties();
  auto air    = n4::material("G4_AIR");

  auto world = n4::box("world") .cube(10*mm).place(air   )          .now();
  auto outer = n4::box("teflon").cube( 9*mm).place(teflon).in(world).now();
  auto inner = n4::box("csi")   .cube( 8*mm).place(csi   ).in(outer).now();

  auto rm = n4::run_manager::create();

}
