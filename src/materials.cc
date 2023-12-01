#include <config.hh>
#include <materials.hh>

#include <n4-all.hh>

#include <G4MaterialPropertiesTable.hh>
#include <G4SystemOfUnits.hh>

using vec_double = std::vector<double>;

// TODO: remove duplication of hc (defined in moth materials.cc and geometry.cc)
const     double hc = CLHEP::h_Planck * CLHEP::c_light;
const     double OPTPHOT_MIN_ENERGY   {1.00*eV};
const     double OPTPHOT_MAX_ENERGY   {8.21*eV};
const vec_double OPTPHOT_ENERGY_RANGE{OPTPHOT_MIN_ENERGY, OPTPHOT_MAX_ENERGY};

G4Material* csi_with_properties() {
  auto csi = n4::material("G4_CESIUM_IODIDE");
  // rindex: values taken from "Optimization of Parameters for a CsI(Tl) Scintillator Detector Using GEANT4-Based Monte Carlo..." by Mitra et al (mainly page 3)
  //  scint: values from Fig. 2 in "A New Scintillation Material: Pure CsI with 10ns Decay Time" by Kubota et al (these are approximate...)
  // must be in increasing ENERGY order (decreasing wavelength) for scintillation to work properly
  auto      energies = n4::scale_by(hc*eV, {1/0.55, 1/0.36, 1/0.3 , 1/0.26}); // denominator is wavelength in micrometres
  auto energies_cold = n4::scale_by(hc*eV, {1/0.5 , 1/0.4 , 1/0.35, 1/0.27}); // denominator is wavelength in micrometres
  // auto     energies = n4::scale_by(hc*eV, {1/0.9, 1/0.7, 1/0.54, 1/0.35});
  vec_double rindex =                      {1.79  , 1.79  , 1.79 , 1.79  };  //vec_double rindex = {2.2094, 1.7611};
  vec_double  scint =                      {0.0   , 0.1   , 1.0  , 0.0   };
  auto    abslength = n4::scale_by(m    ,  {5     , 5     , 5    , 5     });
  // Values from "Temperature dependence of pure CsI: scintillation light yield and decay time" by Amsler et al
  // "cold" refers to ~77K, i.e. liquid nitrogen temperature
  double scint_yield = my.scint_yield.value_or(50'000 / MeV); // 50000 / MeV in cold
  double time_fast   =  1015 * ns; // only one component at cold temps!
  double time_slow   =  1015 * ns;
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies, rindex)
    .add("SCINTILLATIONCOMPONENT1"   , energies, scint)
    .add("SCINTILLATIONCOMPONENT2"   , energies, scint)
    .add("ABSLENGTH"                 , energies, abslength)
    .add("SCINTILLATIONTIMECONSTANT1", time_fast)
    .add("SCINTILLATIONTIMECONSTANT2", time_slow)
    .add("SCINTILLATIONYIELD"        , scint_yield)
    .add("SCINTILLATIONYIELD1"       ,     0.57   )
    .add("SCINTILLATIONYIELD2"       ,     0.43   )
    .add("RESOLUTIONSCALE"           ,     1.0    )
    .done();
  csi -> GetIonisation() -> SetBirksConstant(0.00152 * mm/MeV);
  csi -> SetMaterialPropertiesTable(mpt);
  return csi;
}

// Refractive index, scintillation spectrum and time constant taken from
//   https://jnm.snmjournals.org/content/jnumed/41/6/1051.full.pdf
G4Material* bgo_with_properties() {
  auto bgo = n4::material("G4_BGO");
  auto       energies = n4::scale_by(hc*eV, {1/0.65, 1/0.48, 1/0.39}); // denominator is wavelength in micrometres
  vec_double scint    =                     {  0.0 ,   1.0 ,   0.0  };
  //double scint_yield = my.scint_yield.value_or(8'500 / MeV); // According to Wikipedia
  double scint_yield = my.scint_yield.value_or(6'000 / MeV); // https://wiki.app.uib.no/ift/images/c/c2/Characterization_of_Scintillation_Crystals_for_Positron_Emission_Tomography.pdf
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies,   2.15)
    .add("SCINTILLATIONCOMPONENT1"   , energies, scint)
    .add("SCINTILLATIONTIMECONSTANT1", 300 * ns)
    .add("SCINTILLATIONYIELD"        , scint_yield)
    .add("RESOLUTIONSCALE"           ,     1.0    ) // TODO what is RESOLUTIONSCALE ?
    .done();
  bgo -> SetMaterialPropertiesTable(mpt);
  return bgo;
}

// TODO get better LYSO material property data
// https://arxiv.org/pdf/2207.06696.pdf
G4Material* lyso_with_properties() {
  auto density = 7.1 * g/cm3;
  auto state = kStateSolid;
  auto               [ fLu ,  fY  ,  fSi ,  fO  ] =
      std::make_tuple(0.714, 0.040, 0.064, 0.182);
  auto lyso = nain4::material_from_elements_F("LYSO", density, {.state=state},
                                              {{"Lu", fLu}, {"Y", fY}, {"Si", fSi}, {"O", fO}});

  auto       energies = n4::scale_by(hc*eV, {1/0.60, 1/0.42, 1/0.20}); // denominator is wavelength in micrometres
  vec_double scint    =                     {  0.0 ,   1.0 ,   0.0  };
  double scint_yield = my.scint_yield.value_or(30'000 / MeV);
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies,  1.82)
    .add("SCINTILLATIONCOMPONENT1"   , energies, scint)
    .add("SCINTILLATIONTIMECONSTANT1", 40 * ns)
    .add("SCINTILLATIONYIELD"        , scint_yield)
    .add("RESOLUTIONSCALE"           ,     1.0    ) // TODO what is RESOLUTIONSCALE ?
    .done();
  lyso -> SetMaterialPropertiesTable(mpt);
  return lyso;
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
  if (my.reflectivity.has_value()) {
    reflectivity = std::vector<double>(energy.size(), my.reflectivity.value());
  }

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
// TODO: implement absorption length from
// https://www.researchgate.net/figure/Absorption-length-1-a-as-a-function-of-wavelength-l-for-Silicon-Actually-not-all-the_fig5_221914026
G4Material* silicon_with_properties() {
  auto si  = n4::material("G4_Si");
  auto mpt = n4::material_properties()
    .add("RINDEX"    , OPTPHOT_ENERGY_RANGE, 4.32)
    .add("EFFICIENCY", OPTPHOT_ENERGY_RANGE, 1) // full absorption at the sipm surface
    .add("ABSLENGTH" , OPTPHOT_ENERGY_RANGE, 10 * nm)
    .done();
  si -> SetMaterialPropertiesTable(mpt);
  return si;
}
