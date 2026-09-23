# Build / validation status

Date: 2026-09-23

## Passed

- Host C++20 shared-library build: `PASS`
- Standalone-project dependency/name check: `PASS`
- `PLGetModRegistration` export: `PASS`
- Required preloader symbol declarations: `PASS`
- Required signature scan against supplied `libminecraftpe.so`: `PASS`
- Signature uniqueness in supplied binary: `PASS`
- Package layout/manifest script: validated with the host smoke library

Resolved signatures in the supplied binary:

- `NormalTick` -> `0xAAC7D68`
- `GameModeStartDestroyBlock` -> `0xF89FA58`
- `GameModeStopDestroyBlock` -> `0xF8A0D6C`
- `BlockSourceGetBlock` -> `0xFB91558`
- `RenderLevel` -> `0xB2F0F60`

## Environment limitation

The execution environment does not contain an Android NDK, so an actual `arm64-v8a` Android link could not be performed here. `scripts/build.sh` checks for the NDK and exits with a clear prerequisite message instead of generating a misleading non-Android artifact.
