#include "config.hh"
#include "geometry.hh"
#include "materials.hh"

#include <n4-material.hh>
#include <n4-sensitive.hh>
#include <n4-sequences.hh>
#include <n4-shape.hh>

#include <G4OpticalSurface.hh>
#include <G4LogicalBorderSurface.hh>
#include <G4TrackStatus.hh>

G4PVPlacement* crystal_geometry(unsigned& n_detected_evt) {
  auto scintillator = scintillator_material(my.scint_params.scint);
  auto air     = n4::material("G4_AIR");
  auto silicon = silicon_with_properties();
  auto teflon  = teflon_with_properties();

  auto [sx, sy, sz] = n4::unpack(my.scint_size());

  auto world  = n4::box("world").xyz(sx*2, sy*2, (sz - my.source_pos)*2.1).place(air).now();
  auto reflector = n4::box("reflector")
    .x(sx + 2*my.reflector_thickness)
    .y(sy + 2*my.reflector_thickness)
    .z(sz +   my.reflector_thickness)
    .place(teflon).at_z(-(sz + my.reflector_thickness) / 2)
    .in(world).now();

  auto crystal = n4::box("crystal")
    .xyz(sx, sy, sz)
    .place(scintillator).at_z(my.reflector_thickness / 2)
    .in(reflector).now();

  auto sipm_thickness = 1*mm;

  auto process_hits = [&n_detected_evt] (G4Step* step) {
    n_detected_evt++;
    step -> GetTrack() -> SetTrackStatus(fStopAndKill);
    return true;
  };

  auto Nx = my.scint_params.n_sipms_x;
  auto Ny = my.scint_params.n_sipms_y;
  auto sipm = n4::box("sipm")
    .xyz(sx/Nx, sy/Ny, sipm_thickness)
    .sensitive("sipm", process_hits)
    .place(silicon).in(world);

  auto n=0;
  for   (size_t i=0; i<my.scint_params.n_sipms_x; i++) {
    auto x = -my.scint_size().x()/2 + (i+0.5)*my.sipm_size;
    for (size_t j=0; j<my.scint_params.n_sipms_y; j++) {
      auto y = -my.scint_size().y()/2 + (j+0.5)*my.sipm_size;
      sipm.clone().at(x, y, sipm_thickness/2).copy_no(n++).now();
    }
  }

  // TODO add abstraction for placing optical surface between volumes
  auto
  teflon_surface = new G4OpticalSurface("teflon_surface");
  teflon_surface -> SetType(dielectric_dielectric);
  teflon_surface -> SetModel(unified);
  teflon_surface -> SetFinish(groundfrontpainted);     // Lambertian
  //teflon_surface -> SetFinish(polishedfrontpainted); // Specular
  teflon_surface -> SetSigmaAlpha(0.0);

  teflon_surface -> SetMaterialPropertiesTable(teflon_properties());
  new G4LogicalBorderSurface("teflon_surface", crystal, reflector, teflon_surface);

  return world;
}
