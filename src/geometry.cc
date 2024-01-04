#include "actions.hh"
#include "config.hh"
#include "geometry.hh"
#include "materials.hh"

#include <n4-inspect.hh>
#include <n4-material.hh>
#include <n4-random.hh>
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
G4Colour       gel_colour{1.0, 1.0, 0.0, 0.3};

G4Colour crystal_colour() {
  switch (my.scint_params().scint) {
    case scintillator_type_enum::bgo:  return  bgo_colour;
    case scintillator_type_enum::csi:  return  csi_colour;
    case scintillator_type_enum::lyso: return lyso_colour;
  }
  return csi_colour; // Unreachable, but GCC complains
}

G4Colour reflector_colour() {
  auto r = my.reflectivity;
  return r.has_value() && r.value() == 0 ? absorbent_colour : teflon_colour;
}

G4OpticalSurface* make_reflector_optical_surface() {
  auto reflector_surface = new G4OpticalSurface("crystal_reflector_interface");
  switch (my.wrapping) {
  case wrapping_enum::teflon: reflector_surface -> SetMaterialPropertiesTable(teflon_properties()); break;
  case wrapping_enum::esr   : reflector_surface -> SetMaterialPropertiesTable(   esr_properties()); break;
  case wrapping_enum::none  : reflector_surface -> SetMaterialPropertiesTable(   air_properties()); break;
  }

  switch (my.reflector_model) {
  case reflector_model_enum::lambertian:
    reflector_surface -> SetType(dielectric_dielectric);
    reflector_surface -> SetModel(unified);
    reflector_surface -> SetFinish(groundfrontpainted);
    break;
  case reflector_model_enum::specular:
    reflector_surface -> SetType(dielectric_dielectric);
    reflector_surface -> SetModel(unified);
    reflector_surface -> SetFinish(polishedfrontpainted);
    break;
  case reflector_model_enum::lut:
    reflector_surface -> SetType(dielectric_LUT);
    reflector_surface -> SetModel(LUT);
    switch (my.wrapping) {
    case wrapping_enum::teflon: reflector_surface -> SetFinish(groundteflonair); break;
    case wrapping_enum::esr   : reflector_surface -> SetFinish(groundvm2000air); break;
    case wrapping_enum::none  :                                                  break;
    }
    break;
  case reflector_model_enum::davis:
    reflector_surface -> SetType(dielectric_LUTDAVIS);
    reflector_surface -> SetModel(DAVIS);
    switch (my.wrapping) {
    case wrapping_enum::teflon: reflector_surface -> SetFinish(RoughTeflon_LUT); break;
    case wrapping_enum::esr   : reflector_surface -> SetFinish(RoughESR_LUT)   ; break;
    case wrapping_enum::none  :                                                  break;
    }
    break;
  }
  return reflector_surface;
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
    .vis(my.absorbent_opposite ? absorbent_colour : reflector_colour())
    .place(my.absorbent_opposite ? vacuum : teflon).at_z(-sz -my.reflector_thickness / 2)
    .in(world).now();

  auto crystal = n4::box("crystal")
    .xyz(sx, sy, sz)
    .vis(crystal_colour())
    .place(scintillator)
    .in(reflector).now();

  auto [pde_energies, pde_values] = sipm_pde();
  static auto pde = n4::interpolator(std::move(pde_energies), std::move(pde_values));

  auto process_hits = [&stats] (G4Step* step) {
    static auto optical_photon = n4::find_particle("opticalphoton");
    auto track = step -> GetTrack();
    if (track -> GetDefinition() == optical_photon) {
      auto p = pde(step -> GetTrack() -> GetTotalEnergy());
      if (n4::random::uniform() < p) {
        stats.n_detected_evt++;
        size_t n = step -> GetPreStepPoint() -> GetPhysicalVolume() -> GetCopyNo();
        ++stats.n_detected_at_sipm[n];
      }
      track -> SetTrackStatus(fStopAndKill);
    }
    return true;
  };

  n4::box("optical-gel")
    .xyz(my.scint_size()).z(my.gel_thickness) // x,y from scint size, override z
    .vis(gel_colour)
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
  auto reflector_surface = make_reflector_optical_surface();

  new G4LogicalBorderSurface("crystal_reflector_interface", crystal, reflector, reflector_surface);
  if (! my.absorbent_opposite) {
    new G4LogicalBorderSurface("crystal_opposite_interface", crystal, opposite, reflector_surface);
  }
  return world;
}
