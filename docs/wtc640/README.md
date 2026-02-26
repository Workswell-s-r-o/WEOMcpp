# WTC640 Docs Index

This folder contains focused docs for integrating and debugging `core::PropertiesWtc640`.

## File Map

- [quickstart.md](./quickstart.md)
  - Purpose: minimal setup to build and run the two reference examples.
  - Use when: you want to get from clone to first successful device interaction.

- [paletteloopexample.md](./paletteloopexample.md)
  - Purpose: explain exactly what `PaletteLoopExample` does and why.
  - Use when: you are learning stream + transaction behavior.

- [captureflowexample.md](./captureflowexample.md)
  - Purpose: explain exactly what `CaptureFlowExample` does and why.
  - Use when: you are learning capture + trigger + reset + preset flows.

- [testuart.md](./testuart.md)
  - Purpose: explain what UART hardware integration test `testuart` validates.
  - Use when: you want to run integration checks with the same UART input pair as examples.

- [concurrencynotes.md](./concurrencynotes.md)
  - Purpose: transaction/locking rules and common misuse patterns.
  - Use when: calls fail intermittently or lock contention appears.

- [constraintsandvalidation.md](./constraintsandvalidation.md)
  - Purpose: validation and access rules enforced by `propertieswtc640.cpp`.
  - Use when: writes fail, are read-only, or are rejected by dependency checks.

- [propertygroups.md](./propertygroups.md)
  - Purpose: source-of-truth map for where properties and adapters are defined.
  - Use when: you need to locate IDs and implementation quickly.

- [troubleshooting.md](./troubleshooting.md)
  - Purpose: failure-oriented checks, including Linux `v4l2` diagnostics.
  - Use when: capture/acquisition/reset/trigger flows fail in real systems.

## Reference Examples

- `PaletteLoopExample`: [paletteloopexample.cpp](../../example/paletteloopexample.cpp)
- `CaptureFlowExample`: [captureflowexample.cpp](../../example/captureflowexample.cpp)
