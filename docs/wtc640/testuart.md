# testuart

Source:
- [testuart.cpp](../../tests/testuart.cpp)

Target:
- `testuart`

## What This Test Covers

`testuart` is a hardware integration test for UART-connected WTC640 devices. It reuses the same connection input values as the examples and validates:
- common integration flow (stream, capture, trigger, reset, reconnect),
- property read/reset/set coverage across available properties,
- UART baudrate write path (lowest/highest available),
- repeated reset/reconnect stability.

## Build And Run

Configure with tests enabled:

```bash
cmake -S . -B build -DW_BUILD_TEST:BOOL=ON -DW_CORE_RESULT_STRING_WITH_DETAIL:BOOL=ON
```

Build:

```bash
cmake --build build --config Release
```

Run directly:

```bash
build/tests/Release/testuart.exe <serial_number> <system_location>
```

Inputs are the same pair used by the examples.
On Linux, `system_location` is usually `/dev/ttyACM0` (or another `/dev/ttyACM*`), not `/dev/tty0`.

## Optional: Run Through CTest

`ctest` can run the target, but `testuart` needs the UART arguments above for real hardware runs, so direct execution is the normal workflow.
