#include "actions.hh"

#include <n4-mandatory.hh>
#include <n4-random.hh>
#include <n4-inspect.hh>

#include <G4PrimaryVertex.hh>


auto my_generator() {
  auto tan = std::max(my.scint_size.x(), my.scint_size.y()) / 2 / (-my.scint_size.z() - my.reflector_thickness - my.source_pos);
  auto gen = n4::random::direction().max_theta(std::atan(tan));

  return [&, gen](G4Event *event) {
    auto particle_type = n4::find_particle("gamma");
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

n4::actions* create_actions(const config& my, unsigned& n_event) {
  auto my_stepping_action = [&] (const G4Step* step) {
    auto pt = step -> GetPreStepPoint();
    auto volume_name = pt -> GetTouchable() -> GetVolume() -> GetName();
    if (volume_name == "straw" || volume_name == "bubble") {
      auto pos = pt -> GetPosition();
      std::cout << volume_name << " " << pos << std::endl;
    }
  };

  auto my_event_action = [&] (const G4Event*) {
     n_event++;
     std::cout << "end of event " << n_event << std::endl;
  };

  return (new n4::        actions{my_generator()    })
 -> set( (new n4::   event_action{                  }) -> end(my_event_action) )
 -> set(  new n4::stepping_action{my_stepping_action});
}
