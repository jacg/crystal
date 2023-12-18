#include "actions.hh"
#include "config.hh"
#include "geometry.hh"
#include "materials.hh"

#include <n4-inspect.hh>
#include <n4-material.hh>
#include <n4-sensitive.hh>
#include <n4-sequences.hh>
#include <n4-shape.hh>

#include <G4Colour.hh>

#include <G4OpticalSurface.hh>
#include <G4LogicalBorderSurface.hh>
#include <G4TrackStatus.hh>

G4Colour       bgo_colour{0.9, 0.6, 0.1, 0.3};
G4Colour       csi_colour{0.0, 0.0, 0.6, 0.3};
G4Colour      lyso_colour{0.1, 0.7, 0.6, 0.3};
G4Colour    teflon_colour{1.0, 1.0, 1.0, 0.3};
G4Colour absorbent_colour{0.5, 0.3, 0.1, 0.3};
G4Colour       gel_colour{0.0, 1.0, 1.0, 0.3};

G4Colour crystal_colour() {
  switch (my.scint_params().scint) {
    case scintillator_type_enum::bgo:  return  bgo_colour;
    case scintillator_type_enum::csi:  return  csi_colour;
    case scintillator_type_enum::lyso: return lyso_colour;
  }
}

G4Colour reflector_colour() {
  auto r = my.reflectivity;
  return r.has_value() && r.value() == 0 ? absorbent_colour : teflon_colour;
}


G4PVPlacement* crystal_geometry(run_stats& stats) {
  auto scintillator = scintillator_material(my.scint_params().scint);
  auto air     = n4::material("G4_AIR");
  auto vacuum  = n4::material("G4_Galactic");
  auto silicon =     silicon_with_properties();
  auto teflon  =      teflon_with_properties();
  auto gel     = optical_gel_with_properties();
  auto [sx, sy, sz] = n4::unpack(my.scint_size());

  auto world  = n4::box("world").xyz(sx*1.5, sy*1.5, sz*2.5).place(air).now();
  auto reflector = n4::box("reflector")
    .x(sx + 2*my.reflector_thickness)
    .y(sy + 2*my.reflector_thickness)
    .z(sz                           )
    .vis(reflector_colour())
    .place(teflon).at_z(-sz/2)
    .in(world).now();

  auto opposite = n4::box("opposite")
    .x(sx + 2*my.reflector_thickness)
    .y(sy + 2*my.reflector_thickness)
    .z(       my.reflector_thickness)
    .vis(n4::vis_attributes(my.absorbent_opposite ? absorbent_colour : reflector_colour()))
    .place(my.absorbent_opposite ? vacuum : teflon).at_z(-sz -my.reflector_thickness / 2)
    .in(world).now();

  auto crystal = n4::box("crystal")
    .xyz(sx, sy, sz)
    .vis(n4::vis_attributes{crystal_colour()})
    .place(scintillator)
    .in(reflector).now();

  auto process_hits = [&stats] (G4Step* step) {
    static auto optical_photon = n4::find_particle("opticalphoton");
    auto track = step -> GetTrack();
    if (track -> GetDefinition() == optical_photon) {
      stats.n_detected_evt++;
      track -> SetTrackStatus(fStopAndKill);
      size_t n = step -> GetPreStepPoint() -> GetPhysicalVolume() -> GetCopyNo();
      ++stats.n_detected_at_sipm[n];
    }
    return true;
  };

  n4::box("optical-gel")
    .xyz(my.scint_size()).z(my.gel_thickness) // x,y from scint size, override z
    .vis(n4::vis_attributes{gel_colour})
    .place(gel).at_z(my.gel_thickness/2).in(world).now();

  auto sipm = n4::box("sipm")
    .xy(my.scint_params().sipm_size).z(my.sipm_thickness)
    .sensitive("sipm", process_hits)
    .place(silicon).in(world);

  auto n=0;
  for (const auto& pos: my.sipm_positions()) {
      sipm.clone().at(pos).copy_no(n++).now();
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
  if (! my.absorbent_opposite) {
    new G4LogicalBorderSurface("teflon_surface", crystal, opposite , teflon_surface);
  }

  return world;
}
