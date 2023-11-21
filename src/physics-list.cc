#include "physics-list.hh"

#include "config.hh"

#include <FTFP_BERT.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4OpticalPhysics.hh>

G4VUserPhysicsList* physics_list() {
  auto physics_list = new FTFP_BERT{my.physics_verbosity};
  physics_list ->  ReplacePhysics(new G4EmStandardPhysics_option4{my.physics_verbosity});
  physics_list -> RegisterPhysics(new G4OpticalPhysics{my.physics_verbosity});
  return physics_list;
}
