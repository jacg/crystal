#include "actions.hh"
#include "config.hh"

#include <cstdlib>
#include <iomanip>
#include <n4-inspect.hh>
#include <n4-mandatory.hh>
#include <n4-random.hh>
#include <n4-sequences.hh>

#include <G4PrimaryVertex.hh>

#include <cstddef>
#include <string>
#include <unordered_map>

using generator_fn = n4::generator::function;

generator_fn gammas_from_outside_crystal() {
  auto params = my.scint_params();
  auto const sipm_positions = my.sipm_positions();
  return [params, sipm_positions](G4Event *event) {
    static size_t event_number = 0;
    static auto particle_type = n4::find_particle("gamma");
    const auto N = event_number++ % (params.n_sipms_x * params.n_sipms_y);
    auto [x, y, _] = n4::unpack(sipm_positions[N]);
    auto vertex = new G4PrimaryVertex(x, y, -params.scint_depth * 1.1, 0);
    vertex -> SetPrimary(new G4PrimaryParticle(
                           particle_type,
                           0,0, my.particle_energy // parallel to z-axis
                         ));
    event  -> AddPrimaryVertex(vertex);
  };
}


generator_fn photoelectric_electrons() {
  auto isotropic                = n4::random::direction{};
  auto xe_kshell_binding_energy = 34.56 * keV;
  auto electron_K               = my.particle_energy - xe_kshell_binding_energy;
  auto electron_mass            = 0.510'998'91 * MeV;
  auto electron_momentum        = std::sqrt(     electron_K * electron_K
                                           + 2 * electron_K * electron_mass);
  auto [sx, sy, sz]             = n4::unpack(my.scint_size());

  return [isotropic, electron_momentum, sx, sy, sz] (G4Event *event) {
    static auto particle_type = n4::find_particle("e-");
    auto x0 =  n4::random::uniform_width(sx);
    auto y0 =  n4::random::uniform_width(sy);
    auto z0 = -n4::random::uniform   (0, sz);
    auto vertex = new G4PrimaryVertex(x0, y0, z0, 0);

    auto p  = isotropic.get() * electron_momentum;
    vertex -> SetPrimary(new G4PrimaryParticle(
                           particle_type,
                           p.x(), p.y(), p.z()
                         ));
    event  -> AddPrimaryVertex(vertex);
  };
}

generator_fn pointlike_photon_source() {
  static G4GenericMessenger msg{nullptr, "/source/", "Commands specific to this generator"};
  static unsigned nphot = 1'000;
  msg.DeclareProperty("nphotons", nphot);

  auto isotropic    = n4::random::direction{};
  auto [sx, sy, sz] = n4::unpack(my.scint_size());

  return [isotropic, sx, sy, sz] (G4Event *event) {
    static auto particle_type = n4::find_particle("opticalphoton");
    auto x0 =  n4::random::uniform_width(sx);
    auto y0 =  n4::random::uniform_width(sy);
    auto z0 = -n4::random::uniform   (0, sz);
    auto vertex = new G4PrimaryVertex(x0, y0, z0, 0);

    for (unsigned i=0; i<nphot; ++i) {
      auto p  = isotropic.get() * my.particle_energy;
      auto particle = new G4PrimaryParticle(
                        particle_type,
                        p.x(), p.y(), p.z()
                        );
      particle -> SetPolarization(isotropic.get());
      vertex   -> SetPrimary(particle);
    }
    event  -> AddPrimaryVertex(vertex);
  };
}

enum class generators {gammas_from_outside_crystal, photoelectric_electrons, pointlike_photon_source};

generators string_to_generator(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "gammas_from_outside_crystal"        ||
      s == "gammas"                  ) { return generators::gammas_from_outside_crystal; }
  if (s == "photoelectric_electrons" ||
      s == "electrons"               ) { return generators::photoelectric_electrons;     }
  if (s == "pointlike_photon_source" ||
      s == "photons"                 ) { return generators::pointlike_photon_source;     }
  throw "Invalid generator name: `" + s + "'"; // TODO think about failure propagation out of here
}

std::function<generator_fn((void))> select_generator() {
  auto symbol = string_to_generator(my.generator);
  switch (symbol) {
    case generators::gammas_from_outside_crystal: return gammas_from_outside_crystal;
    case generators::photoelectric_electrons    : return photoelectric_electrons;
    case generators::pointlike_photon_source    : return pointlike_photon_source;
  }
  throw "[select_generator]: unreachable";
}

csv_writer::csv_writer() : out{my.outfile}, number_of_sipms{my.N_sipms()} {
  if (!out) {
    std::cerr << "ERROR: Could not open output file '" << my.outfile << "'." << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void csv_writer::write(const G4ThreeVector& pos, const std::unordered_map<size_t, size_t>& counts) {
  out


    << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(6) << pos.x() << ' '
    << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(6) << pos.y() << ' '
    << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(6) << pos.z() << ' ';
  for (size_t sipm=0; sipm<number_of_sipms; ++sipm) {
    out << std::setw(4) << (counts.contains(sipm) ? counts.at(sipm) : 0) << ' ';
  }
  out << std::endl;
}

n4::actions* create_actions(run_stats& stats) {
  auto my_event_action = [&] (const G4Event* event) {
    static csv_writer writer;
    stats.n_over_threshold += stats.n_detected_evt >= my.event_threshold;
    stats.n_detected_total += stats.n_detected_evt;
    //auto n_sipms_over_threshold = stats.n_sipms_over_threshold(my.sipm_threshold);

    auto primary_vertex = event -> GetPrimaryVertex();
    const auto& counts = stats.n_detected_at_sipm;
    writer.write(primary_vertex -> GetPosition(), counts);
    stats.n_detected_evt = 0;
    stats.n_detected_at_sipm.clear();
  };

  return (new n4::      actions{select_generator()()})
 -> set( (new n4::event_action {                    }) -> end(my_event_action));
}
