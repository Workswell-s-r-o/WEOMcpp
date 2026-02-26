# CaptureFlowExample

Source:
- [captureflowexample.cpp](../../example/captureflowexample.cpp)
- [captureflowexample.h](../../example/captureflowexample.h)

Target:
- `CaptureFlowExample`

## What This Example Does

`CaptureFlowExample` is a full workflow example. It shows how to:
1. connect to camera,
2. capture frames,
3. run reset trigger and reconnect,
4. loop through valid preset IDs,
5. add one dummy dead pixel per loop iteration,
6. trigger dead-pixel clean and verify the property is empty,
7. stop safely by pressing Enter.

## Runtime Flow

1. Connect via `connectUartAuto(...)`.
2. Capture images with `captureImages(...)`.
3. Execute `SOFTWARE_RESET` trigger and call `reconnectCoreAfterReset(...)`.
4. Read `ALL_VALID_LENS_RANGES`.
5. For each preset:
   - read `DEAD_PIXELS_CURRENT`,
   - insert dummy dead pixel and write property,
   - trigger `NUC_OFFSET_UPDATE`,
   - trigger `CLEAN_USER_DP`,
   - re-read dead pixels and log result,
   - apply preset with `setLensRangeCurrent(...)`.
6. `main()` runs the workflow on a worker thread.
7. Pressing Enter calls `requestStop()` and cancels progress.

## Why It Is Useful

- Validates the highest-risk operations in one run: capture, trigger path, reset/reconnect.
- Demonstrates dead-pixel write + clean side effects.
- Demonstrates correct preset selection from `ALL_VALID_LENS_RANGES`.
- Shows graceful shutdown behavior for long-running workflows.

## Build And Run

```bash
cmake -S . -B build -DW_BUILD_EXAMPLE:BOOL=ON -DW_CORE_RESULT_STRING_WITH_DETAIL:BOOL=ON
cmake --build build --config Release --target CaptureFlowExample
build/example/Release/CaptureFlowExample.exe <serial_number> <system_location>
```

On Linux, `system_location` is usually `/dev/ttyACM0` (or another `/dev/ttyACM*`), not `/dev/tty0`.

Press Enter in the console to stop the example.

## If Capture Fails On Linux

Use the `Linux V4L2 Diagnostics (Acquisition Failures)` section in [troubleshooting.md](./troubleshooting.md).
