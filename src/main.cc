#include "config.hh"
#include "geometry.hh"
#include "materials.hh"
#include "physics-list.hh"

#include <n4-all.hh>

#include <G4PrimaryParticle.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>   // physical units such as `m` for metre
#include <G4Event.hh>           // needed to inject primary particles into an event
#include <G4Box.hh>             // for creating shapes in the geometry
#include <G4Sphere.hh>          // for creating shapes in the geometry
#include <FTFP_BERT.hh>         // our choice of physics list


#include <G4ThreeVector.hh>
#include <cstdlib>

auto my_generator(const config& my) {
  return [&](G4Event *event) {
    auto particle_type = n4::find_particle("gamma");
    auto vertex = new G4PrimaryVertex(0, 0, my.source_pos, 0);
    auto tan = std::max(my.scint_size.x(), my.scint_size.y()) / 2 / (-my.scint_size.z() - my.reflector_thickness - my.source_pos);
    auto r = n4::random::direction().max_theta(std::atan(tan)).get();
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

  return (new n4::        actions{my_generator(my)  })
 -> set( (new n4::   event_action{                  }) -> end(my_event_action) )
 -> set(  new n4::stepping_action{my_stepping_action});
}

int main(int argc, char* argv[]) {
  unsigned n_event = 0;

  n4::run_manager::create()
    .ui("crystal", argc, argv)
    .macro_path("macs")
    //.apply_command("/my/scint_size 30 10 2 mm")
    .apply_early_macro("early-hard-wired.mac")
    .apply_cli_early() // CLI --early executed at this point
    // .apply_command(...) // also possible after apply_early_macro

    .physics(physics_list)
    .geometry([&] { return my_geometry(my); })
    .actions(create_actions(my, n_event))

    //.apply_command("/my/particle e-")
    .apply_late_macro("late-hard-wired.mac")
    .apply_cli_late() // CLI --late executed at this point
    // .apply_command(...) // also possible after apply_late_macro

    .run();



  // Important! physics list has to be set before the generator!

}
