#include "actions.hh"
#include "config.hh"

#include <n4-inspect.hh>
#include <n4-mandatory.hh>
#include <n4-random.hh>
#include <n4-sequences.hh>

#include <G4PrimaryVertex.hh>

#include <cstddef>

auto gammas_from_afar() {
  auto params = my.scint_params();
  return [params](G4Event *event) {
    static size_t event_number = 0;
    static auto particle_type = n4::find_particle("gamma");
    const auto N = event_number++ % (params.n_sipms_x * params.n_sipms_y);
    auto [x, y, _] = n4::unpack(my.sipm_positions()[N]);
    auto vertex = new G4PrimaryVertex(x, y, -params.scint_depth * 1.1, 0);
    vertex -> SetPrimary(new G4PrimaryParticle(
                           particle_type,
                           0,0,1, // parallel to z-axis
                           my.particle_energy
                         ));
    event  -> AddPrimaryVertex(vertex);
  };
}

n4::actions* create_actions(run_stats& stats) {

  auto my_event_action = [&] (const G4Event*) {
    stats.n_over_threshold += stats.n_detected_evt >= my.event_threshold;
    stats.n_detected_total += stats.n_detected_evt;
    auto n_sipms_over_threshold = stats.n_sipms_over_threshold(my.sipm_threshold);

    using std::setw; using std::fixed; using std::setprecision;
    std::cout
        << "event "
        << setw( 4) << n4::event_number()     << ':'
        << setw( 7) << stats.n_detected_evt   << " photons detected;"
        << setw( 6) << n_sipms_over_threshold << " SiPMs detected over "
        << setw( 6) << my.sipm_threshold << " photons;"
        << setw(10) << fixed << setprecision(1) << "     so far,"
        << setw( 6) << stats.n_events_over_threshold_fraction() << "% of events detected over"
        << setw( 7) << my.event_threshold << " photons."
        << std::endl;
    stats.n_detected_evt = 0;
    stats.n_detected_at_sipm.clear();
  };

  return (new n4::      actions{gammas_from_afar()})
 -> set( (new n4::event_action {                  }) -> end(my_event_action));
}
