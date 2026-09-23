# Block Break Analyzer

Standalone native Minecraft Bedrock mod for the supplied 1.26.51.01_RC0 ARM64 build.

## HUD

While breaking a block, the overlay shows exactly three lines:

```text
Stone

Required tool: Pickaxe
Estimated break time: 0.8s
```

The UI uses the bundled Minecraft TTF registered as the `minecraft` font, measures the current HUD surface at render time, scales from the shorter screen dimension, and clamps itself inside the screen bounds. This makes it suitable for phones, tablets, and landscape/portrait surfaces supported by the preloader HUD API.

## How the analyzer works

- `GameModeStartDestroyBlock` captures the block position being broken.
- `NormalTick` samples the game's live `mDestroyProgress` field at offset `0x24`.
- `BlockSourceGetBlock` resolves the current block object from the player's dimension.
- The block's full identifier is read through the documented source offsets (`Block +0x68 -> BlockType +0x88 -> NameInfo +0x40 -> HashedString +0x8`).
- Break time is estimated from the observed progress rate, so the displayed value reflects the actual current game conditions rather than a fixed table.
- `RenderLevel` submits the cached HUD snapshot using the preloader drawing API.

The mod fails closed: if any required signature cannot be resolved or a hook cannot be installed, it unregisters its menu entry and does not leave a partially installed hook set.

## Build

Android ARM64 release build:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
./scripts/build.sh
```

The script produces `BlockBreakAnalyzer.levipack`.

Local compiler/package smoke test:

```bash
./scripts/smoke_test.sh
```

The host smoke test checks C++ compilation, package inputs, manifest structure, and that no source/project file contains a dependency on the reference framework.
