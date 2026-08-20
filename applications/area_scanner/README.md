# Area Scanner

Zone monitoring for the uRAD Industrial (IWR6843AoP): tracking of moving
people and objects **plus** detection of newly added static objects, with
configurable critical/warning occupancy zones around the sensor.

Typical use cases:

- **Machine safety**: slow down or stop a machine when a person or vehicle
  enters (or is about to enter) a protection zone, with early warning based
  on the approach speed.
- **Blocked area detection**: detect boxes, carts, pallets or equipment
  left inside a keep-clear zone (emergency exits, robot lanes, docking
  areas) — even though they do not move.
- **Autonomous vehicles / AGVs**: scan the area ahead for both moving and
  stationary obstacles.

## 1. How it works

The firmware runs two detection chains in parallel plus a tracker
(processing chain summary from the TI Area Scanner user guide):

1. **Dynamic chain** — the classic range-Doppler processing of the
   out-of-box demo with static clutter removal enabled, so it detects only
   *moving* reflectors. Its point cloud is fed to the group tracker
   (GTRACK), which groups points and tracks each object with position,
   velocity and acceleration.
2. **Static chain** — a range-angle heatmap algorithm. During the first
   **15 frames after configuration** the device records the empty scene
   (walls, floor, fixtures) as a calibration heatmap. Afterwards, each
   frame's heatmap is compared against the recording: significant new peaks
   are reported as **newly added static objects**. Objects present during
   the calibration are part of the background and are never reported.
3. **Zone occupancy** — computed on the host by the uRAD client, exactly
   like the TI MATLAB visualizer:
   - **Critical zone** (default 0–2 m radial): triggered by *any*
     detection inside it — a dynamic point, a static point or a track.
   - **Warning zone** (default 2–4 m radial): triggered when a tracked
     object's current position, or its position projected
     `projection-time` seconds ahead (default 2 s, constant velocity), is
     inside it. Static points outside the critical zone never trigger the
     warning zone.

Mounting recommendations (TI user guide): elevate the sensor at least 1 m
from the ground, Z-axis towards the ceiling, Y-axis towards the scene,
tilted ~10° downwards. Keep the scene clear of temporary objects and
people during the 15-frame calibration; if the background changes or the
sensor moves, reset and reconfigure to recalibrate.

## 2. Required firmware

| Binary | Board |
|---|---|
| `area_scanner_68xx_demo_aop.bin` ([Releases](../../../../releases)) | uRAD Industrial (IWR6843AoP) |

The binary is the prebuilt AOP image from the TI Radar Toolbox 4.00.00.05
(only compatible with ES2.0 silicon, which the uRAD Industrial uses).

Flashing (see chapter 3 of the [uRAD Industrial user
manual](../../docs/user-manual-en.pdf) for the DIP switch positions):

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Set the DIP switches to **flashing mode** and power-cycle the board.
3. In UniFlash load `area_scanner_68xx_demo_aop.bin` as *meta image 1* and
   flash.
4. Restore the **functional mode** switches and power-cycle again.

## 3. Chirp configurations provided

All four files are the AOP variants from the TI Radar Toolbox
([`chirp_config/`](chirp_config)); they differ only in the `profileCfg`
line (bandwidth / TX power) and one CFAR threshold:

| File | Bandwidth | TX power | Range resolution | Max range | Max radial velocity |
|---|---|---|---|---|---|
| `area_scanner_68xx_AOP.cfg` (default) | ~3.45 GHz | +12 dBm | 0.070 m | 14.4 m | 2.45 m/s |
| `..._full_power_full_bandwidth.cfg` | 3.95 GHz | +12 dBm | 0.047 m | 10.8 m | 2.25 m/s |
| `..._low_power_full_bandwidth.cfg` | 3.95 GHz | −12 dBm | 0.047 m | 10.8 m | 2.25 m/s |
| `..._full_power_low_bandwidth.cfg` | 480 MHz | +12 dBm | 0.502 m | 115.7 m | 2.42 m/s |

The `full_power_low_bandwidth` and `low_power_full_bandwidth` variants
exist to meet regional regulatory limits on bandwidth or transmit power
(e.g. FCC in the USA — see "Optimizing for FCC Restrictions" in the TI
user guide). Bandwidth = frequency slope (8th `profileCfg` value, MHz/µs)
× ramp end time (5th value, µs); TX power = 12 dBm − backoff (6th value,
one hex byte per transmitter).

### Parameters you may tune

The `.cfg` files contain three command groups:

- **Standard mmWave SDK commands** (`profileCfg`, `frameCfg`, `cfarCfg`,
  `clutterRemoval`, …) — documented in the mmWave SDK 3.5 user guide.
  Note that `cfarCfg`, `multiObjBeamForming`, `clutterRemoval`,
  `aoaFovCfg` and `cfarFovCfg` affect **only the dynamic chain**, never
  the static detection.
