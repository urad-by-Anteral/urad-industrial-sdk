# CPD with Classification (in-cabin occupancy)

Vehicle Occupancy Detection (VOD) and Child Presence Detection (CPD) for
the uRAD Industrial (IWR6843AoP), using the TI *CPD with Classification*
firmware. The radar streams a 3D point cloud of the cabin; the client maps
it to configurable seat zones, runs an occupancy state machine per zone and
— optionally — classifies each occupied seat as **adult** or **child**.

Typical use cases: child presence detection after the driver leaves (CPD
regulations / Euro NCAP), seat occupancy for seat-belt reminders, occupancy
detection for up to 3 seat rows (cars, vans, minibuses), intruder detection
inside a parked vehicle, and occupancy of footwells or cargo areas through
custom zones. The radar sees through seat backs, blankets and low light —
scenarios where cameras fail.

> **⚠️ Antenna module compatibility (IWR6843AoP).** The TI example is
> written for the AWR6843 automotive in-cabin EVMs (xWR6843ISK-ODS and
> xWR6843AOP, both ES2.0). TI states that *"Both AWR6843 and IWR6843 are
> supported and can be used interchangeably"*, and the single prebuilt
> binary supports every antenna layout — the antenna geometry is selected
> by the `antGeometry0` / `antGeometry1` / `antPhaseRot` commands of the
> chirp configuration. Only the **AOP** configurations are shipped here,
> matching the IWR6843AoP of the uRAD Industrial. TI also warns that,
> because of the wider antenna field of view, detection performance on AOP
> modules may be slightly less optimal than on the ISK-ODS module. This
> combination (IWR6843AoP + AOP configuration) follows the TI documentation
> but has not yet been validated on uRAD Industrial hardware.

## How it works

The processing is split between the radar and the host:

**On the radar** (based on TI's Overhead 3D People Tracking detection
chain, modified for near-static occupants):

1. **Range processing** (hardware accelerator) builds the radar cube.
2. **Doppler binning**: chirps from a sliding window of 4 frames are
   Doppler-filtered (only the lowest Doppler bins are kept) to boost the
   SNR of nearly-static occupants — this is what enables 3-row coverage.
3. **Capon (MVDR) beamforming** generates a range-azimuth-elevation
   heatmap.
4. **Two-pass CASO CFAR** with separate "near" and "far" range search
   windows, plus angle- and range-dependent thresholds that suppress the
   static leakage around boresight and at close range.
5. **Zoom-in angle estimation** refines azimuth/elevation per detection.
6. The resulting **3D point cloud** (range, azimuth, elevation, SNR;
   Doppler is not computed and reported as 0) is sent over UART.

**On the host** (this Python client — in TI's demo, the MATLAB
visualizer):

1. The spherical points are transformed to **car coordinates** (X to the
   driver side, Y to the rear, Z up from the floor) using the mounting
   offsets/rotations of the `sensorPosition` command.
2. Each point is mapped to the **zones** defined as cuboids
   (`cuboidDef`) — typically three cuboids per seat: head/chest, lap and
   footwell.
3. An **occupancy state machine** per zone (`occStateMach`) decides
   empty/occupied from the number of points and their average SNR, with
   entry/stay/leave hysteresis, per-zone-type thresholds, a neighbor-SNR
   comparison that suppresses false detections on the middle seat
   (`zoneNeighDef`), and an overload freeze during large movements.
4. When enabled (`classParam`), the **classifier** accumulates the points
   of each occupied zone over N frames and compares the total SNR (dB) and
   the spatial variance ("volume") against per-zone thresholds:
   small + weak → **child**, otherwise → **adult**.

## Required firmware

Flash `occupancy_detection_3d_68xx.bin` ([Releases](../../../../releases))
with TI UniFlash:

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in **flashing mode** with the DIP/SOP switches — see
   chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
   and [`firmware/README.md`](../../firmware/README.md).
3. Load the `.bin` as *meta image 1* and flash.
4. Restore the **functional mode** switch position and power-cycle.

The same binary serves **all** the chirp configurations of this
application (the antenna geometry travels in the configuration file, not in
the firmware).

## Physical setup and calibration

