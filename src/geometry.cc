#include "geometry.hh"
#include "materials.hh"

#include <n4-material.hh>
#include <n4-shape.hh>

std::tuple<G4double, G4double, G4double> unpack(const G4ThreeVector& v) { return {v.x(), v.y(), v.z()}; }

G4PVPlacement* crystal_geometry(const config& my) {
  auto scintillator = scintillator_material(my.scintillator_type);
  auto air    = air_with_properties();
  auto teflon = teflon_with_properties();

  auto [sx, sy, sz] = unpack(my.scint_size);

  auto world  = n4::box("world").xyz(sx*2, sy*2, (sz - my.source_pos)*2.1).place(air).now();
  auto reflector = n4::box("reflector")
    .x(sx + 2*my.reflector_thickness)
    .y(sy + 2*my.reflector_thickness)
    .z(sz +   my.reflector_thickness)
    .place(teflon).at_z(-(sz + my.reflector_thickness) / 2)
    .in(world).now();

  n4::box("crystal")
    .xyz(sx, sy, sz)
    .place(scintillator).at_z(my.reflector_thickness / 2)
    .in(reflector).now();

  return world;
}
