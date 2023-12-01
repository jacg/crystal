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


// TODO: see if this warning can be removed
//
// -------- WWWW ------- G4Exception-START -------- WWWW -------
// *** G4Exception : DET1010
//       issued by : G4SDStructure::AddNewDetector()
// sipm had already been stored in /. Object pointer is overwritten.
// It's users' responsibility to delete the old sensitive detector object.
// *** This is just a warning message. ***
// -------- WWWW -------- G4Exception-END --------- WWWW -------


TEST_CASE("geometry crystal size", "[geometry][default]") {
  run_stats stats;
  auto sipm_size = 1.2345*mm;

  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/sipm_size " + std::to_string(sipm_size/mm) + " mm");

  // Only do this once. The run manager is needed to use
  // n4::clear_geometry, but it doesn't play any other role.  However,
  // because of the way Catch2 does Sections, it will attempt to
  // create the run manager every time. This is one way to avoid it.
  if (!n4::run_manager::available()) {
    n4::test::default_run_manager().run(0);
  }

  auto check_crystal = [] (auto x, auto y, auto z) {
    REQUIRE(n4::find_solid   ("crystal") != nullptr);
    REQUIRE(n4::find_logical ("crystal") != nullptr);
    REQUIRE(n4::find_physical("crystal") != nullptr);
    auto crystal = n4::find_solid<G4Box>("crystal");

    CHECK_THAT(crystal -> GetXHalfLength(), WithinULP( x/2, 1));
    CHECK_THAT(crystal -> GetYHalfLength(), WithinULP( y/2, 1));
    CHECK_THAT(crystal -> GetZHalfLength(), WithinULP( z/2, 1));
  };

  auto check_sipms = [sipm_size] (auto n_sipms_x, auto n_sipms_y) {
    REQUIRE(n4::find_solid("sipm") != nullptr);
    auto sipm_solid   = n4::find_solid<G4Box>("sipm");

    CHECK_THAT(sipm_solid -> GetXHalfLength(), WithinULP( sipm_size/2, 1));
    CHECK_THAT(sipm_solid -> GetYHalfLength(), WithinULP( sipm_size/2, 1));

    auto sipm_logical = n4::find_logical("sipm");
    REQUIRE(sipm_logical != nullptr);
    CHECK(sipm_logical -> GetSensitiveDetector() != nullptr);

    // Check that there are exactly n_sipms
    auto n_sipms = n_sipms_x * n_sipms_y;
    for (auto i=0; i<n_sipms; i++) {
      CHECK(n4::find_physical("sipm-" + std::to_string(i)) != nullptr);
    }
    // For some reason, the following line doesn't return a nullptr,
    // even though it says it is doing so in a warning message (change
    // true -> false to silence the warning). This makes me worry that
    // the previous CHECK doesn't really count as a valid test.
    // CHECK(n4::find_physical("sipm-" + std::to_string(n_sipms), true) == nullptr);
  };


  auto size_from_params = [sipm_size] (const scint_parameters& params) -> std::tuple<double, double, double> {
    return { params.n_sipms_x*sipm_size
           , params.n_sipms_y*sipm_size
           , params.scint_depth};
  };

#define TEST_CONFIG(NAME)                            \
  SECTION(#NAME) {                                   \
    n4::clear_geometry();                            \
    UI -> ApplyCommand("/my/config_type " #NAME);    \
    crystal_geometry(stats);                         \
    auto params = my.scint_params();                 \
    auto [x,y,z] = size_from_params(params);         \
    check_crystal(x, y, z);                          \
    check_sipms(params.n_sipms_x, params.n_sipms_y); \
  }

  TEST_CONFIG(csi);
  TEST_CONFIG(csi_mono);
  TEST_CONFIG(bgo);
  TEST_CONFIG(lyso);

#undef TEST_CONFIG

  SECTION("Custom") {
    n4::clear_geometry();
    auto n_sipms_x = 13;
    auto n_sipms_y =  7;
    auto size_x = n_sipms_x * sipm_size;
    auto size_y = n_sipms_y * sipm_size;
    auto size_z = 4.2 * mm;

    UI -> ApplyCommand("/my/config_type CsI");
    UI -> ApplyCommand("/my/scint       LYSO");
    UI -> ApplyCommand("/my/scint_depth " + std::to_string(size_z   ) + " mm");
    UI -> ApplyCommand("/my/n_sipms_x   " + std::to_string(n_sipms_x)        );
    UI -> ApplyCommand("/my/n_sipms_y   " + std::to_string(n_sipms_y)        );
    UI -> ApplyCommand("/my/sipm_size   " + std::to_string(sipm_size) + " mm");

    crystal_geometry(stats);
    check_crystal(size_x, size_y, size_z);
    check_sipms(n_sipms_x, n_sipms_y);
  }

}
