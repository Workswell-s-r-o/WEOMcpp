# Concurrency Notes

`PropertiesWtc640` has two important transaction styles.

## Mode Selection

`Properties::Mode` is defined in:
- `core/include/core/properties/properties.h` (enum starts around line 39)

Available modes and practical difference:
- `SYNC_DIRECT`
  - operations execute synchronously on the caller thread
  - simplest for CLI/tools
  - long operations block the caller
- `ASYNC_QUEUED`
  - operations are queued and processed asynchronously
  - preferred for GUI/apps that must stay responsive
  - requires proper main-thread/event-loop handling to avoid deadlocks

Both reference examples currently use `ASYNC_QUEUED`.

## Properties Transaction

Use for normal reads/writes where exclusive device ownership is not required.

API:
- `createPropertiesTransaction()`
- `tryCreatePropertiesTransaction(timeout)`

Typical usage:
- periodic telemetry reads
- UI-bound property gets/sets

## Exclusive Transaction

Use for operations that must be serialized against other tasks.

API:
- `createConnectionExclusiveTransactionWtc640(cancelRunningTasks)`

Typical usage:
- trigger activation
- image capture
- low-level write/read sequences
- reset-trigger flow

## Practical Rule

Do not keep an exclusive transaction open while calling high-level helpers that open their own exclusive transaction internally (for example `setLensRangeCurrent(...)`).

Pattern:
1. open exclusive scope for trigger/capture/DP sequence
2. close scope
3. call helper that acquires exclusive lock itself