- **Overhead mounting** (all `overhead` configurations): sensor on the
  cabin ceiling, facing the floor, typically between the two seat rows.
  The shipped configurations assume the geometry encoded in their
  `sensorPosition` command — e.g. `sensorPosition 0 1.2 1.1 90 0 0` means
  the sensor is at X = 0 m, Y = 1.2 m from the origin (brake pedal),
  Z = 1.1 m above the floor, rotated 90° in the Y-Z plane (facing down).
  **Update this command to match your actual mounting**, otherwise the
  points will not fall inside the zones.
- **Zone geometry**: adapt the `cuboidDef` volumes to your cabin (values
  in meters, car coordinates, floor at Z = 0). Leave gaps between zones;
  do not overlap the cuboids of different zones. To disable a zone, make
  it a NULL zone (a single all-zero cuboid).
- **Antenna calibration (recommended)**: run the out-of-box demo *Range
  Bias and Rx Channel Gain/Phase Measurement and Compensation* procedure
  once per board (with a corner reflector) and replace the
  `compRangeBiasAndRxChanPhase` line of the chirp configuration with the
  measured coefficients. The shipped files carry TI's coefficients for
  their EVM — per-board calibration noticeably improves detection quality.

## Chirp configurations provided

All files are the AOP variants of the TI demo (60 GHz, 3 Tx × 4 Rx,
frame period 200 ms → 5 frames/s output):

| File | Zones | Rows | Classification | Use |
|---|---|---|---|---|
| [`vod_6843_aop_overhead_2row.cfg`](chirp_config/vod_6843_aop_overhead_2row.cfg) | 5 | 2 | off | Default 5-seat car, occupancy only |
| [`vod_6843_aop_overhead_2row_classification.cfg`](chirp_config/vod_6843_aop_overhead_2row_classification.cfg) | 5 | 2 | **on** | 5-seat car with adult/child classification |
| [`vod_6843_aop_overhead_2row_intruder.cfg`](chirp_config/vod_6843_aop_overhead_2row_intruder.cfg) | 7 | 2 | off | 5 seats + 2 intruder-warning zones |
| [`vod_6843_aop_overhead_2row_van.cfg`](chirp_config/vod_6843_aop_overhead_2row_van.cfg) | 5 | 2 | off | Van geometry (higher ceiling: sensor at 1.2 m) |
| [`vod_6843_aop_overhead_3row_bus.cfg`](chirp_config/vod_6843_aop_overhead_3row_bus.cfg) | 8 | 3 | off | 3-row / 6+ seat bus or minivan (raised thresholds) |

TI also ships ISK-ODS and ISK (front-mount) variants of these files; they
are **not** included because their antenna geometry does not match the
IWR6843AoP.

### Parameters you may tune

Radar-side commands (take effect on the chip):

| Command | What to tune | Notes |
|---|---|---|
| `dynamicRACfarCfg` | `K0` cross-range / cross-angle thresholds (params 12–13, dB-like linear factors, default 8.0 / 6.0) and `refRangeBinIdx` (last param, default 15) | Lower K0 → richer point cloud but more false points. Lower `refRangeBinIdx` → richer points at close range. Keep at least 29 range bins processed (small skip sizes), or false detections appear (TI known issue). |
| `dynamic2DAngleCfg` | Linear SNR threshold for peak expansion (param 6) | Lower → more neighboring points per detection. |
| `dopplerBinSelCfg` | enable (param 2), FFT size, first/last kept bin | `-1 1 64 0 4` enables Doppler binning (recommended for 3 rows); `-1 0 64 0 4` disables it. |
| `sensorPosition` | x y z offsets (m) + yz/xy/xz clockwise rotations (deg) | Must match the real mounting. |
| `fovCfg`, `dynamicRangeAngleCfg` | — | **Keep at defaults**: the firmware hardcodes the angle-dependent CFAR ratio for the default 17×17 angle grid. |

Host-side commands (parsed by this client; the firmware CLI simply ignores
them):

| Command | Parameters | Meaning |
|---|---|---|
| `numZones` | n (5–8) | Number of zones; the first 5 map to the standard 5 seats. |
| `totNumRows` | n | Seat rows (2 or 3). |
| `cuboidDef` | zone cuboid xmin xmax ymin ymax zmin zmax | Up to 3 cuboids per zone, meters, car coordinates. All-zero = NULL zone. |
| `zoneNeighDef` | zone zoneType numNeigh neighbors… | Zone type selects the `occStateMach` set; declared neighbors must be beaten in average SNR for entry condition 2 (anti-false-detection for the middle seat). |
| `occStateMach` | zoneType p1…p10 | Per zone type: enter condition 1 (points, avgSNR), enter condition 2 (points, avgSNR), consecutive entry frames, stay condition (points, avgSNR), forget frames, forget points, overload avgSNR. Raise thresholds for zones close to the sensor, lower them for far zones. |
| `classParam` | mode numFrameAvg snrTh×zones volTh×zones | mode 1 enables classification; N frames accumulated per decision; per-zone thresholds: total SNR in dB (`10·log10`) and volume variance in m². Both below threshold → child. Needs re-tuning per mounting position. |
| `interiorBounds` | minX maxX minY maxY | Cabin footprint, used for display scaling only. |

