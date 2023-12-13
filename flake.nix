{
  description = "Monte Carlo for PET crystal studies";

  inputs = {
    nain4  .url     = "github:jacg/nain4";
    nosys  .follows = "nain4/nosys";
    nixpkgs.follows = "nain4/nixpkgs";
    newpkgs.url     = "github:NixOS/nixpkgs/nixos-23.11";
  };

  outputs = inputs @ {
    nosys,
    nixpkgs, # <---- This `nixpkgs` still has the `system` e.g. legacyPackages.${system}.zlib
    newpkgs,
    ...
  }: let outputs = import ./flake/outputs.nix;
         systems = inputs.nain4.contains-systems.systems;
    in nosys (inputs // { inherit systems; }) outputs;

}
