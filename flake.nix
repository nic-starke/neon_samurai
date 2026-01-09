{
  description = "Neon Samurai - Optimized AVR Toolchain (Source Build)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    # Use nixpkgs 24.05 for AVR GCC 13.x baseline (avoids GCC 15.x inline asm issues)
    nixpkgs-avr.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, nixpkgs-avr, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        # 1. THE OVERLAY
        # This contains the logic to swap sources and apply flags.
        # We use 'hostPlatform' and 'targetPlatform' checks to ensure we 
        # only patch the AVR tools, not the native tools (like CMake/Python).
        avrToolchainOverlay = final: prev: {
          
          # --- AVR-LIBC (The library that runs ON the AVR chip) ---
          avrlibc = if final.stdenv.hostPlatform.isAvr then
            prev.avrlibc.overrideAttrs (oldAttrs: {
              pname = "avr-libc-git";
              version = "2025-git-main";
              
              src = final.fetchFromGitHub {
                owner = "avrdudes";
                repo = "avr-libc";
                rev = "main";
                # ⚠️ UPDATE HASH ON FIRST RUN
                hash = "sha256-jwXnmtIrP1GXuryZPCPiyDXDbA3aOm/dZyZkNXRtS0Q=";
              };

              enableParallelBuilding = true;

              # Dependencies for bootstrapping git repo
              nativeBuildInputs = (oldAttrs.nativeBuildInputs or []) ++ [ 
                final.buildPackages.python3 
                final.buildPackages.automake
                final.buildPackages.autoconf
              ];

              preConfigure = ''
                patchShebangs .
                ./bootstrap
              '';
            })
          else
            prev.avrlibc;

          # --- BINUTILS (Runs on PC, Targets AVR) ---
          # We define a custom attribute to ensure we get the overridden version
          # regardless of how buildPackages is structured.
          # avrBinutilsCustom = ... (Removed to save build time/space)

          # --- GCC (The Compiler: Runs on PC, Targets AVR) ---
          # We override the 'gcc' wrapper to use our custom source.
          # avrGccCustom = ... (Removed to save build time/space)
        };

        # 2. INSTANTIATE THE CROSS ENVIRONMENT
        # Instead of using pkgsCross.avr, we manually import nixpkgs configured for AVR.
        # This allows our overlay to work perfectly without 'overrideScope' errors.
        pkgsAvrCross = import nixpkgs-avr {
          inherit system;
          crossSystem = {
            config = "avr";
            libc = "avrlibc";
          };
          overlays = [ avrToolchainOverlay ];
        };

        # Standard native packages
        pkgs = import nixpkgs { inherit system; };

      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            # 3. USE THE CUSTOM TOOLCHAIN
            # buildPackages = Tools running on YOUR computer (GCC, Binutils)
            pkgsAvrCross.buildPackages.gcc
            pkgsAvrCross.buildPackages.binutils
            
            # The LibC runs on the AVR, so it's in the top level of the cross set
            pkgsAvrCross.avrlibc
            
            # Native tools
            pkgs.avrdude
            pkgs.cmake
            pkgs.ninja
            pkgs.git
            pkgs.pkg-config
          ];

          shellHook = ''
            echo "🎮 Neon Samurai Development Environment"
            echo "========================================="
            echo "AVR GCC:    $(avr-gcc --version | head -n1)"
            echo "AVR LibC:   Built from source (avrdudes/avr-libc)"
            echo "CMake:      $(cmake --version | head -n1)"
            echo ""
            
            export AVR_GCC_PATH="${pkgsAvrCross.buildPackages.gcc}/bin/"
            export AVR_LIBC_PATH="${pkgsAvrCross.avrlibc}/avr/include/"
            
            mkdir -p build
          '';

          CMAKE_EXPORT_COMPILE_COMMANDS = "1";
        };
      }
    );
}