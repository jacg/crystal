#include "config.hh"
#include "materials.hh"
#include "n4-random.hh"

#include <cstdlib>
#include <n4-sequences.hh>

#include <G4ThreeVector.hh>

#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

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

  msg -> DeclareMethod          ("config_type"         ,          &config::set_config_type    );
  msg -> DeclarePropertyWithUnit("reflector_thickness" ,    "mm",  reflector_thickness        );
  msg -> DeclareMethodWithUnit  ("particle_energy"     ,   "keV", &config::set_particle_energy);
  msg -> DeclareProperty        (   "fixed_energy"     ,           fixed_energy               );
  msg -> DeclareProperty        ("physics_verbosity"   ,           physics_verbosity          );
  msg -> DeclareMethod          ("seed"                ,          &config::set_random_seed    );
  msg -> DeclareProperty        ("debug"               ,           debug                      );
  msg -> DeclareMethodWithUnit  ("scint_yield"         , "1/MeV", &config::set_scint_yield    );
  msg -> DeclareMethod          ("teflon_model"        ,          &config::set_teflon_model   );
  msg -> DeclareProperty        ("event_threshold"     ,           event_threshold            );
  msg -> DeclareProperty        ( "sipm_threshold"     ,            sipm_threshold            );
  msg -> DeclareMethod          ("reflectivity"        ,          &config::set_reflectivity   );
  msg -> DeclareProperty        ("absorbent_opposite"  ,           absorbent_opposite         );
  msg -> DeclareProperty        ( "generator"          ,           generator                  );
  msg -> DeclareProperty        ( "outfile"            ,           outfile                    );
  msg -> DeclareProperty        ( "chunk_size"         ,           chunk_size                 );
  msg -> DeclareProperty        ( "compression"        ,           compression                );

  msg -> DeclareMethod        ("scint"      ,       &config::set_scint);
  msg -> DeclareMethodWithUnit("scint_depth", "mm", &config::set_scint_depth);
  msg -> DeclareMethod        ("n_sipms_x"  ,       &config::set_n_sipms_x);
  msg -> DeclareMethod        ("n_sipms_y"  ,       &config::set_n_sipms_y);
  msg -> DeclareMethod        ("n_sipms_xy" ,       &config::set_n_sipms_xy);
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

std::string teflon_model_enum_to_string(teflon_model_enum s) {
  switch (s) {
    case teflon_model_enum ::lambertian: return "Lambertian";
    case teflon_model_enum ::specular  : return "Specular"  ;
    case teflon_model_enum ::lut       : return "LUT"       ;
    case teflon_model_enum ::davis     : return "DAVIS"     ;
  }
  return "unreachable!";
}

teflon_model_enum string_to_teflon_model_enum(std::string s) {
  for (auto& c: s) { c = std::tolower(c); }
  if (s == "lambertian") { return teflon_model_enum::lambertian;}
  if (s == "specular"  ) { return teflon_model_enum::specular;  }
  if (s == "lut"       ) { return teflon_model_enum::lut;       }
  if (s == "davis"     ) { return teflon_model_enum::davis;     }
  std::cerr << "\n\n\n\n         ERROR in string_to_teflon_model: unknown model '" << s << "'\n\n\n\n" << std::endl;
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}

void config::set_config_type(const std::string& s) {
  switch (string_to_config_type(s)) {
    case config_type_enum::lyso    : scint_params_ = lyso     ; break;
    case config_type_enum::bgo     : scint_params_ = bgo      ; break;
    case config_type_enum::csi     : scint_params_ = csi      ; break;
    case config_type_enum::csi_mono: scint_params_ = csi_mono ; break;
  }
  sipm_positions_need_recalculating = true;
  return;
}

n4::random::piecewise_linear_distribution scint_spectrum() {
  std::string msg{"Scintillation spectrum not implemented for scintillator "};
  std::pair<std::vector<double>, std::vector<double>> data;
  switch (my.scint_params().scint) {
    case scintillator_type_enum::csi : data = csi_scint_spectrum(); break;
    case scintillator_type_enum::lyso: throw std::runtime_error(msg + "LYSO");
    case scintillator_type_enum::bgo : throw std::runtime_error(msg +  "BGO");
  }
  return {std::move(data.first), std::move(data.second)};
}


double config::particle_energy() const {
  if (fixed_energy) { return particle_energy_; }
  if (! energy_spectrum.has_value()) { energy_spectrum.emplace(scint_spectrum()); }
  return energy_spectrum.value().sample();
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
  auto z  = gel_thickness + sipm_thickness/2;

  sipm_positions_.reserve(params.n_sipms_x * params.n_sipms_y);
  for   (auto x: n4::linspace(-lim_x, lim_x, Nx)) {
    for (auto y: n4::linspace(-lim_y, lim_y, Ny)) {
      sipm_positions_.push_back({x,y,z});
    }
  }
  sipm_positions_need_recalculating = false;
}

size_t config::n_sipms() const {
  auto params = scint_params();
  return params.n_sipms_x * params.n_sipms_y;
}

std::unordered_map<std::string, std::string> config::as_map() {
  std::unordered_map<std::string, std::string> it;
  auto params   = scint_params();
  auto sipm_pos = sipm_positions();

  it["scint"              ] = scintillator_type_to_string(params.scint);
  it["scint_depth"        ] = std::to_string(params.scint_depth/mm) + " mm";
  it["n_sipms_x"          ] = std::to_string(params.n_sipms_x);
  it["n_sipms_y"          ] = std::to_string(params.n_sipms_y);
  it["sipm_size"          ] = std::to_string(params.sipm_size/mm) + " mm";
  it["sipm_thickness"     ] = std::to_string(my.sipm_thickness/mm) + " mm";
  it["reflector_thickness"] = std::to_string(my.reflector_thickness/mm) + " mm";
  it["particle_energy"    ] = std::to_string(my.particle_energy_/keV) + " keV";
  it["fixed_energy"       ] = std::to_string(my.fixed_energy);
  it["physics_verbosity"  ] = std::to_string(my.physics_verbosity);
  it["seed"               ] = std::to_string(my.seed);
  it["debug"              ] = std::to_string(my.debug);
  it["event_threshold"    ] = std::to_string(my.event_threshold);
  it[ "sipm_threshold"    ] = std::to_string(my. sipm_threshold);
  it["chunk_size"         ] = std::to_string(my.chunk_size);
  it["scint_yield"        ] = my.scint_yield .has_value() ? std::to_string(my.scint_yield .value()*MeV) + " MeV^-1" : "NULL";
  it["reflectivity"       ] = my.reflectivity.has_value() ? std::to_string(my.reflectivity.value()    )             : "NULL";
  it["absorbent_opposite" ] = my.absorbent_opposite ? "true" : "false";
  it["generator"          ] = my.generator;
  it["outfile"            ] = my.outfile;

  size_t n = 0;
  for (const auto& p: sipm_positions()) {
    const auto& [x, y, _] = n4::unpack(p);
    it["x_" + std::to_string(n)] = std::to_string(x);
    it["y_" + std::to_string(n)] = std::to_string(y);
    n++;
  }

  return it;
}
