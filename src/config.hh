#pragma once

#include <G4GenericMessenger.hh>
#include <G4Material.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4UnitsTable.hh>
#include <Randomize.hh>


enum class scintillator_type_enum { lyso, bgo, csi };
enum class config_type_enum       { lyso, bgo, csi, csi_mono, custom };

struct scint_parameters {
  scintillator_type_enum scint;
  double   scint_depth;
  unsigned n_sipms_x;
  unsigned n_sipms_y;
  double   scint_yield;
};

extern const scint_parameters lyso;
extern const scint_parameters bgo;
extern const scint_parameters csi;
extern const scint_parameters csi_mono;

std::string scintillator_type_to_string(scintillator_type_enum s);
scintillator_type_enum string_to_scintillator_type(std::string s);

std::string config_type_to_string(config_type_enum s);
config_type_enum string_to_config_type(std::string s);

struct config {
  scint_parameters scint_params        = csi;
  double           sipm_size           =  6    * mm;
  double           reflector_thickness =  0.25 * mm;
  double           particle_energy     = 511   * keV;
  double           source_pos          = -50   * mm;
  int              physics_verbosity   = 0;
  long             seed                = 123456789;
  bool             debug               = false ;

  config()
  // The trailing slash after '/my_geometry' is CRUCIAL: without it, the
  // messenger violates the principle of least surprise.
  : msg{new G4GenericMessenger{this, "/my/", "docs: bla bla bla"}}
  {
    G4UnitDefinition::BuildUnitsTable();

    new G4UnitDefinition("1/MeV","1/MeV", "1/Energy", 1/MeV);

    msg -> DeclareMethod("config_type", &config::set_config_type);

    msg -> DeclareProperty        ("reflector_thickness" ,        reflector_thickness    );
    msg -> DeclarePropertyWithUnit("particle_energy"     , "keV", particle_energy        );
    msg -> DeclareProperty        ("physics_verbosity"   ,        physics_verbosity      );
    msg -> DeclareProperty        ("source_pos"          ,        source_pos             );
    msg -> DeclareMethod          ("seed"                ,       &config::set_random_seed);
    msg -> DeclareProperty        ("debug"               ,        debug                  );

    set_random_seed(seed);
  }

  G4ThreeVector scint_size() const;
private:
  void set_config_type(const std::string& s);
  void set_scint (std::string   s) { scint_params.scint       = string_to_scintillator_type(s); }
  void set_scint_depth(double   d) { scint_params.scint_depth = d; }
  void set_n_sipms_x  (unsigned n) { scint_params.n_sipms_x   = n; }
  void set_n_sipms_y  (unsigned n) { scint_params.n_sipms_y   = n; }
  void set_random_seed(long seed)  { G4Random::setTheSeed(seed); }
  G4GenericMessenger* msg;
};

extern config my;

G4Material* scintillator_material(scintillator_type_enum type);
