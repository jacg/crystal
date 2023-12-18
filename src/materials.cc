#include <config.hh>
#include <materials.hh>

#include <n4-all.hh>

#include <G4MaterialPropertiesTable.hh>
#include <G4SystemOfUnits.hh>

const             double  OPTPHOT_MIN_ENERGY  {1.00*eV};
const             double  OPTPHOT_MAX_ENERGY  {8.21*eV};
const std::vector<double> OPTPHOT_ENERGY_RANGE{OPTPHOT_MIN_ENERGY, OPTPHOT_MAX_ENERGY};

std::pair<std::vector<double>, std::vector<double>> csi_scint_spectrum() {
  // From R. Soleti
  auto energies = n4::const_over(c4::hc/nm, {  460,   400,   380,   340,   320,   300,   280,   260}); // wl in nm
  auto spectrum = n4::scale_by  (0.01     , {    4,    10,    29,    67,    88,    29,    10,     2});
  return {std::move(energies), std::move(spectrum)};
}

G4Material* csi_with_properties() {
  auto csi = n4::material("G4_CESIUM_IODIDE");
  // rindex: values taken from "Optimization of Parameters for a CsI(Tl) Scintillator Detector Using GEANT4-Based Monte Carlo..." by Mitra et al (mainly page 3)
  //  scint: values from Fig. 2 in "A New Scintillation Material: Pure CsI with 10ns Decay Time" by Kubota et al (these are approximate...)
  // must be in increasing ENERGY order (decreasing wavelength) for scintillation to work properly

  auto [energies, spectrum] = csi_scint_spectrum();
  // latest numbers from https://refractiveindex.info/?shelf=main&book=CsI&page=Querry
  auto rindex = n4::scale_by (1.0, {1.766, 1.794, 1.806, 1.845, 1.867, 1.902, 1.955, 2.043});
  // Values from "Temperature dependence of pure CsI: scintillation light yield and decay time" by Amsler et al
  // "cold" refers to ~77K, i.e. liquid nitrogen temperature
  double scint_yield = my.scint_yield.value_or(50'000 / MeV); // 50000 / MeV in cold
  double time_fast   =  1015 * ns; // only one component at cold temps!
  double time_slow   =  1015 * ns;
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies, rindex)
    .add("SCINTILLATIONCOMPONENT1"   , energies, spectrum)
    .add("SCINTILLATIONCOMPONENT2"   , energies, spectrum)
    .add("ABSLENGTH"                 , energies, 5*m)
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
  auto bgo      = n4::material("G4_BGO");
  auto energies = n4::const_over(c4::hc/nm, {650, 480, 390}); // wl in nm
  auto spectrum = n4::scale_by  (0.01     , {  0, 100,   0});
  //double scint_yield = my.scint_yield.value_or(8'500 / MeV); // According to Wikipedia
  double scint_yield = my.scint_yield.value_or(6'000 / MeV); // https://wiki.app.uib.no/ift/images/c/c2/Characterization_of_Scintillation_Crystals_for_Positron_Emission_Tomography.pdf
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies, 2.15)
    .add("SCINTILLATIONCOMPONENT1"   , energies, spectrum)
    .add("SCINTILLATIONTIMECONSTANT1", 300 * ns)
    .add("SCINTILLATIONYIELD"        , scint_yield)
    .add("RESOLUTIONSCALE"           ,     1.0    ) // TODO what is RESOLUTIONSCALE ?
    .done();
  bgo -> SetMaterialPropertiesTable(mpt);
  return bgo;
}

G4Material* lyso_material() {
  auto density = 7.1 * g/cm3;
  auto               [ fLu ,  fY  ,  fSi ,  fO  ] =
      std::make_tuple(0.714, 0.040, 0.064, 0.182);

  return nain4::material_from_elements_F("LYSO", density, {.state=kStateSolid},
                                         {{"Lu", fLu}, {"Y", fY}, {"Si", fSi}, {"O", fO}});

}

// TODO get better LYSO material property data
// https://arxiv.org/pdf/2207.06696.pdf
G4Material* lyso_with_properties() {
  auto lyso = lyso_material();
  auto energies = n4::const_over(c4::hc/nm, {600, 420, 200}); // wl in nm
  auto spectrum = n4::scale_by  (0.01     , {  0, 100,   0});
  double scint_yield = my.scint_yield.value_or(30'000 / MeV);
  auto mpt = n4::material_properties()
    .add("RINDEX"                    , energies, 1.82)
    .add("SCINTILLATIONCOMPONENT1"   , energies, spectrum)
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


G4Material* optical_gel() {
  // Silicone resin with a methyl group
  // (https://en.wikipedia.org/wiki/Silicone_resin)
  auto name = "OpticalSilicone";
  auto density = 1.05*g/cm3;
  return n4::material_from_elements_N( name
                                     , density
                                     , {.state=kStateSolid}
                                     , { {"H", 3}, {"C", 1}, {"O", 1}, {"Si", 1} });
}

G4MaterialPropertiesTable* optical_gel_properties() {
  // gel NyoGel OCK-451
  auto r_index_fn = [] (auto e) {
    auto wl = c4::hc / e;
    static const auto  const_term = 1.4954;
    static const auto square_term = 0.008022 * um*um;
    return const_term + square_term/wl/wl;
  };
  auto [r_energies, r_index] = n4::interpolate(r_index_fn, 100, OPTPHOT_MIN_ENERGY, OPTPHOT_MAX_ENERGY);

  // ABSORPTION LENGTH
  // Values estimated from printed plot (to be improved).
  const auto very_long = 1e6;
  auto abs_energies = n4::scale_by(eV, {OPTPHOT_MIN_ENERGY/eV,
                                           1.70,   1.77,    2.07,   2.48,   2.76,  2.92,
                                           3.10,   3.31,    3.54,   3.81,   4.13,
                                        OPTPHOT_MAX_ENERGY/eV});
  auto abs_length = n4::scale_by(mm, {very_long,
                                      very_long, 1132.8, 1132.8 , 1132.8, 666.17, 499.5,
                                          399.5,  199.5,  132.83,   99.5,   4.5 ,
                                        4.5});

  return n4::material_properties()
    .add("RINDEX", r_energies, r_index)
    .add("ABSLENGTH", abs_energies, abs_length)
    .done();
}

G4Material* optical_gel_with_properties() {
  auto gel = optical_gel();
  gel -> SetMaterialPropertiesTable(optical_gel_properties());
  return gel;
}

std::pair<std::vector<double>, std::vector<double>> sipm_pde() {
  auto energies = n4::const_over(c4::hc/nm, { 908.37, 820.02, 730.61, 635.70, 470.29,
                                              407.14, 364.92, 347.73, 304.17, 284.26});
  auto pde      = n4::scale_by  (0.01     , {   3.49,   8.04,  15.79,  28.41,  49.74,
                                               45.62,  35.37,  35.39,  26.60,   9.14});
  return {energies, pde};
}
