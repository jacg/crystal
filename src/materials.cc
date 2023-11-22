#include "materials.hh"
#include "G4MaterialPropertiesTable.hh"
#include "config.hh"

#include <n4-all.hh>

#include <G4SystemOfUnits.hh>

G4Material* lyso_with_properties() { return n4::material("G4_WATER"); }
G4Material*  bgo_with_properties() { return n4::material("G4_WATER"); }


using vec_double = std::vector<G4double>;

// TODO: remove duplication of hc (defined in moth materials.cc and geometry.cc)
const   G4double hc = CLHEP::h_Planck * CLHEP::c_light;
const   G4double OPTPHOT_MIN_ENERGY   {1.00*eV};
const   G4double OPTPHOT_MAX_ENERGY   {8.21*eV};
const vec_double OPTPHOT_ENERGY_RANGE{OPTPHOT_MIN_ENERGY, OPTPHOT_MAX_ENERGY};

G4Material* csi_with_properties() {
    auto csi = n4::material("G4_CESIUM_IODIDE");
    // csi_rindex: values taken from "Optimization of Parameters for a CsI(Tl) Scintillator Detector Using GEANT4-Based Monte Carlo..." by Mitra et al (mainly page 3)
    //  csi_scint: values from Fig. 2 in "A New Scintillation Material: Pure CsI with 10ns Decay Time" by Kubota et al (these are approximate...)
    // must be in increasing ENERGY order (decreasing wavelength) for scintillation to work properly
    auto      csi_energies = n4::scale_by(hc*eV, {1/0.55, 1/0.36, 1/0.3 , 1/0.26}); // denominator is wavelength in micrometres
    auto csi_energies_cold = n4::scale_by(hc*eV, {1/0.5 , 1/0.4 , 1/0.35, 1/0.27}); // denominator is wavelength in micrometres
    // auto     csi_energies = n4::scale_by(hc*eV, {1/0.9, 1/0.7, 1/0.54, 1/0.35});
    vec_double csi_rindex =                     {1.79  , 1.79  , 1.79 , 1.79  };  //vec_double csi_rindex = {2.2094, 1.7611};
    vec_double  csi_scint =                     {0.0   , 0.1   , 1.0  , 0.0   };
    auto    csi_abslength = n4::scale_by(m    , {5     , 5     , 5    , 5     });
    // Values from "Temperature dependence of pure CsI: scintillation light yield and decay time" by Amsler et al
    // "cold" refers to ~77K, i.e. liquid nitrogen temperature
    G4double csi_scint_yield = my.scint_yield.value_or(my.scint_params.scint_yield); // 50000 / MeV in cold
    G4double csi_time_fast   =  1015 * ns; // only one component at cold temps!
    G4double csi_time_slow   =  1015 * ns;
    auto mpt = n4::material_properties()
        .add("RINDEX"                 , csi_energies, csi_rindex)
        .add("SCINTILLATIONCOMPONENT1", csi_energies,  csi_scint)
        .add("SCINTILLATIONCOMPONENT2", csi_energies,  csi_scint)
        .add("ABSLENGTH"              , csi_energies, csi_abslength)
        .add("SCINTILLATIONTIMECONSTANT1", csi_time_fast)
        .add("SCINTILLATIONTIMECONSTANT2", csi_time_slow)
        .add("SCINTILLATIONYIELD"        , csi_scint_yield)
        .add("SCINTILLATIONYIELD1"       ,     0.57   )
        .add("SCINTILLATIONYIELD2"       ,     0.43   )
        .add("RESOLUTIONSCALE"           ,     1.0    )
        .done();
    csi -> GetIonisation() -> SetBirksConstant(0.00152 * mm/MeV);
    csi -> SetMaterialPropertiesTable(mpt);
    return csi;
}

G4Material* air_with_properties() {
    auto air = n4::material("G4_AIR");
    std::vector<double> x{1., 2.};
    auto mpt = n4::material_properties()
        .add("RINDEX", x, x)
        .done();
    air -> SetMaterialPropertiesTable(mpt);
    return air;
}

G4Material* teflon_with_properties() {
  auto teflon = n4::material("G4_TEFLON");
  teflon -> SetMaterialPropertiesTable(teflon_properties());
  return teflon;
}

G4MaterialPropertiesTable* teflon_properties() {
    // Values could be taken from "Optical properties of Teflon AF amorphous fluoropolymers" by Yang, French & Tokarsky (using AF2400, Fig.6)
    // but are also stated in the same paper as above
    auto energy       = n4::scale_by(  eV, {OPTPHOT_MIN_ENERGY/eV,  2.8,  3.5,  4.0,  6.0,  7.2, OPTPHOT_MAX_ENERGY/eV});
    auto reflectivity = n4::scale_by(0.01, {                   98, 98  , 98  , 98  , 72  , 72  , 72});

  return n4::material_properties()
    .add("RINDEX"               , energy, 1.41)
    .add("REFLECTIVITY"         , energy, reflectivity)
    .add("SPECULARLOBECONSTANT" , OPTPHOT_ENERGY_RANGE, 0.0)
    //.add("SPECULARSPIKECONSTANT", OPTPHOT_ENERGY_RANGE, 0.0)
    .add("BACKSCATTERCONSTANT"  , OPTPHOT_ENERGY_RANGE, 0.0)
    .done();
}

// TODO: implement E dependence from
// https://refractiveindex.info/?shelf=main&book=Si&page=Aspnes
G4Material* silicon_with_properties() {
    auto si  = n4::material("G4_Si");
    auto mpt = n4::material_properties()
      .add("RINDEX", OPTPHOT_ENERGY_RANGE, 4.32)
      .add("EFFICIENCY", OPTPHOT_ENERGY_RANGE, 1) // full absorption at the sipm surface
      .done();
    si -> SetMaterialPropertiesTable(mpt);
    return si;
}
