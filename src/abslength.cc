#include <materials.hh>
#include <physics-list.hh>

#include <n4-sequences.hh>
#include <n4-stream.hh>
#include <n4-will-become-external-lib.hh>

#include <G4Material.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

std::vector<double> doit(const std::string name, G4Material* material, std::vector<double> distances) {
  auto abs_lengths = measure_abslength(test_config{ .physics         = physics_list()
                                                  , .material        = material
                                                  , .particle_name   = "gamma"
                                                  , .particle_energy = 511 * keV
                                                  , .distances       = distances});

  return abs_lengths;
}

int main(int argc, char** argv) {
  auto choice = std::string{argv[1]};
  auto distances = n4::scale_by(mm, {5, 10, 15, 20, 25, 30, 35, 40, 45, 50});
  std::cout << "Calculating attenuation lengths for : '" << choice << "'\n"
            << distances.size() << " statistical samples ...\n\n";
  std::string name;
  G4Material* material;
  if      (choice == "csi" ) { name = "CsI" ; material =  csi_with_properties(); }
  else if (choice == "bgo" ) { name = "BGO" ; material =  bgo_with_properties(); }
  else if (choice == "lyso") { name = "LYSO"; material = lyso_with_properties(); }
  auto results = [&] {
    n4::silence hush{std::cout};
    return doit(name, material, distances);
  }();
  for (const auto& d: results) { std::cout << G4BestUnit(d, "Length") << ' '; }
  std::cout << std::endl << std::endl << std::endl;
}
