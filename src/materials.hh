#pragma once

#include <G4Material.hh>

std::pair<std::vector<double>, std::vector<double>> csi_scint_spectrum();


G4Material*    csi_with_properties();
G4Material*   lyso_with_properties();
G4Material*    bgo_with_properties();

G4Material*         air_with_properties();
G4Material*         esr_with_properties();
G4Material*      teflon_with_properties();
G4Material*     silicon_with_properties();
G4Material* optical_gel_with_properties();

G4MaterialPropertiesTable*         esr_properties();
G4MaterialPropertiesTable*      teflon_properties();
G4MaterialPropertiesTable* optical_gel_properties();

std::pair<std::vector<double>, std::vector<double>> sipm_pde();
