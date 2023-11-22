#include "config.hh"
#include "materials.hh"

#include <G4ThreeVector.hh>

#include <cctype>

config my;

extern const scint_parameters lyso {
  .scint       = scintillator_type_enum::lyso,
  .scint_depth = 11.4*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .scint_yield = 666.6, // TODO look up scintillation yield for LYSO
};

extern const scint_parameters bgo {
  .scint       = scintillator_type_enum::bgo,
  .scint_depth = 11.4*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .scint_yield = 666.6, // TODO look up scintillation yield for BGO
};

extern const scint_parameters csi {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = 18.6*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
  .scint_yield = 50'000 / MeV,
};

extern const scint_parameters csi_mono {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = csi.scint_depth,
  .n_sipms_x   = 6,
  .n_sipms_y   = 6,
  .scint_yield = csi.scint_yield,
};

std::string scintillator_type_to_string(scintillator_type_enum s) {
  switch (s) {
    case scintillator_type_enum::lyso: return "LYSO";
    case scintillator_type_enum::bgo : return "BGO" ;
    case scintillator_type_enum::csi : return "CsI" ;
  }
  return "unreachable!";
}

scintillator_type_enum string_to_scintillator_type(const std::string& s) {
  auto z = s;
  for (auto& c: z) { c = std::toupper(c); }
  if (z == "lyso") { return scintillator_type_enum::lyso; }
  if (z == "bgo" ) { return scintillator_type_enum::bgo;  }
  if (z == "csi" ) { return scintillator_type_enum::csi;  }
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}

void config::set_config_type(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "lyso")      { scint_params = lyso; }
  if (s == "bgo" )      { scint_params = bgo;  }
  if (s == "csi" )      { scint_params = csi;  }
  if (s == "csi_mono" ) { scint_params = csi_mono;  }
  if (s == "custom"   ) {
    msg -> DeclareMethod("scint"      , &config::set_scint);
    msg -> DeclareMethod("scint_depth", &config::set_scint_depth);
    msg -> DeclareMethod("n_sipms_x"  , &config::set_n_sipms_x);
    msg -> DeclareMethod("n_sipms_y"  , &config::set_n_sipms_y);
    scint_params = csi;
  }
  throw "up"; // TODO think about failure propagation out of config::set_config_type
}

G4ThreeVector config::scint_size() const {
  return { scint_params.n_sipms_x * sipm_size
         , scint_params.n_sipms_y * sipm_size
         , scint_params.scint_depth          };
}

G4Material* scintillator_material(scintillator_type_enum type) {
  switch (type) {
    case scintillator_type_enum::csi : return  csi_with_properties();
    case scintillator_type_enum::lyso: return lyso_with_properties();
    case scintillator_type_enum::bgo : return  bgo_with_properties();
  }
  return nullptr; // unreachable
}
