#pragma once

#include <G4PVPlacement.hh>

G4PVPlacement* crystal_geometry();

// TODO: donate unpack to nain4
std::tuple<G4double, G4double, G4double> unpack(const G4ThreeVector& v);
