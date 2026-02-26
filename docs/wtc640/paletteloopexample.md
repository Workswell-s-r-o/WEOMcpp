# PaletteLoopExample

Source:
- [paletteloopexample.cpp](../../example/paletteloopexample.cpp)
- [paletteloopexample.h](../../example/paletteloopexample.h)

Target:
- `PaletteLoopExample`

## What This Example Does

`PaletteLoopExample` shows how to:
1. connect to the device via UART,
2. start an RGB stream,
3. run one thread that continuously reads frames,
4. run another thread that periodically changes palette index under an exclusive transaction,
5. observe transaction lock contention safely with `tryCreatePropertiesTransaction(...)`.

This is a good first example for understanding stream + properties interaction.

## Runtime Flow

1. Build `PropertiesWtc640` with `Mode::ASYNC_QUEUED`.
2. Connect using `connectUartAuto(...)`.
3. Acquire stream with `getOrCreateStream(...)` and start `ImageData::Type::RGB`.
4. Start two threads:
   - `videoThread()`:
     - tries short property transactions (`SHUTTER_TEMPERATURE` read),
     - reads RGB frames.
   - `mainThread()`:
     - opens exclusive transaction,
     - reads `PALETTE_INDEX_CURRENT`,
     - writes next index.
5. Main process waits for Enter key and exits.

## Why It Is Useful

- Demonstrates when to use normal vs exclusive transactions.
- Shows expected behavior when a normal transaction cannot acquire lock quickly.
- Provides a small but realistic loop you can adapt for UI palette controls.

## Build And Run

```bash
cmake -S . -B build -DW_BUILD_EXAMPLE:BOOL=ON -DW_CORE_RESULT_STRING_WITH_DETAIL:BOOL=ON
cmake --build build --config Release --target PaletteLoopExample
build/example/Release/PaletteLoopExample.exe <serial_number> <system_location>
```

On Linux, `system_location` is usually `/dev/ttyACM0` (or another `/dev/ttyACM*`), not `/dev/tty0`.

Use your device port values from enumeration output (or OS device manager).
