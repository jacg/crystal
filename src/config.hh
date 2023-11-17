#pragma once

#include <G4GenericMessenger.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <Randomize.hh>


enum class scintillator_type { lyso, bgo, csi };

std::string scintillator_type_to_string(scintillator_type s);

scintillator_type string_to_scintillator_type(const std::string &s);

struct config {
  scintillator_type scintillator_type  {scintillator_type::csi};
  G4ThreeVector     scint_size         {6*mm, 6*mm, 20*mm};
  G4int             physics_verbosity  {0};
  G4double          reflector_thickness{  0.25 * mm};
  G4double          particle_energy    {511    * keV};
  G4double          source_pos         {-50    * mm};
  G4double          scint_yield        {50000    / MeV};
  G4long            seed               {123456789};

  config()
  // The trailing slash after '/my_geometry' is CRUCIAL: without it, the
  // messenger violates the principle of least surprise.
  : msg{new G4GenericMessenger{this, "/my/", "docs: bla bla bla"}}
  {
    G4UnitDefinition::BuildUnitsTable();

    new G4UnitDefinition("1/MeV","1/MeV", "1/Energy", 1/MeV);

    msg -> DeclareMethod          ("scint_type"          ,       &config::set_scint      );
    msg -> DeclareProperty        ("scint_size"          ,        scint_size             );
    msg -> DeclarePropertyWithUnit("scint_yield", "1/MeV",        scint_yield            );
    msg -> DeclareProperty        ("reflector_thickness" ,        reflector_thickness    );
    msg -> DeclarePropertyWithUnit("particle_energy"     , "keV", particle_energy        );
    msg -> DeclareProperty        ("physics_verbosity"   ,        physics_verbosity      );
    msg -> DeclareProperty        ("source_pos"          ,        source_pos             );
    msg -> DeclareMethod          ("seed"                ,       &config::set_random_seed);

    set_random_seed(seed);
  }
private:
  void set_scint(const std::string& s) { scintillator_type = string_to_scintillator_type(s); }
  void set_random_seed(G4long seed) { G4Random::setTheSeed(seed); }
  G4GenericMessenger* msg;
};

extern config my;
