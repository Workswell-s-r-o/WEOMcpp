# Troubleshooting

## Connect Fails

Symptoms:
- `connectUartAuto` returns error

Checks:
1. verify serial/system location pair
2. on Linux, verify you are using the correct ACM device path (for example `/dev/ttyACM0`), not `/dev/tty0`
3. verify no other process owns the port
4. retry with a fresh `ConnectionStateTransaction`

## Capture Fails

Symptoms:
- `captureImages(...)` fails

Checks:
1. device is connected and responsive
2. `IMAGE_FREEZE` is not forcing single-image mode when requesting multiple frames
3. run capture inside exclusive transaction
4. build with `-DW_CORE_RESULT_STRING_WITH_DETAIL:BOOL=ON` for full result errors in your troubleshooting setup

## Linux V4L2 Diagnostics (Acquisition Failures)

If streaming/capture fails on Linux, run these baseline checks:

```bash
ls -l /dev/video*
v4l2-ctl --list-devices
v4l2-ctl --device=/dev/video0 --all
v4l2-ctl --device=/dev/video0 --list-formats-ext
fuser -v /dev/video0
dmesg | grep -Ei "uvc|video|v4l2|usb" | tail -n 100
```

Optional direct acquisition probe:

```bash
ffmpeg -hide_banner -f v4l2 -i /dev/video0 -frames:v 1 -f null -
```

If these fail, fix the V4L2/driver/device access issue first, then retry the SDK example.

## Trigger Fails

Symptoms:
- common/reset trigger call returns access/timeout errors

Checks:
1. current device type allows that trigger
2. run through exclusive transaction
3. after reset trigger, reconnect explicitly

## Dead Pixels Not Empty After Clean

Expected after `CLEAN_USER_DP`:
- `DEAD_PIXELS_CURRENT` map size should become `0`

If not:
1. confirm trigger result is `OK`
2. re-read property in same transaction after trigger call
3. retry in a new exclusive transaction

## Preset Set Fails

Checks:
1. read `ALL_VALID_LENS_RANGES` and use returned IDs
2. avoid calling `setLensRangeCurrent(...)` while holding another exclusive transaction
