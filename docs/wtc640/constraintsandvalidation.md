# WTC640 Constraints And Validation

This page summarizes guard rails implemented in `wtc640/source/propertieswtc640.cpp`.

## Access Model

Most adapters use `createStatusFunction(readFlags, readMode, writeFlags, writeMode)`:
- device gates: MAIN user vs LOADER
- mode gate: USER mode
- resulting status: read/write, read-only, write-only, or disabled

## Property Status Constraints

Additional runtime constraints are layered on top of access flags.

### Motor Focus Feature Gating

Source: `FOCUS_TYPE_CURRENT`

- Enables `MOTOR_FOCUS_MODE`, `CURRENT_MF_POSITION`, `TARGET_MF_POSITION`, `MAXIMAL_MF_POSITION` only for motorized focus types.
- Enables `LENS_SERIAL_NUMBER`, `LENS_ARTICLE_NUMBER` only for bayonet focus types.

### Image Freeze Constraint

Source: `IMAGE_FREEZE`

- `TEST_PATTERN` is forced read-only when:
  - freeze value is unknown, or
  - freeze is `true`

### Connection Constraint

Source: `PLUGIN_TYPE`

- `UART_BAUDRATE_CURRENT` and `UART_BAUDRATE_IN_FLASH` are disabled for:
  - unknown plugin
  - `PLEORA`
  - `ONVIF`

### Plugin/Video-Format Editability

Source: `PLUGIN_TYPE`

- `VIDEO_FORMAT_CURRENT`:
  - read-only if plugin unknown
  - for USB on non-Apple builds, read-only unless stream is running
- `VIDEO_FORMAT_IN_FLASH`:
  - read-only when plugin unknown or plugin is USB

## Dependency Validators

### Framerate <= Max Framerate

Applied for:
- `FRAMERATE_CURRENT` vs `MAX_FRAMERATE_CURRENT`
- `FRAMERATE_IN_FLASH` vs `MAX_FRAMERATE_IN_FLASH`

Rule:
- framerate must not exceed max framerate
- `FPS_8_57` is treated as always allowed

### FPS Lock Compatibility

Applied for:
- `FPS_LOCK` vs `MAX_FRAMERATE_IN_FLASH`

Rule:
- if `FPS_LOCK == true`, max framerate must be `FPS_8_57`

### Video Format Compatibility By Plugin

Applied for:
- `PLUGIN_TYPE` vs `VIDEO_FORMAT_CURRENT`
- `PLUGIN_TYPE` vs `VIDEO_FORMAT_IN_FLASH`

Rules from `isValidVideoFormat(...)`:
- `USB`: `PRE_IGC`, `POST_COLORING`
- `PLEORA`, `CMOS`: `PRE_IGC`, `POST_IGC`
- `CVBS`, `HDMI`, `ANALOG`: `POST_COLORING`
- `ONVIF`: `PRE_IGC`

## Trigger Access Rules

Trigger writes are checked against current device type before activation.

### CommonTrigger

Items:
- `NUC_OFFSET_UPDATE`
- `CLEAN_USER_DP`
- `SET_SELECTED_PRESET`
- `MOTORFOCUS_CALIBRATION`
- `FRAME_CAPTURE_START`

Rule:
- allowed only on MAIN user firmware

### ResetTrigger

Items and gates:
- `RESET_FROM_LOADER`: LOADER only
- `STAY_IN_LOADER`: LOADER only
- `SOFTWARE_RESET`: MAIN user only
- `RESET_TO_FACTORY_DEFAULT`: MAIN user only
- `RESET_TO_LOADER`: allowed from either side

## Data/Format Validation

### Serial Number

Pattern:
- `^[0-9]{5}-[0-9]{3}-[0-9]{4}$`

Additional check:
- date is parsed from final 4 digits (`YYMM`)
- month must be in `01..12`

### Article Number

`ArticleNumber::createFromString(...)` validates against generated token regex:
- sensor: `WTC640`
- core: `R` or `N`
- sensitivity: `P`, `S`, `U`
- focus: `H25`, `H34`, `E25`, `E34`, `B25`, `B34`
- max framerate token: `9`, `30`, `60`

### Dynamic Preset Address Safety

When creating dynamic preset adapters:
- address must map to FLASH memory
- address range must not conflict with already mapped properties

If either check fails, dynamic preset setup is aborted.

## Runtime Safety Checks

### Connection Validation On Link Switch

`setDataLinkInterface(...)`:
1. resets dynamic adapters
2. probes device identity and status (`testDeviceType`)
3. if probe fails, link is reverted to null and dummy USB adapters are installed

### Connection Lost Signal

`connectionLost()` is emitted when:
- protocol interface reports lost connection, or
- active data link reports lost connection, or
- refresh loop detects camera-not-ready/device-type mismatch

### Capture And Baudrate Constraints

- `captureImages(...)`: if `IMAGE_FREEZE == true`, only one frame may be captured.
- `setCoreBaudrate(...)`: requires UART data link; returns error for non-UART links.
