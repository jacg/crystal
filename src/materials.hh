#pragma once

#include <G4Material.hh>

G4Material*    csi_with_properties();
G4Material*   lyso_with_properties();
G4Material*    bgo_with_properties();

G4Material*         air_with_properties();
G4Material*      teflon_with_properties();
G4Material*     silicon_with_properties();
G4Material* optical_gel_with_properties();

G4MaterialPropertiesTable*      teflon_properties();
G4MaterialPropertiesTable* optical_gel_properties();
