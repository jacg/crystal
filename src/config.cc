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

scintillator_type_enum string_to_scintillator_type(std::string s) {
  for (auto& c: s) { c = std::toupper(c); }
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
    case config_type_enum ::custom  : return "Custom" ;
  }
  return "unreachable!";
}

config_type_enum string_to_config_type(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "lyso"    ) { return config_type_enum::lyso;     }
  if (s == "bgo"     ) { return config_type_enum::bgo;      }
  if (s == "csi"     ) { return config_type_enum::csi;      }
  if (s == "csi-mono") { return config_type_enum::csi_mono; }
  if (s == "custom"  ) { return config_type_enum::custom;   }
  std::cerr << "\n\n\n\n         ERROR in string_to_config_type: unknown config '" << s << "'\n\n\n\n" << std::endl;
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}

void config::set_config_type(const std::string& s) {
  switch (string_to_config_type(s)) {
    case config_type_enum::lyso    : scint_params = lyso;     return;
    case config_type_enum::bgo     : scint_params = bgo;      return;
    case config_type_enum::csi     : scint_params = csi;      return;
    case config_type_enum::csi_mono: scint_params = csi_mono; return;
    case config_type_enum::custom  :
      msg -> DeclareMethod("scint"      , &config::set_scint);
      msg -> DeclareMethod("scint_depth", &config::set_scint_depth);
      msg -> DeclareMethod("n_sipms_x"  , &config::set_n_sipms_x);
      msg -> DeclareMethod("n_sipms_y"  , &config::set_n_sipms_y);
      scint_params = csi;
      return;
  }
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