- **Tracking commands** — documented in the TI *3D People Tracking Tracker
  Layer Tuning Guide*:
  - `boundaryBox x_min x_max y_min y_max z_min z_max` (m): tracker
    operating volume; points outside are ignored (association code 254).
  - `staticBoundaryBox …` (m): volume where static (stopped) tracks are
    retained.
  - `allocationParam`, `gatingParam`, `stateParam`, `maxAcceleration`,
    `trackingCfg`: track creation thresholds, gating, aging and limits.
- **Static detection commands** — documented in the TI *Area Scanner
  Static Detection CLI Commands* page:
  - `heatmapGenCfg <subFrameIdx> <recordingMode> <phaseRotDeg>
    <minRangeBin> <maxRangeBin> <maxAngleDeg> <angleStepDeg>
    <rangeBinForNoiseLevelCalc>` — heatmap span. Detectable range bins are
    `minRangeBin + 1` to `maxRangeBin − 1`; the angle grid is
    `−maxAngleDeg : angleStepDeg : maxAngleDeg` (suggested step 1–3°).
  - `staticDetectionCfg <subFrameIdx> <numAngleBinToSum> <minAziAngleDeg>
    <maxAziAngleDeg> <minEleAngleDeg> <maxEleAngleDeg> <localPeakTH>
    <heatmapDiffTH> <significantTH> <eAngleBinDiffTH>
    <heatmapDiffToNoiseTH>` — detection thresholds (all linear scale):
    `localPeakTH` suggested 0.3–1.0 (lower → more points),
    `heatmapDiffTH` suggested 4–6.3 (the most important threshold; lower →
    more points), `significantTH` suggested 0.1–0.4 (higher → fewer false
    detections).

## 4. Running the client

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
```

Edit [`config_radar.json`](config_radar.json) (serial ports and chirp
file), or use the CLI overrides:

```bash
# Windows, USB (two virtual COM ports; check the Device Manager)
urad-area-scanner --config config_radar.json --control-port COM8 --data-port COM7

# With the live GUI and custom zones (critical 0-1.5 m, warning 1.5-4 m)
urad-area-scanner -c config_radar.json --gui --critical-zone 0 1.5 --warning-zone 1.5 4

# Raspberry Pi single-UART adapter, resetting the radar via GPIO pin 18
urad-area-scanner -c config_radar.json --single-port /dev/ttyS0 --gpio-reset-pin 18

