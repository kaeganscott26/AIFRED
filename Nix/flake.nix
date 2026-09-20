{
  description = "Maestro MacBook Neo AI/ML and Audio Tooling Stack for AIFRED and FORGE";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
        
        # Core Apple SDK Frameworks needed for hardware-accelerated AI and Audio
        appleFrameworks = with pkgs.darwin.apple_sdk.frameworks; [
          # AI/ML & Core Compute Accelerators
          CoreML                  # On-device model execution (Optimal for 8GB Neo)
          Metal                   # Direct GPU tensor access 
          MetalPerformanceShaders # Low-level optimized primitives for A18Pro GPU
          Accelerate              # High-performance vector/matrix math (CPU side)
          
          # Audio System Bindings (Essential for JUCE/AIFRED VST Compilation)
          AudioToolbox
          CoreAudio
          CoreMIDI
          Cocoa
          WebKit
        ];
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # C++ & Audio Plugin Development (AIFRED Support)
            cmake
            ninja
            pkg-config
            llvmPackages_latest.clang

            # Lightweight Local Execution & Tooling (FORGE Support)
            nodejs_22 # High efficiency runtime for context routing/orchestration
            git
            sqlite    # Lightweight state memory engine rather than heavy DB engines
            
            # Hardware-Optimized Python Stack
            (python311.withPackages (ps: with ps; [
              pip
              numpy
              # Note: Install mlx and coremltools inside the venv to guarantee 
              # direct wheel hooks into your preloaded Maestro framework paths
            ]))
          ] ++ appleFrameworks;

          # Environment Hooks to map Apple SDK paths for compilers & Python
          shellHook = ''
            echo "════════════════════════════════════════════════════════════"
            echo "🚀 Maestro MacBook Neo A18Pro Dev Environment Loaded"
            echo "💡 Memory Constraint Warning: Enforcing Apple Silicon Native ML Engine"
            echo "════════════════════════════════════════════════════════════"
            
            # Force JUCE/CMake and clang to find the correct SDK Framework pathways
            export C_INCLUDE_PATH="${pkgs.lib.makeSearchPathOutput "dev" "include" appleFrameworks}:$C_INCLUDE_PATH"
            export CPLUS_INCLUDE_PATH="${pkgs.lib.makeSearchPathOutput "dev" "include" appleFrameworks}:$CPLUS_INCLUDE_PATH"
            export FRAMEWORK_SEARCH_PATHS="${pkgs.lib.concatStringsSep " " (map (f: "${f}/Library/Frameworks") appleFrameworks)}"
            
            # Setup isolated Python virtual environment to leverage native extensions safely
            if [ ! -d ".venv" ]; then
              python3 -m venv .venv
              source .venv/bin/activate
              pip install --upgrade pip
              # mlx provides unified zero-copy array operations directly on A18Pro GPU/CPU
              pip install mlx coremltools
            else
              source .venv/bin/activate
            fi
            
            echo "✅ Python VirtualEnv loaded with native [MLX] and [CoreMLTools]"
            echo "✅ C++ Audio compiler flags paths successfully linked to local Darwin SDK"
          '';
        };
      });
}
