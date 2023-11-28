#include "actions.hh"
#include "config.hh"
#include "geometry.hh"
#include "materials.hh"
#include "physics-list.hh"

#include <iomanip>
#include <n4-all.hh>

#include <G4PrimaryParticle.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>   // physical units such as `m` for metre
#include <G4Event.hh>           // needed to inject primary particles into an event
#include <G4Box.hh>             // for creating shapes in the geometry
#include <G4Sphere.hh>          // for creating shapes in the geometry
#include <FTFP_BERT.hh>         // our choice of physics list
#include <G4ThreeVector.hh>


#include <ios>
#include <cstdlib>

int main(int argc, char* argv[]) {
  run_stats stats;

  n4::run_manager::create()
    .ui("crystal", argc, argv)
    .macro_path("macs")
    //.apply_command("/my/scint_size 30 10 2 mm")
    .apply_early_macro("early-hard-wired.mac")
    .apply_cli_early() // CLI --early executed at this point
    // .apply_command(...) // also possible after apply_early_macro

    .physics(physics_list)
    .geometry([&] {return crystal_geometry(stats);})
    .actions(create_actions(stats))

    //.apply_command("/my/particle e-")
    .apply_late_macro("late-hard-wired.mac")
    .apply_cli_late() // CLI --late executed at this point
    // .apply_command(...) // also possible after apply_late_macro

    .run();

  using std::setprecision; using std::fixed; using std::setw;
  std::cout << "total detected: " << stats.n_detected_total << std::endl;

  // Important! physics list has to be set before the generator!

}