## Running the client

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
```

Edit [`config_radar.json`](config_radar.json) (serial ports, chirp
configuration path) or use the CLI overrides, then:

```bash
# Windows, USB (control port 115200 baud, data port 921600 baud)
urad-cpd --config config_radar.json --control-port COM8 --data-port COM7

# Select another configuration variant
urad-cpd --config config_radar.json --chirp chirp_config/vod_6843_aop_overhead_3row_bus.cfg

# Live GUI (requires: pip install urad-mmwave[gui])
urad-cpd --config config_radar.json --gui

# Timed capture: 60 seconds or 300 frames, whichever comes first
urad-cpd --config config_radar.json --duration 60 --max-frames 300

# Raspberry Pi (single UART + GPIO reset — see below)
urad-cpd --config config_radar_rpi.json
```

### CLI reference

| Flag | Description |
|---|---|
| `-c`, `--config` PATH | **Required.** JSON configuration file (serial ports and chirp path) |
| `--control-port` PORT | Override the control serial port (config commands, 115200 baud) |
| `--data-port` PORT | Override the data serial port (TLV stream, 921600 baud) |
| `--chirp` PATH | Override the chirp configuration file |
| `--output-dir` DIR | Directory for the output text files (default: `./output`) |
| `--no-save` | Disable all file output |
| `--gui` | Live cabin view (requires the `gui` extra) |
| `--duration` SECONDS | Stop after this many seconds (default: run until Ctrl+C; ignored with `--gui`) |
| `--max-frames` N | Stop after N frames (default: run until Ctrl+C; ignored with `--gui`) |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

On Raspberry Pi (with the uRAD Raspberry Pi adapter) the control and data
streams share one UART: set both `control_serial.port` and
`data_serial.port` to `/dev/serial0` in the JSON file, and optionally add
`"gpio_reset_pin": <BCM pin>` so the client resets the radar automatically
before configuring it (requires `pip install urad-mmwave[rpi]`). See the
[Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).

> **Important — one configuration per boot:** TI Radar Toolbox application
> firmwares accept only **one** chirp configuration per boot. Before running
> the client again, reset the radar: unplug and replug the USB cable, press
> the reset button, or drive the RESET pin on the board connector (on
> Raspberry Pi the client does this automatically via `gpio_reset_pin`).

## GUI

`--gui` opens a live top view of the cabin (X = width in meters, driver
side positive and drawn on the left as in the TI visualizer; Y = depth,
rear positive):

- **Zones**: one rectangle per configured zone (the X/Y bounding box of
  its cuboids; NULL zones are not drawn). The fill color encodes the
  state: gray = **empty**, orange = **occupied** (no decision yet),
  blue = **adult**, red = **child**.
- **Zone labels**: zone id and state; while occupied also the point count
  and, once a classification decision is available, the metrics behind it
  (accumulated SNR in dB and volume variance in m²) so the thresholds can
  be tuned by eye.
- **Point cloud**: gray dots in car coordinates; the marker size encodes
  the point SNR.
- The window title shows the occupied-zone count, the number of points and
  the frame number.

The console output and file writing remain active while the GUI runs; close
the window to stop.

## Output

Per frame the client prints one console line:

```
Z1:adult Z2:empty Z3:empty Z4:empty Z5:child  points: 34  occupied: 2
```

Zone states are `empty`, `occupied` (occupied, no classification decision
yet — always the case when classification is disabled), `child` or
`adult`.

Unless `--no-save` is given, two append-only text files are written to
`--output-dir` (one line per frame — frames with no data write no line;
every line ends with the host epoch timestamp in seconds):

| File | Per-frame content |
|---|---|
| `PointCloud.txt` | For each point: x, y, z (m, car coordinates), SNR (linear firmware units) — then timestamp |
| `Zones.txt` | For each zone: zone id, occupied (0/1), number of points, average SNR, decision (0 = none, 1 = child, 2 = adult) — then timestamp |

For programmatic use, decode packets with
`urad_mmwave.apps.cpd.parse_frame` and run the host processing with
`urad_mmwave.apps.cpd.OccupancyTracker` (see the module docstring).

### UART/TLV protocol (for integrators)

Little-endian. This firmware does **not** use the standard out-of-box
header: each packet starts with a **48-byte** header —

| Field | Type | Notes |
|---|---|---|
| magic word | 8 bytes | `02 01 04 03 06 05 08 07` (same sync as the OOB demo) |
| version | uint32 | mmWave SDK version |
| totalPacketLen | uint32 | Bytes, including this header |
| platform | uint32 | `0xA6843` |
| frameNumber | uint32 | Frames 0–3 are never sent (the 4-frame heatmap sliding window must fill first) |
| subFrameNumber | uint32 | |
| chirpProcessingMargin | uint32 | Timing, 600 MHz clocks |
| frameProcessingTimeInUsec | uint32 | |
| trackingProcessingTimeInUsec | uint32 | |
| uartSendingTimeInUsec | uint32 | |
| numTLVs | uint16 | 0 (frame without detections) or 1 |
| checksum | uint16 | One's-complement 16-bit sum over the header |

followed by at most one TLV:

| TLV type | Content | Payload layout |
|---|---|---|
| 6 | Compressed point cloud | **The 32-bit length field includes the 8-byte TLV header itself** (unlike the people tracking firmwares). Body: 5 × float32 units (elevation rad, azimuth rad, doppler m/s, range m, snr linear — hardcoded to 0.01, 0.01, 0.00028, 0.00025, 0.04), then per point: elevation int8, azimuth int8, doppler int16, range uint16, snr uint16 (8 bytes/point, multiply by the units). Maximum 750 points. Doppler is always 0 in this demo. |

The zone states are **not** on the UART stream — they are computed by this
client from the point cloud and the chirp configuration parameters.

## Troubleshooting

- **No packets / client hangs after sending the configuration**: the radar
  was already configured this boot — reset it (one configuration per boot,
  see above). Note also that the first 4 frames after `sensorStart` are
  never transmitted (sliding window fill).
- **`Serial port not found` / wrong COM port**: in Windows Device Manager
  look for two *Silicon Labs CP210x* ports — *Enhanced* is the control
  port (115200), *Standard* is the data port (921600). On Linux they
  enumerate as `/dev/ttyUSB0` (control) and `/dev/ttyUSB1` (data).
- **Garbage or no data on the data port**: check the data baud rate is
  921600 in the JSON config, and that the board is in functional mode (not
  flashing mode).
- **Points appear but never inside the zones**: `sensorPosition` does not
  match the real mounting (offsets in meters, rotations in degrees), or
  the `cuboidDef` volumes do not match the cabin. Use `--gui` to see where
  the points fall.
- **Seats detected as occupied when empty (or vice versa)**: tune the
  `occStateMach` thresholds for your environment; check zone cuboids do
  not overlap; use `zoneNeighDef` neighbors against middle-seat false
  detections; run the antenna calibration.
- **Everything flagged occupied during door slams / lots of movement**:
  that is the overload freeze working as designed — the state machine
  holds the previous states while the average SNR exceeds the overload
  threshold.
- **Adults classified as children (or vice versa)**: retune the
  `classParam` per-zone SNR/volume thresholds for your mounting position —
  the GUI labels show the measured metrics next to the thresholds.

## Documentation and links

- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX) (browsable in
  the [TI Resource Explorer](https://dev.ti.com)) → *Automotive → In-cabin
  security and safety → AWR6843 CPD with Classification*: user guide and
  setup guide (mounting examples, antenna patterns).
- TI *3D People Tracking Demo Implementation Guide* (same toolbox) — the
  detection chain this firmware is based on.
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf) (flashing:
  chapter 3) and [Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) —
  Python client source, configuration reference and general
  troubleshooting.

## Credits

Based on the **AWR6843 CPD with Classification** example of the TI Radar
Toolbox **4.00.00.05** (firmware and chirp configurations, plus the host
processing ported from its MATLAB visualizer). The firmware binary is
redistributed with Texas Instruments' authorization and remains subject to
the applicable TI license terms. The uRAD client code (urad-mmwave) is
released under the MIT License by Anteral.
