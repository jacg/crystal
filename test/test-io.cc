#include <actions.hh>
#include <geometry.hh>
#include <physics-list.hh>
#include <run_stats.hh>
#include <config.hh>
#include <io.hh>

#include <n4-all.hh>

#include <G4UImanager.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstdio>
#include <unordered_map>

using Catch::Matchers::WithinULP;

void read_and_check(const auto& filename, const auto& source_pos, const auto& sipm_ids, const auto& counts) {
  // Read data
  auto maybe_data = read_entire_file(filename);
  REQUIRE(maybe_data.ok());
  auto rows = maybe_data.ValueOrDie();

  // Check values
  // Sizes and element presence are checked before values using
  // REQUIRE
  REQUIRE(rows.size() == source_pos.size());

  for (auto i=0; i<rows.size(); i++) {
    auto [pos, map] = rows[i];

    CHECK_THAT(pos.x(), WithinULP(source_pos[i].x(), 1));
    CHECK_THAT(pos.y(), WithinULP(source_pos[i].y(), 1));
    CHECK_THAT(pos.z(), WithinULP(source_pos[i].z(), 1));

    REQUIRE(map.size() == counts[i].size());

    for (auto sipm_id : sipm_ids) {
      REQUIRE(map.contains(sipm_id));
      CHECK  (map[sipm_id] == counts[i][sipm_id]);
    }
  }
}

TEST_CASE("io parquet reader", "[io][parquet][reader]") {
  // Needed for CLI metadata
  n4::test::default_run_manager().run(0);

  // Match schema with generation
  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/n_sipms_x 2");
  UI -> ApplyCommand("/my/n_sipms_y 2");

  // Expected values
  float z = -37.2 * mm;
  std::vector<G4ThreeVector>       source_pos{{-3, -3, z}, {-3, 3, z}, {3, -3, z}, {3, 3, z}};
  std::vector<size_t       >       sipm_ids  {          0,          1,          2,         3};
  std::vector<std::vector<size_t>> counts{   {         46,         49,         50,        45},
                                             {          0,          0,          0,         0},
                                             {        414,        411,        412,       408},
                                             {          0,          0,          0,         0}
  };

  read_and_check("data/reader-test.parquet", source_pos, sipm_ids, counts);
}

TEST_CASE("io parquet roundtrip", "[io][parquet][writer]") {
  // Needed for CLI metadata
  n4::test::default_run_manager().run(0);

  std::string filename = std::tmpnam(nullptr);
  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/n_sipms_x 2");
  UI -> ApplyCommand("/my/n_sipms_y 2");
  UI -> ApplyCommand("/my/outfile " + filename);

  std::vector<G4ThreeVector>       source_pos{{0, 1, 2}, {0.25, 1.25, 2.25}, {0.5, 1.5, 2.5}, {0.75, 1.75, 2.75}};
  std::vector<size_t       >       sipm_ids  {        0,                  1,               2,                 3};
  std::vector<std::vector<size_t>> counts{   {       16,                 15,              14,                13},
                                             {       12,                 11,              10,                 9},
                                             {        8,                  7,               6,                 5},
                                             {        4,                  3,               2,                 1}
  };

  {
    auto writer = parquet_writer();
    std::unordered_map<size_t, size_t> map;
    arrow::Status status;
    for (auto i=0; i<source_pos.size(); i++) {
      for (auto sipm_id : sipm_ids) {
        map[sipm_id] = counts[i][sipm_id];
      }
      status = writer.append(source_pos[i], map);
      REQUIRE(status.ok());
    }
  } // writer goes out of scope, file should be written

  read_and_check(filename, source_pos, sipm_ids, counts);
}

