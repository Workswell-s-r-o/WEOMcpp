# Quickstart

This is the minimum path to a working WTC640 session.

## 1) Create Properties Instance

```cpp
auto indicator = std::make_shared<MainThreadIndicator>();
auto properties = core::PropertiesWtc640::createInstance(
    core::Properties::Mode::ASYNC_QUEUED,
    indicator,
    nullptr // eBus plugin
);
```

## 2) Connect To Device

```cpp
auto notifier = core::ProgressNotifier::createProgressNotifier();
auto progress = notifier->getOrCreateProgressController();

auto stateTx = properties->createConnectionStateTransaction();
std::vector<core::connection::SerialPortInfo> ports{portInfo};
auto connectResult = stateTx.connectUartAuto(ports, progress);
```

Linux note: for `systemLocation`, use the actual USB ACM port (for example `/dev/ttyACM0`), not `/dev/tty0`.

## 3) Read A Property

```cpp
auto tx = properties->createPropertiesTransaction();
auto temp = tx.getValue<double>(core::PropertyIdWtc640::SHUTTER_TEMPERATURE);
```

## 4) Run An Exclusive Operation

```cpp
auto ex = properties->createConnectionExclusiveTransactionWtc640(false);
auto capture = ex.captureImages(1, progress);
```

## 5) Use The Two Reference Examples

- `PaletteLoopExample` ([paletteloopexample.cpp](../../example/paletteloopexample.cpp)):
  streaming + transaction lock behavior + palette cycling
- `CaptureFlowExample` ([captureflowexample.cpp](../../example/captureflowexample.cpp)):
  capture + triggers + reset/reconnect + preset loop + dead-pixel add/clean verification

Build:

```bash
cmake -S . -B build -DW_BUILD_EXAMPLE:BOOL=ON -DW_CORE_RESULT_STRING_WITH_DETAIL:BOOL=ON
cmake --build build --config Release --target PaletteLoopExample CaptureFlowExample
```
