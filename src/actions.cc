#include "actions.hh"
#include "config.hh"

#include <n4-inspect.hh>
#include <n4-mandatory.hh>
#include <n4-random.hh>
#include <n4-sequences.hh>

#include <G4PrimaryVertex.hh>


auto my_generator() {
  return [&](G4Event *event) {
    static size_t event_number = 0;
    static auto particle_type = n4::find_particle("gamma");
    const auto N = event_number++ % (my.scint_params.n_sipms_x * my.scint_params.n_sipms_y);
    auto [x, y, _] = n4::unpack(my.sipm_positions()[N]);
    auto vertex = new G4PrimaryVertex(x, y, my.source_z, 0);
    vertex -> SetPrimary(new G4PrimaryParticle(
                           particle_type,
                           0,0,1, // parallel to z-axis
                           my.particle_energy
                         ));
    event  -> AddPrimaryVertex(vertex);
  };
}

n4::actions* create_actions(unsigned& n_event, unsigned& n_detected_evt, std::vector<unsigned>& n_detected_run) {

  auto my_event_action = [&] (const G4Event*) {
     n_event++;
     n_detected_run.push_back(n_detected_evt);
     n_detected_evt = 0;
     std::cout << "end of event " << n_event << std::endl;
  };

  return (new n4::      actions{my_generator()})
 -> set( (new n4::event_action {              }) -> end(my_event_action));
}
