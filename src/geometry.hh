#pragma once

#include "run_stats.hh"

#include <G4PVPlacement.hh>
#include <G4OpticalSurface.hh>

G4OpticalSurface* make_reflector_optical_surface(); // exposed so it can be tested

G4PVPlacement* crystal_geometry(run_stats&);
