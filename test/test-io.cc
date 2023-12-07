#include <config.hh>
#include <io.hh>

#include <G4UImanager.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinULP;

void read_and_check(const auto& filename, const auto& source_pos, const auto& sipm_ids, const auto& counts) {
  // Read data
  auto maybe_data = read_entire_file(filename);
  CHECK(maybe_data.ok());
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

TEST_CASE("io reader", "[io][reader]") {
  // Match schema with generation
  auto UI = G4UImanager::GetUIpointer();
  UI -> ApplyCommand("/my/n_sipms_x 2");
  UI -> ApplyCommand("/my/n_sipms_y 2");

  // Expected values
  auto z = -40.91999816894531;
  std::vector<G4ThreeVector>       source_pos{{-3, -3, z}, {-3, 3, z}, {3, -3, z}, {3, 3, z}};
  std::vector<size_t       >       sipm_ids  {          0,          1,          2,         3};
  std::vector<std::vector<size_t>> counts{   {       4472,       4579,       4539,      4556},
                                             {       1592,       1677,       1640,      1711},
                                             {       3990,       4054,       4015,      3936},
                                             {       1501,       1560,       1496,      1589}
  };

  read_and_check("data/reader-test.parquet", source_pos, sipm_ids, counts);
}
