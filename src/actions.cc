#include "actions.hh"
#include "config.hh"

#include <n4-inspect.hh>
#include <n4-mandatory.hh>
#include <n4-random.hh>
#include <n4-sequences.hh>

#include <G4PrimaryVertex.hh>


auto my_generator() {
  auto [sx, sy, sz] = n4::unpack(my.scint_size());
  auto tan = std::max(sx, sy) / 2 / (-sz - my.reflector_thickness - my.source_pos);
  auto gen = n4::random::direction().max_theta(std::atan(tan));

  return [&, gen](G4Event *event) {
    static auto particle_type = n4::find_particle("gamma");
    auto vertex = new G4PrimaryVertex(0, 0, my.source_pos, 0);
    auto r = gen.get();
    vertex -> SetPrimary(new G4PrimaryParticle(
                           particle_type,
                           r.x(), r.y(), r.z(),
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