# Unattended capture: 500 frames, no files
urad-area-scanner -c config_radar.json --max-frames 500 --no-save
```

Full CLI reference:

| Flag | Meaning |
|---|---|
| `-c`, `--config` | JSON configuration file (required) |
| `--control-port` | Override the control serial port (115200 baud) |
| `--data-port` | Override the data serial port (921600 baud) |
| `--single-port PORT` | One shared UART for control and data (Raspberry Pi adapter) |
| `--chirp` | Override the chirp configuration file path |
| `--critical-zone START END` | Critical zone radial range in meters (default `0 2`) |
| `--warning-zone START END` | Warning zone radial range in meters (default `2 4`) |
| `--projection-time SECONDS` | Look-ahead for the warning projection (default `2.0`) |
| `--output-dir` | Directory for the output files (default `./output`) |
| `--no-save` | Disable all file output |
| `--gui` | Live top view (requires `pip install urad-mmwave[gui]`) |
| `--duration SECONDS` | Stop after this many seconds |
| `--max-frames N` | Stop after this many frames |
| `--gpio-reset-pin PIN` | Reset the radar through a GPIO pin before configuring (requires `pip install urad-mmwave[rpi]`) |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

> **⚠️ IMPORTANT — one configuration per boot:** TI toolbox firmwares
> accept only **one** configuration per boot. Before running the client
> again, reset the radar: unplug and replug the USB cable, press the reset
> button, or drive the RESET pin (on Raspberry Pi the client does it for
> you with `--gpio-reset-pin` or `gpio_reset_pin` in the JSON). Resetting
> also restarts the 15-frame static calibration — keep the scene clear.

## 5. GUI

`--gui` opens a live top view (X-Y, meters, sensor at the origin):

- **Dynamic points** — filled dots, marker size proportional to SNR.
- **Static points** — magenta squares (newly added static objects).
- **Tracked objects** — numbered circles, one stable color per track id,
  with a **projection line** from the current to the projected position;
  the line is red when the projected position falls in the critical zone,
  orange in the warning zone, green otherwise (longer line = faster
  object).
- **Occupancy zones** — dashed half-circle arcs at the critical (red) and
  warning (orange) radii.
- **Tracker boxes** — `boundaryBox` (gray) and `staticBoundaryBox` (blue)
  read from the chirp configuration file.

The window title shows the live counts and the zone status
(`CRITICAL` / `warning` / `clear`).

## 6. Output

Console, one line per frame:

```
dynamic: 12  static: 3  tracks: 2  zone: CRITICAL
```

Unless `--no-save` is given, four text files are appended in the output
directory. Each line holds one frame (frames with no data are skipped) and
ends with the host epoch timestamp in seconds:

| File | Per-item fields (repeated) |
|---|---|
| `DynamicPoints.txt` | range (m), azimuth (deg), elevation (deg), doppler (m/s), snr, noise |
| `StaticPoints.txt` | x (m), y (m), z (m), doppler (m/s), snr, noise |
| `Targets.txt` | tid, x, y, z (m), vx, vy, vz (m/s), ax, ay, az (m/s²) |
| `TargetsIndex.txt` | track id per dynamic point (253 = weak SNR, 254 = outside boundaryBox, 255 = noise) |

### UART/TLV protocol summary (for integrators)

Data UART at 921600 baud, 8N1. Each frame packet is padded with zero bytes
to a multiple of 32 bytes and starts with the magic word
`01 02 03 04 05 06 07 08`.

Frame header — 44 bytes, little-endian (`<Q9I`), the out-of-box header
plus one extra word:

| Field | Type |
|---|---|
| magic word | uint64 |
| version, totalPacketLen, platform, frameNumber, timeCpuCycles, numDetectedObj, numTLVs, subFrameNumber | 8 × uint32 |
| **numStaticDetectedObj** | uint32 |

TLVs (8-byte header: type uint32, length uint32; length excludes the
header):

| Type | Content | Item layout (little-endian) |
|---|---|---|
| 1 | Dynamic point cloud | `4f`: range (m), azimuth (rad), elevation (rad), doppler (m/s) — spherical |
| 7 | Dynamic side info | `2H`: snr, noise |
| 8 | Static point cloud | `4f`: x, y, z (m), doppler (m/s) — cartesian |
| 9 | Static side info | `2H`: snr, noise |
| 10 | Tracked object list | `I9f`: tid, posX, posY, velX, velY, accX, accY, posZ, velZ, accZ |
| 11 | Point-to-track association | `B` per dynamic point: track id, or 253/254/255 (see above) |

For programmatic use decode packets with
`urad_mmwave.apps.area_scanner.parse_frame` (see the module docstring):

```python
from urad_mmwave import RadarSession, load_config
from urad_mmwave.apps.area_scanner import HEADER_FORMAT, parse_frame

config = load_config("config_radar.json")
config.packet.header_format = HEADER_FORMAT  # 44-byte area scanner header
with RadarSession(config) as session:
    for fields, payload, timestamp in session.packets():
        frame = parse_frame(payload, timestamp, num_tlvs=fields[6])
        print(len(frame.targets), "tracked,", len(frame.static_points), "static")
```

## 7. Troubleshooting

- **No detections at all** — check the data port and baud rate (921600);
  verify the radar was reset before this run (one configuration per boot);
  run with `-v` to see the configuration command responses.
- **The radar does not answer configuration commands** — wrong control
  port (on Windows the control port is usually the *enhanced*/lower COM
  number of the XDS110/CP210x pair, the data port the *standard* one), or
  the sensor is still running from a previous configuration: power-cycle.
- **No static points** — the object was already present during the
  15-frame calibration (only *newly added* objects are detected: remove
  the object, reset, reconfigure with a clear scene, then add it); or the
  object is outside the `heatmapGenCfg` range/angle span; or the
  `staticDetectionCfg` thresholds are too high (lower `heatmapDiffTH`
  towards 4.0 first).
- **Too many false static points** — raise `significantTH` (up to ~0.4)
  or `heatmapDiffTH` (up to ~6.3); make sure the sensor is rigidly mounted
  (vibration invalidates the calibration).
- **Moving objects not tracked** — they may be outside the `boundaryBox`
  volume (association code 254 in `TargetsIndex.txt`), or the allocation
  thresholds (`allocationParam`) are too strict for the target size.
- **LED meanings** — steady power LED on power; if the firmware boots
  correctly the data stream starts only after a valid configuration is
  sent.

Links:

- TI Area Scanner user guide and Static Detection CLI commands — TI Radar
  Toolbox 4.00.00.05, `source/ti/examples/Industrial_and_Personal_Electronics/Area_Scanner/docs/`
  (also in the [TI Resource Explorer](https://dev.ti.com)).
- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX)
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core)
  (Python client source)

## 8. Credits

Based on the **Area Scanner** example of the TI Radar Toolbox
**4.00.00.05** (© Texas Instruments). The firmware binary is redistributed
with TI authorization for use with TI devices, subject to the applicable
TI license terms. The uRAD Python client code is MIT licensed
([urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core)).
