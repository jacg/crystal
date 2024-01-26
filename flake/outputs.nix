{ self
, nixpkgs # <---- This `nixpkgs` has systems removed e.g. legacyPackages.zlib
, nain4
, pet-materials
, ...
}: let
  inherit (nixpkgs.legacyPackages) pkgs;
  inherit (import ./helpers.nix { inherit nain4 pkgs self; }) shell-shared;
  inherit (nain4.deps) args-from-cli make-app;
  dev-shell-packages = [pkgs.arrow-cpp pet-materials.packages.pet-materials];
  in {

    packages.default = self.packages.crystal;

    packages.crystal = pkgs.stdenv.mkDerivation {
      pname = "crystal";
      version = "0.0.0";
      src = "${self}/src";
      postInstall = "${pkgs.coreutils}/bin/cp -r ${self}/macs $out";
      nativeBuildInputs = [];
      buildInputs = [ nain4.packages.nain4 pet-materials.packages.pet-materials pkgs.arrow-cpp];
    };

    # Executed by `nix run <URL of this flake> -- <args?>`
    apps.default = self.apps.crystal;

    # Executed by `nix run <URL of this flake>#crystal`
    apps.crystal = make-app {
      executable = "crystal";
      args = "--macro-path ${self}/macs ${args-from-cli}";
      package = self.packages.crystal;
    };

    # Executed by `nix run <URL of this flake>#stats`
    apps.stats = make-app {
      executable = "stats";
      args = args-from-cli;
      package = self.packages.crystal;
    };

    # Used by `direnv` when entering this directory (also by `nix develop <URL to this flake>`)
    devShell = self.devShells.clang;

    # Activated by `nix develop <URL to this flake>#clang`
    devShells.clang = pkgs.mkShell.override { stdenv = nain4.packages.clang_16.stdenv; } (shell-shared // {
      name = "crystal-clang-devenv";
      packages = nain4.deps.dev-shell-packages ++
                 dev-shell-packages ++
                 [
        nain4.packages.clang_16
        (pkgs.python3.withPackages(ps: with ps; [parquet pandas ipython pyarrow]))
      ];
    });

    # Activated by `nix develop <URL to this flake>#gcc`
    devShells.gcc = pkgs.mkShell (shell-shared // {
      name = "crystal-gcc-devenv";
      packages = nain4.deps.dev-shell-packages ++ dev-shell-packages;
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
