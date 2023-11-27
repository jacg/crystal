{ self
, nixpkgs # <---- This `nixpkgs` has systems removed e.g. legacyPackages.zlib
, nain4
, ...
}: let
  inherit (nixpkgs.legacyPackages) pkgs;

  shell-shared = {
    G4_DIR = "${pkgs.geant4}";

      shellHook = ''
        export CRYSTAL_LIB=$PWD/install/crystal/lib
        export LD_LIBRARY_PATH=$CRYSTAL_LIB:$LD_LIBRARY_PATH;
        export PKG_CONFIG_PATH=$CRYSTAL_LIB/pkgconfig:$PKG_CONFIG_PATH;
      '';
    };

  make-app = executable: args:
    let
      g4-data = nain4.deps.g4-data-package;
      crystal-app-package = pkgs.writeShellScriptBin executable ''
        export PATH=${pkgs.lib.makeBinPath [ self.packages.crystal ]}:$PATH
        # export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath [ nain4.packages.geant4 ] }:$LD_LIBRARY_PATH

        # TODO replace manual envvar setting with with use of packages' setupHooks
        export G4NEUTRONHPDATA="${g4-data.G4NDL}/share/Geant4-11.0.4/data/G4NDL4.6"
        export G4LEDATA="${g4-data.G4EMLOW}/share/Geant4-11.0.4/data/G4EMLOW8.0"
        export G4LEVELGAMMADATA="${g4-data.G4PhotonEvaporation}/share/Geant4-11.0.4/data/G4PhotonEvaporation5.7"
        export G4RADIOACTIVEDATA="${g4-data.G4RadioactiveDecay}/share/Geant4-11.0.4/data/G4RadioactiveDecay5.6"
        export G4PARTICLEXSDATA="${g4-data.G4PARTICLEXS}/share/Geant4-11.0.4/data/G4PARTICLEXS4.0"
        export G4PIIDATA="${g4-data.G4PII}/share/Geant4-11.0.4/data/G4PII1.3"
        export G4REALSURFACEDATA="${g4-data.G4RealSurface}/share/Geant4-11.0.4/data/G4RealSurface2.2"
        export G4SAIDXSDATA="${g4-data.G4SAIDDATA}/share/Geant4-11.0.4/data/G4SAIDDATA2.0"
        export G4ABLADATA="${g4-data.G4ABLA}/share/Geant4-11.0.4/data/G4ABLA3.1"
        export G4INCLDATA="${g4-data.G4INCL}/share/Geant4-11.0.4/data/G4INCL1.0"
        export G4ENSDFSTATEDATA="${g4-data.G4ENSDFSTATE}/share/Geant4-11.0.4/data/G4ENSDFSTATE2.3"

        exec ${executable} ${args}
      '';
    in { type = "app"; program = "${crystal-app-package}/bin/${executable}"; };

  args-from-cli = ''"$@"'';

  in {

    packages.default = self.packages.crystal;

    packages.crystal = pkgs.stdenv.mkDerivation {
      pname = "crystal";
      version = "0.0.0";
      src = "${self}/src";
      nativeBuildInputs = [];
      buildInputs = [ nain4.packages.nain4 ];
    };

    # Executed by `nix run <URL of this flake> -- <args?>`
    apps.default = self.apps.crystal;

    apps.crystal = make-app "crystal" "--macro-path ${self}/macs ${args-from-cli}";
    apps.stats   = make-app "stats"                                args-from-cli;

    # Used by `direnv` when entering this directory (also by `nix develop <URL to this flake>`)
    devShell = self.devShells.clang;

    # Activated by `nix develop <URL to this flake>#clang`
    devShells.clang = pkgs.mkShell.override { stdenv = nain4.packages.clang_16.stdenv; } (shell-shared // {
      name = "crystal-clang-devenv";
      packages = nain4.deps.dev-shell-packages ++ [ nain4.packages.clang_16 ];
    });

    # Activated by `nix develop <URL to this flake>#gcc`
    devShells.gcc = pkgs.mkShell (shell-shared // {
      name = "crystal-gcc-devenv";
      packages = nain4.deps.dev-shell-packages;
    });

    # 1. `nix build` .#singularity
    # 2. `scp result <me>@lxplus7.cern.ch:hello.img`
    # 3. [on lxplus] `singularity run hello.img`
    packages.singularity = pkgs.singularity-tools.buildImage {
      name = "crystal";
      contents = [ self.apps.crystal.program ];
      runScript = "${self.apps.crystal.program} $@";
      diskSize = 10240;
      memSize = 5120;
    };

  }