// The parquet test file was generated with
//
//   just run -e "/my/n_sipms_xy 2" -n 4
//
TEST_CASE("io parquet reader metadata", "[io][parquet][reader][metadata]") {
  std::string filename = "data/reader-test.parquet";

  auto maybe_meta = read_metadata(filename);
  REQUIRE(maybe_meta.ok());

  std::unordered_map<std::string, std::string> expected {
    {"-e", "/my/n_sipms_xy 2"},
    {"-g", "vis.mac"},
    {"-m", "macs:"},
    {"commit-date", "2023-12-19 13:07:38 +0100"},
    {"-n", "4"},
    {"-l", "NOT SET"},
    {"commit-msg", "Unwrap lines in sipm_pde()"},
    {"commit-hash", "db9f8b1e4b35aa02865e6cad65cadfaa8b7e017f"},
    {"debug", "0"},
    {"physics_verbosity", "0"},
    {"particle_energy", "511.000000 keV"},
    {"fixed_energy", "1"},
    {"chunk_size", "1024"},
    {"sipm_thickness", "0.550000 mm"},
    {"sipm_size", "6.000000 mm"},
    {"sipm_threshold", "1"},
    {"event_threshold", "1"},
    {"reflector_thickness", "0.250000 mm"},
    {"n_sipms_x", "2"},
    {"n_sipms_y", "2"},
    {"scint", "CsI"},
    {"seed", "123456789"},
    {"scint_depth", "37.200000 mm"},
    {"scint_yield", "NULL"},
    {"reflectivity", "NULL"},
    {"absorbent_opposite", "true"},
    {"generator", "gammas_from_outside_crystal"},
    {"outfile", "crystal-out.parquet"},
    {"x_0", "-3.000000"},
    {"x_1", "-3.000000"},
    {"x_2",  "3.000000"},
    {"x_3",  "3.000000"},
    {"y_0", "-3.000000"},
    {"y_1",  "3.000000"},
    {"y_2", "-3.000000"},
    {"y_3",  "3.000000"},
  };

  auto meta = maybe_meta.ValueOrDie();

  CHECK(meta.size() == expected.size() + 1); // Schema missing from `expected`

  for (const auto& [key, value]: expected) {
    CHECK(meta.contains(key));
    CHECK(meta[key] == value);
  }

  CHECK(  meta.contains("ARROW:schema"));
  CHECK(! meta["ARROW:schema"].empty());
}

TEST_CASE("io parquet roundtrip metadata", "[io][parquet][writer][metadata]") {
  std::string filename = std::tmpnam(nullptr);
  auto nevt = "2";
  auto args_list = std::initializer_list<std::string>{
      "progname"
    , "-n", nevt
    , "-e"
    , "/my/outfile " + filename
    , "/my/scint_depth 13 mm"
    , "/my/scint_yield 123"
    , "/my/seed 9876"
    , "/my/n_sipms_y 19"
    , "/my/sipm_size 83 mm"
  };
  auto args = n4::test::argcv(args_list);

  run_stats stats;
  n4::run_manager::create()
    .ui("progname", args.argc, args.argv)
    .apply_cli_early()
    .physics(physics_list())
    .geometry([&] {return crystal_geometry(stats);})
    .actions(create_actions(stats))
    .run();

  auto args_vec = std::vector<std::string>{args_list};
  auto maybe_meta = read_metadata(filename);
  REQUIRE(maybe_meta.ok());

  auto meta = maybe_meta.ValueOrDie();
  REQUIRE(meta.contains("-n"));
  CHECK  (meta["-n"] == args_vec[2]);

  REQUIRE(meta.contains("-e"));
  const auto& early = meta["-e"];
  for (auto i=4; i<args_vec.size(); i++) {
    CHECK(early.find(args_vec[i]) != std::string::npos);
  }

  REQUIRE(meta.contains("commit-hash"));
  CHECK(! meta["commit-hash"].empty());

  REQUIRE(meta.contains("commit-date"));
  CHECK(! meta["commit-date"].empty());

  REQUIRE(meta.contains("commit-msg"));
  CHECK(! meta["commit-msg"].empty());
}
