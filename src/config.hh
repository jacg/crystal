#pragma once

#include <G4GenericMessenger.hh>
#include <G4Material.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4UnitsTable.hh>
#include <Randomize.hh>

#include <n4-run-manager.hh>

#include <cstdint>
#include <optional>
#include <unordered_map>


enum class scintillator_type_enum { lyso, bgo, csi };
enum class config_type_enum       { lyso, bgo, csi, csi_mono };

struct scint_parameters {
  scintillator_type_enum scint;
  double   scint_depth;
  unsigned n_sipms_x;
  unsigned n_sipms_y;
  double   sipm_size;
};

struct scint_overrides {
  std::optional<scintillator_type_enum> scint;
  std::optional<double>                 scint_depth;
  std::optional<unsigned>               n_sipms_x;
  std::optional<unsigned>               n_sipms_y;
  std::optional<double>                 sipm_size;
};

std::string scintillator_type_to_string(scintillator_type_enum s);
scintillator_type_enum string_to_scintillator_type(std::string s);

std::string config_type_to_string(config_type_enum s);
config_type_enum string_to_config_type(std::string s);

struct config {
private:
  scint_parameters        scint_params_;
  scint_overrides         overrides           =  {};
public:
  double                  sipm_thickness      =   1    * mm;
  double                  reflector_thickness =   0.25 * mm;
  double                  particle_energy     = 511    * keV;
  int                     physics_verbosity   =   0;
  long                    seed                = 123456789;
  bool                    debug               = false ;
  std::optional<G4double> scint_yield         = std::nullopt;
  size_t                  event_threshold     = 1;
  size_t                   sipm_threshold     = 1;
  std::optional<double>   reflectivity        = std::nullopt;
  std::string             generator           = "gammas_from_outside_crystal";
  std::string             outfile             = "crystal-out.parquet";
  int64_t                 chunk_size          = 1024; // TODO find out what chuck_size default should be
  std::string             compression         = "brotli";

  config();

  G4ThreeVector scint_size() const;
  const std::vector<G4ThreeVector>& sipm_positions() const;
  const scint_parameters scint_params() const;
  size_t n_sipms() const;
  std::unordered_map<std::string, std::string> as_map();
  std::unordered_map<std::string, std::string> cli_args() {return n4::run_manager::get_ui().arg_map();}

private:

  void set_config_type(const std::string& s);
  void set_scint      (const std::string& s) { overrides.scint = string_to_scintillator_type(s); }
  void set_scint_depth(double   d)           { overrides.scint_depth = d; }
  void set_n_sipms_x  (unsigned n)           { overrides.n_sipms_x   = n; sipm_positions_need_recalculating = true; }
  void set_n_sipms_y  (unsigned n)           { overrides.n_sipms_y   = n; sipm_positions_need_recalculating = true; }
  void set_n_sipms_xy (unsigned n)           { set_n_sipms_x(n); set_n_sipms_y(n); }
  void set_sipm_size  (double   d)           { overrides.sipm_size   = d; sipm_positions_need_recalculating = true; }

  void set_scint_yield(double   y) { scint_yield = y; }
  void set_random_seed(long  seed) { G4Random::setTheSeed(seed); }
  void set_reflectivity(double  r) { reflectivity = r; }
  G4GenericMessenger* msg;

  mutable std::vector<G4ThreeVector> sipm_positions_;
  mutable bool                       sipm_positions_need_recalculating = true;
  void recalculate_sipm_positions() const;
};

extern config my;

G4Material* scintillator_material(scintillator_type_enum type);
