#include "config.hh"

config my;


std::string scintillator_type_to_string(scintillator_type s) {
  switch (s) {
    case scintillator_type::lyso: return "LYSO";
    case scintillator_type::bgo : return "BGO" ;
    case scintillator_type::csi : return "CsI" ;
  }
}

scintillator_type string_to_scintillator_type(const std::string& s) {
  auto z = s;
  for (auto& c: z) { c = std::toupper(c); }
  if (z == "lyso") { return scintillator_type::lyso; }
  if (z == "bgo" ) { return scintillator_type::bgo;  }
  if (z == "csi" ) { return scintillator_type::csi;  }
  throw "up"; // TODO think about failure propagation out of string_to_scintillator_type
}
