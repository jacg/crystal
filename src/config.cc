#include "config.hh"
#include "materials.hh"

config my;

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

G4Material* scintillator_material(scintillator_type_enum type) {
  switch (type) {
    case scintillator_type_enum::csi : return  csi_with_properties();
    case scintillator_type_enum::lyso: return lyso_with_properties();
    case scintillator_type_enum::bgo : return  bgo_with_properties();
  }
  return nullptr; // unreachable
}
