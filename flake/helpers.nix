{ pkgs, nain4, self }:

{
   shell-shared = {
    G4_DIR = "${pkgs.geant4}";

      shellHook = ''
        export CRYSTAL_LIB=$PWD/install/crystal/lib
        export LD_LIBRARY_PATH=$CRYSTAL_LIB:$LD_LIBRARY_PATH;
        export PKG_CONFIG_PATH=$CRYSTAL_LIB/pkgconfig:$PKG_CONFIG_PATH;
      '';
    };
}
