#include "config.hh"
#include "materials.hh"

#include <n4-sequences.hh>

#include <G4ThreeVector.hh>

#include <cctype>

config my;

extern const scint_parameters lyso {
  .scint       = scintillator_type_enum::lyso,
  .scint_depth = 22.8*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
};

extern const scint_parameters bgo {
  .scint       = scintillator_type_enum::bgo,
  .scint_depth = 22.8*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
};

extern const scint_parameters csi {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = 37.2*mm,
  .n_sipms_x   = 1,
  .n_sipms_y   = 1,
};

extern const scint_parameters csi_mono {
  .scint       = scintillator_type_enum::csi,
  .scint_depth = csi.scint_depth,
  .n_sipms_x   = 8,
  .n_sipms_y   = 8,
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

#define INVALIDATE_CACHE_AND_RETURN reset_sipm_positions(); return
void config::set_config_type(const std::string& s) {
  switch (string_to_config_type(s)) {
    case config_type_enum::lyso    : scint_params = lyso;     INVALIDATE_CACHE_AND_RETURN;
    case config_type_enum::bgo     : scint_params = bgo;      INVALIDATE_CACHE_AND_RETURN;
    case config_type_enum::csi     : scint_params = csi;      INVALIDATE_CACHE_AND_RETURN;
    case config_type_enum::csi_mono: scint_params = csi_mono; INVALIDATE_CACHE_AND_RETURN;
    case config_type_enum::custom  :
      msg -> DeclareMethod("scint"      , &config::set_scint);
      msg -> DeclareMethod("scint_depth", &config::set_scint_depth);
      msg -> DeclareMethod("n_sipms_x"  , &config::set_n_sipms_x);
      msg -> DeclareMethod("n_sipms_y"  , &config::set_n_sipms_y);
      scint_params = csi;
      INVALIDATE_CACHE_AND_RETURN;
  }
}
#undef INVALIDATE_CACHE_AND_RETURN

G4ThreeVector config::scint_size() const {
  return { scint_params.n_sipms_x * sipm_size
         , scint_params.n_sipms_y * sipm_size
         , scint_params.scint_depth          };
}

void config::reset_sipm_positions() {
  sipm_positions_.clear();
  auto Nx = my.scint_params.n_sipms_x; auto lim_x = my.sipm_size * (Nx / 2.0 - 0.5);
  auto Ny = my.scint_params.n_sipms_y; auto lim_y = my.sipm_size * (Ny / 2.0 - 0.5);
  sipm_positions_.reserve(scint_params.n_sipms_x * scint_params.n_sipms_y);
  for   (auto x: n4::linspace(-lim_x, lim_x, Nx)) {
    for (auto y: n4::linspace(-lim_y, lim_y, Ny)) {
      sipm_positions_.push_back({x,y,0});
    }
  }
}

G4Material* scintillator_material(scintillator_type_enum type) {
  switch (type) {
    case scintillator_type_enum::csi : return  csi_with_properties();
    case scintillator_type_enum::lyso: return lyso_with_properties();
    case scintillator_type_enum::bgo : return  bgo_with_properties();
  }
  return nullptr; // unreachable
}
