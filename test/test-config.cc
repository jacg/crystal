#include <G4UImanager.hh>

#include <config.hh>

#include <n4-sequences.hh>

#include <set>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinULP;

TEST_CASE("config has a sane default", "[config][default]") {
  auto params = my.scint_params();
  CHECK(params.scint_depth > 0);
  CHECK(params.n_sipms_x   > 0);
  CHECK(params.n_sipms_y   > 0);
  CHECK(params.sipm_size   > 0);

  auto scint_size = my.scint_size();
  CHECK(scint_size.x() > 0);
  CHECK(scint_size.y() > 0);
  CHECK(scint_size.z() > 0);

  CHECK(my.     sipm_thickness > 0);
  CHECK(my.reflector_thickness > 0);

  CHECK(my.particle_energy() > 0);
  CHECK(my.seed > 0);

  CHECK(! my.scint_yield .has_value());
  CHECK(! my.reflectivity.has_value());

  CHECK(my.generator != "");
  CHECK(my.outfile   != "");

  CHECK(my.chunk_size > 0);

  CHECK(my.n_sipms() > 0);

  auto sipm_pos = my.sipm_positions();
  CHECK(sipm_pos.size() == my.n_sipms());
}


TEST_CASE("scint params overrides", "[config]") {
  // Values that will eventually override the defaults
  auto params0  = my.scint_params();
  auto depth     = 1.234 * cm;
  auto nx        = 17;
  auto ny        =  5;
  auto sipm_size = 42 * mm;
  auto scint     = "bgo";

  // Check that the original values are *different* from the overrides
  REQUIRE     (params0.scint        != scintillator_type_enum::bgo);
  REQUIRE_THAT(params0.scint_depth, !  WithinULP(depth, 1));
  REQUIRE     (params0.n_sipms_x    != nx);
  REQUIRE     (params0.n_sipms_y    != ny);
  REQUIRE_THAT(params0.sipm_size  , !  WithinULP(sipm_size, 1));

  // Use messengers to override defaults
  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/scint "       + std::string{scint});
  UI -> ApplyCommand("/my/scint_depth " + std::to_string(depth/mm) + " mm");
  UI -> ApplyCommand("/my/n_sipms_x "   + std::to_string(nx));
  UI -> ApplyCommand("/my/n_sipms_y "   + std::to_string(ny));
  UI -> ApplyCommand("/my/sipm_size "   + std::to_string(sipm_size/mm) + " mm");

  // Check that new values match what was set by the messengers
  auto params1 = my.scint_params();
  CHECK     (params1.scint      == scintillator_type_enum::bgo);
  CHECK_THAT(params1.scint_depth,  WithinULP(depth, 1));
  CHECK     (params1.n_sipms_x  == nx);
  CHECK     (params1.n_sipms_y  == ny);
  CHECK_THAT(params1.sipm_size  ,  WithinULP(sipm_size, 1));

  auto nxy = 13;
  UI -> ApplyCommand("/my/n_sipms_xy " + std::to_string(nxy));
  auto params2 = my.scint_params();
  CHECK(params2.n_sipms_x == nxy);
  CHECK(params2.n_sipms_y == nxy);
}

TEST_CASE("number of sipms is updated", "[config]") {
  auto pos0 = my.sipm_positions();
  auto UI   = G4UImanager::GetUIpointer();

  UI -> ApplyCommand("/my/n_sipms_x 2"); // Default was 1, so this doubles no. of sipms
  auto pos1 = my.sipm_positions();
  CHECK(pos1.size() == 2*pos0.size());

  UI -> ApplyCommand("/my/n_sipms_y 3"); // Default was 1, so this triples no. of sipms
  auto pos2 = my.sipm_positions();
  CHECK(pos2.size() == 3*pos1.size());

  UI -> ApplyCommand("/my/n_sipms_xy 6"); // pos2 contained 2x3, so this multiplies no. sipms by 6
  auto pos3 = my.sipm_positions();
  CHECK(pos3.size() == 6*pos2.size());

  auto current_sipm_size = my.scint_params().sipm_size / mm;
  UI -> ApplyCommand("/my/sipm_size " + std::to_string(current_sipm_size/4) + " mm");
  auto pos4 = my.sipm_positions();
  CHECK(pos4.size() == pos3.size()); // Changing size doesn't change the number of sipms
}

TEST_CASE("sipm positions are consistent", "[config]") {
  auto nx =  7;
  auto ny = 13;
  auto n  = nx*ny;
  auto sipm_size = 5*mm;

  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/n_sipms_x " + std::to_string(nx));
  UI -> ApplyCommand("/my/n_sipms_y " + std::to_string(ny));
  UI -> ApplyCommand("/my/sipm_size " + std::to_string(sipm_size/mm) + " mm");

  auto pos = my.sipm_positions();
  CHECK(pos.size()   == n);
  CHECK(my.n_sipms() == n);

  std::set<double> xset, yset, zset;
  for (const auto& p : pos) {
    xset.insert(p.x());
    yset.insert(p.y());
    zset.insert(p.z());
  }

  CHECK(xset.size() == nx);
  CHECK(yset.size() == ny);
  CHECK(zset.size() ==  1);

  std::vector<double> x{cbegin(xset), cend(xset)};
  std::vector<double> y{cbegin(yset), cend(yset)};
  std::vector<double> z{cbegin(zset), cend(zset)};

  for (auto i=1; i<x.size(); i++) { CHECK_THAT(std::abs(x[i] - x[i-1]), WithinULP(sipm_size, 1)); }
  for (auto i=1; i<y.size(); i++) { CHECK_THAT(std::abs(y[i] - y[i-1]), WithinULP(sipm_size, 1)); }
}
