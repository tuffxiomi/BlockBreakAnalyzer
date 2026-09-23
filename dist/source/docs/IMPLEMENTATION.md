# Implementation notes

Target Minecraft build: `1.26.51.01_RC0`.

Required game signatures are copied as byte patterns with wildcard bytes; they are resolved at runtime inside `libminecraftpe.so` rather than hard-coded as absolute addresses.

Required signatures:
- `NormalTick`
- `GameModeStartDestroyBlock`
- `GameModeStopDestroyBlock`
- `BlockSourceGetBlock`
- `RenderLevel`

The render hook uses the signature's known callable form:
`void(void* self, void* screenContext, void* a3)`.

The break hook uses the known callable form:
`bool(void* gameMode, const void* position, uint8_t face, bool* destroyed)`.

The stop hook uses:
`void(void* gameMode, const void* position)`.

The block lookup uses:
`const void*(void* blockSource, const void* blockPosition, int32_t layer)`.

The module intentionally does not ship the reference framework's source, headers, build files, or libraries. It only contains the minimal public preloader ABI declarations needed to call the loader/mod-menu APIs.
