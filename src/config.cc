#include "config.hh"
#include "materials.hh"

#include <cstdlib>
#include <n4-sequences.hh>

#include <G4ThreeVector.hh>

#include <cctype>

config my;

static const scint_parameters lyso {
  .scint       = scintillator_type_enum::lyso,
  .scint_depth = 22.8*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .sipm_size   = 6   *mm
};

static const scint_parameters bgo {
  .scint       = scintillator_type_enum::bgo,
  .scint_depth = 22.8*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .sipm_size   = 6   *mm
};

static const scint_parameters csi {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = 37.2*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .sipm_size   = 6   *mm
};

static const scint_parameters csi_mono {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = csi.scint_depth,
  .n_sipms_x   = 8,
  .n_sipms_y   = 8,
  .sipm_size   = 6   *mm
};

config::config()
: scint_params_{csi}
// The trailing slash after '/my_geometry' is CRUCIAL: without it, the
// messenger violates the principle of least surprise.
, msg{new G4GenericMessenger{this, "/my/", "docs: bla bla bla"}}
{
  G4UnitDefinition::BuildUnitsTable();
  new G4UnitDefinition("1/MeV","1/MeV", "1/Energy", 1/MeV);

  msg -> DeclareMethod          ("config_type"         ,          &config::set_config_type );
  msg -> DeclarePropertyWithUnit("reflector_thickness" ,    "mm",  reflector_thickness     );
  msg -> DeclarePropertyWithUnit("particle_energy"     ,   "keV",  particle_energy         );
  msg -> DeclareProperty        ("physics_verbosity"   ,           physics_verbosity       );
  msg -> DeclareMethod          ("seed"                ,          &config::set_random_seed );
  msg -> DeclareProperty        ("debug"               ,           debug                   );
  msg -> DeclareMethodWithUnit  ("scint_yield"         , "1/MeV", &config::set_scint_yield );
  msg -> DeclareProperty        ("event_threshold"     ,           event_threshold         );
  msg -> DeclareProperty        ( "sipm_threshold"     ,            sipm_threshold         );
  msg -> DeclareMethod          ("reflectivity"        ,          &config::set_reflectivity);
  msg -> DeclareProperty        ( "generator"          ,           generator               );
  msg -> DeclareProperty        ( "outfile"            ,           outfile                 );

  msg -> DeclareMethod        ("scint"      ,       &config::set_scint);
  msg -> DeclareMethodWithUnit("scint_depth", "mm", &config::set_scint_depth);
  msg -> DeclareMethod        ("n_sipms_x"  ,       &config::set_n_sipms_x);
  msg -> DeclareMethod        ("n_sipms_y"  ,       &config::set_n_sipms_y);
  msg -> DeclareMethodWithUnit("sipm_size"  , "mm", &config::set_sipm_size);

  set_random_seed(seed);
}


std::string scintillator_type_to_string(scintillator_type_enum s) {
  switch (s) {
    case scintillator_type_enum::lyso: return "LYSO";
    case scintillator_type_enum::bgo : return "BGO" ;
    case scintillator_type_enum::csi : return "CsI" ;
  }
  return "unreachable!";
}

scintillator_type_enum string_to_scintillator_type(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "lyso") { return scintillator_type_enum::lyso; }
  if (s == "bgo" ) { return scintillator_type_enum::bgo;  }
  if (s == "csi" ) { return scintillator_type_enum::csi;  }
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}

std::string config_type_to_string(config_type_enum s) {
  switch (s) {
    case config_type_enum ::lyso    : return "LYSO";
    case config_type_enum ::bgo     : return "BGO" ;
    case config_type_enum ::csi     : return "CsI" ;
    case config_type_enum ::csi_mono: return "CsI-monolithic" ;
  }
  return "unreachable!";
}

config_type_enum string_to_config_type(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "lyso"    ) { return config_type_enum::lyso;     }
  if (s == "bgo"     ) { return config_type_enum::bgo;      }
  if (s == "csi"     ) { return config_type_enum::csi;      }
  if (s == "csi-mono") { return config_type_enum::csi_mono; }
  if (s == "csi_mono") { return config_type_enum::csi_mono; }
  std::cerr << "\n\n\n\n         ERROR in string_to_config_type: unknown config '" << s << "'\n\n\n\n" << std::endl;
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}

void config::set_config_type(const std::string& s) {
  switch (string_to_config_type(s)) {
    case config_type_enum::lyso    : scint_params_ = lyso; return;
    case config_type_enum::bgo     : scint_params_ = bgo; return;
    case config_type_enum::csi     : scint_params_ = csi; return;
    case config_type_enum::csi_mono: scint_params_ = csi_mono; return;
  }
  throw "[config::set_config_type] unreachable";
}

G4ThreeVector config::scint_size() const {
  auto params = scint_params();
  return { params.n_sipms_x * params.sipm_size
         , params.n_sipms_y * params.sipm_size
         , params.scint_depth                 };
}

G4Material* scintillator_material(scintillator_type_enum type) {
  switch (type) {
    case scintillator_type_enum::csi : return  csi_with_properties();
    case scintillator_type_enum::lyso: return lyso_with_properties();
    case scintillator_type_enum::bgo : return  bgo_with_properties();
  }
  return nullptr; // unreachable
}

#define APPLY_OVERRIDE(X) .X = overrides.X.has_value() ? overrides.X.value() : scint_params_.X
const scint_parameters config::scint_params() const {
  auto params = scint_parameters{
    APPLY_OVERRIDE(scint),
    APPLY_OVERRIDE(scint_depth),
    APPLY_OVERRIDE(n_sipms_x),
    APPLY_OVERRIDE(n_sipms_y),
    APPLY_OVERRIDE(sipm_size)
  };
  return params;
}
#undef APPLY_OVERRIDE

const std::vector<G4ThreeVector>& config::sipm_positions() const {
  if (sipm_positions_need_recalculating) { recalculate_sipm_positions(); }
  return sipm_positions_;
}

void config::recalculate_sipm_positions() const {
  auto params = scint_params();
  sipm_positions_.clear();
  auto Nx = params.n_sipms_x; auto lim_x = params.sipm_size * (Nx - 1) / 2.0;
  auto Ny = params.n_sipms_y; auto lim_y = params.sipm_size * (Ny - 1) / 2.0;
  sipm_positions_.reserve(params.n_sipms_x * params.n_sipms_y);
  for   (auto x: n4::linspace(-lim_x, lim_x, Nx)) {
    for (auto y: n4::linspace(-lim_y, lim_y, Ny)) {
      sipm_positions_.push_back({x,y,sipm_thickness/2});
    }
  }
  sipm_positions_need_recalculating = false;
}

size_t config::n_sipms() const {
  auto params = scint_params();
  return params.n_sipms_x * params.n_sipms_y;
}
