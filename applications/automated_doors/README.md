# Automated Doors and Gates

Smart door/gate triggering for the uRAD Industrial (IWR6843AoP), using the
TI Automated Doors firmware. Where a traditional door sensor reacts to mere
proximity, this application tracks the **range, speed and direction** of
people and opens the door only for someone actually walking towards it — a
person walking past or away never triggers it, and a fast walker triggers it
earlier. A second algorithm detects **newly added static objects** (a cart,
a box, a pallet left in the doorway) so the controller can refuse to close
the door while the doorway is obstructed.

Typical use cases: automatic doors and gates in buildings, supermarkets and
hospitals (fewer false openings → energy savings); industrial roller doors
and barriers; safety interlocks that keep a door from closing on an
obstruction; footfall-aware entrance automation.

| Item | Value |
|---|---|
| Firmware | `automated_doors_68xx_demo_aop.bin` ([Releases](../../../../releases)) |
| Chirp configuration | [`chirp_config/profile_3d_automated_doors_AOP.cfg`](chirp_config/profile_3d_automated_doors_AOP.cfg) |
| Client | `urad-automated-doors` (part of [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core)) |
| Mounting | 2–3 m high, ~70° downtilt (AoP antenna), aimed at the approach area |

## How it works

The full processing chain runs on the radar chip; the host receives results
over UART. Three stages run per frame:

1. **Dynamic detection** (DSP): range FFT, static clutter removal, CFAR
   detection and angle estimation produce a 3D point cloud of *moving*
   reflectors (range, azimuth, elevation, radial velocity, SNR).
2. **Static object detection** (DSP): during the first frames after
   `sensorStart` the firmware records the range-angle heatmap of the empty
   scene (background calibration). Afterwards, each frame's heatmap is
   compared against the recording; peaks that exceed the recorded background
   become **newly added static points** (x, y, z cartesian). Objects already
   present during calibration are part of the background and are never
   reported — clear the doorway before starting the sensor.
3. **Tracker** (group tracker GTRACK, Arm core): an Extended Kalman Filter
   groups the dynamic point cloud into tracks, one per person/moving object,
   each with position, velocity and acceleration.

The **door decision** itself is not part of the UART stream. On the device
it drives GPIO2 of the EVM (`MmwDemo_setDoorState` in the firmware), and
TI's MATLAB visualizer recomputes the same logic host-side. The uRAD client
reimplements it faithfully:

- A track **activates** the door when it is inside the approach zone
  (|x| < 2.0 m, 0 < y < 3.5 m by default), moving towards the door plane
  (y = 0), and would reach it within **3 seconds** at its current velocity
  (`time = y / velY`).
- Any activation re-arms a **hold counter** (5 frames by default, i.e.
  0.5 s at the 100 ms frame period), so the door stays open briefly after
  the last activation instead of flickering.
- A **static obstruction** is flagged while any newly added static point
  lies within **1.5 m** of the sensor (the state that turns the TI
  visualizer's door yellow). Static objects beyond ~3.5 m are outside the
  configured heatmap range and are not reported at all.

All coordinates are in the **sensor frame**: the firmware GPIO logic (and
therefore this client) does not rotate them by the mounting tilt. The TI
MATLAB visualizer rotates only for display. With the recommended 70°
downtilt the defaults reproduce TI's demo behavior; tune the zone with the
CLI flags below if your geometry differs.

## Required firmware

Flash `automated_doors_68xx_demo_aop.bin` ([Releases](../../../../releases))
(the IWR6843**AoP** build of the TI demo; the ISK/ODS builds do not match
this board's antenna) with TI UniFlash:

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in **flashing mode** with the DIP/SOP switches — see
   chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
   and [`firmware/README.md`](../../firmware/README.md).
3. Load the `.bin` as *meta image 1* and flash.
4. Restore the **functional mode** switch position and power-cycle.

Requires an ES2.0 device — every uRAD Industrial ships with ES2.0 silicon.

## Physical setup

- Height: **2–3 m**, above the top of the objects/people to be tracked.
- Downtilt: **~70°** for the AoP antenna (per the TI user guide), so the
  antenna beam covers the area directly in front of and under the door.
- Expected coverage at 2 m height / 70° downtilt: 0–4 m detection range,
  ±2 m width at the minimum range; position accuracy ≈ 0.1 m, azimuth
  accuracy ≈ 8°.
- Start the sensor with the doorway **clear**: the static detection records
  the background during the first frames, and anything present then becomes
  invisible to the obstruction logic.

## Chirp configuration provided

[`chirp_config/profile_3d_automated_doors_AOP.cfg`](chirp_config/profile_3d_automated_doors_AOP.cfg)
is TI's AoP profile for this demo (the ISK/ODS variants are not included —
they pair with other antennas). Approximate characteristics: 60.5 GHz start
frequency, ≈ 2.1 GHz sweep used (range resolution ≈ 7 cm), maximum tracked
range limited to ≈ 11 m by `cfarFovCfg`, maximum radial velocity ± 2.04 m/s,
3 Tx × 4 Rx, 32 Doppler chirps, **100 ms frame period**.

Parameters worth tuning (from the TI CLI documentation — the file contains
standard SDK commands plus two application-specific groups):

### Tracker commands

| Command | Parameters | Meaning |
|---|---|---|
| `gatingParam` | gain width depth height velocity | Association gate around each track (m, m, m, m/s). Too small → one person splits into several tracks; too large → several people merge. |
| `allocationParam` | snrThre snrThreObscured velocityThre pointsThre maxDistanceThre maxVelThre | Criteria to create a **new** track from unassociated points. Lower thresholds detect people earlier but increase false tracks. |
| `stateParam` | det2act det2free active2free static2free exit2free | Frame counters of the track state machine (confirm/drop hysteresis). |
| `maxAcceleration` | X Y Z | Allowed acceleration (m/s²) between frames; raise it if fast direction changes lose the track. |
| `trackingCfg` | enable configIdx maxPoints maxTracks maxRadialVel(0.1 m/s) velResolution(mm/s) frameTime(ms) tilt(deg) | Tracker memory/velocity setup; must stay consistent with the chirp (frame period, velocity limits). |

### Static object detection commands

| Command | Key parameters | Meaning |
|---|---|---|
| `heatmapGenCfg` | subFrameIdx recordingMode phaseRotDeg **minRangeBin maxRangeBin** maxAngleDeg angleStepDeg noiseBin | Range-angle heatmap generation. `minRangeBin`/`maxRangeBin` (6 and 50 in this profile ≈ 0.5–3.5 m with the ~7 cm bins) bound where static objects can be detected. `angleStepDeg` 1–3° suggested. |
| `staticDetectionCfg` | subFrameIdx numAngleBinToSum minAzi maxAzi minEle maxEle **localPeakTH heatmapDiffTH significantTH** eAngleBinDiffTH heatmapDiffToNoiseTH | Detection thresholds against the recorded background. `heatmapDiffTH` (5.0 here, suggested 4–6.3) is the most important: lower it for more static points, raise it against false detections. `significantTH` 0.1–0.4 (higher = fewer false detections). Angles in degrees limit the detection field of view. |

The standard SDK commands (`cfarCfg`, `clutterRemoval`, `aoaFovCfg`, …)
apply only to the *dynamic* chain; they have no effect on the static object
detection.

The host-side door logic has its own knobs (`--door-halfwidth`,
`--door-depth`, `--open-time`, `--hold-frames`, `--obstruction-range`) — see
the CLI reference. They do not touch the radar; they change how the client
interprets the tracks and static points.

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
urad-automated-doors --config config_radar.json --control-port COM8 --data-port COM7

# Live GUI with the door indicator (requires: pip install urad-mmwave[gui])
urad-automated-doors --config config_radar.json --gui

# Narrower approach zone and a 2-second opening horizon
urad-automated-doors --config config_radar.json --door-halfwidth 1.0 --open-time 2.0

# Raspberry Pi (single UART + GPIO reset)
urad-automated-doors --config config_radar.json --single-port /dev/serial0 --gpio-reset-pin 17
```

### CLI reference

| Flag | Description |
|---|---|
| `-c`, `--config` PATH | **Required.** JSON configuration file (serial ports and chirp path) |
| `--control-port` PORT | Override the control serial port (config commands, 115200 baud) |
| `--data-port` PORT | Override the data serial port (TLV stream, 921600 baud) |
| `--single-port` PORT | One shared UART for control and data (Raspberry Pi adapter) |
| `--chirp` PATH | Override the chirp configuration file |
| `--door-halfwidth` METERS | Approach zone half width; tracks with \|x\| beyond it never trigger (default: 2.0) |
| `--door-depth` METERS | Approach zone depth in y (default: 3.5) |
| `--open-time` SECONDS | Open when an approaching track is within this many seconds of the door (default: 3.0) |
| `--hold-frames` N | Frames the door stays open after the last activation (default: 5) |
| `--obstruction-range` METERS | Static obstruction radius around the sensor (default: 1.5) |
| `--output-dir` DIR | Directory for the output text files (default: `./output`) |
| `--no-save` | Disable all file output |
| `--gui` | Live top view with the door indicator (requires the `gui` extra) |
| `--duration` SECONDS | Stop after this many seconds (ignored with `--gui`) |
| `--max-frames` N | Stop after this many frames (ignored with `--gui`) |
| `--gpio-reset-pin` PIN | Reset the radar through this GPIO pin before configuring (Raspberry Pi; `rpi` extra) |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

On Raspberry Pi (with the uRAD Raspberry Pi adapter) the control and data
streams share one UART: use `--single-port /dev/serial0` (or set both ports
in the JSON file) and optionally `--gpio-reset-pin <BCM pin>` so the client
resets the radar automatically before configuring it (requires
`pip install urad-mmwave[rpi]`). See the
[Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).

> **Important — one configuration per boot:** TI Radar Toolbox application
> firmwares accept only **one** chirp configuration per boot. Before running
> the client again, reset the radar: unplug and replug the USB cable, press
> the reset button, or drive the RESET pin on the board connector (on
> Raspberry Pi the client does this automatically via `--gpio-reset-pin`).
> A reset also restarts the static background calibration — keep the
> doorway clear for the first seconds after every start.

## GUI

`--gui` opens a live 2D top view (X/Y in meters, sensor at the origin):

- **Door indicator**: a filled bar at the sensor position labeled
  `DOOR OPEN` / `DOOR CLOSED` — **green** while the door trigger is active,
  **red** while closed, **yellow** while closed with a static obstruction.
- **Approach zone** (blue dashed rectangle): only tracks inside it can
  trigger the door.
- **Obstruction range** (orange dotted semicircle): static points inside it
  flag an obstruction.
- **Dynamic points**: gray dots, marker size encodes SNR.
- **Static points**: orange squares (newly added static objects).
- **Tracks**: numbered circles, one stable color per track id.
- The window title mirrors the door state, track count and point counts.

The console output and file writing remain active while the GUI runs; close
the window to stop.

## Output

Per frame the client prints one console line with the door state first:

```
door: CLOSED             tracks: 0  dynamic: 0  static: 0
door: OPEN               tracks: 1  dynamic: 42  static: 0
door: OPEN (obstructed)  tracks: 1  dynamic: 38  static: 6
door: OBSTRUCTED         tracks: 0  dynamic: 0  static: 6
```

States: `OPEN` (a track is approaching, or the hold window is running),
`CLOSED`, `OBSTRUCTED` (closed with a static object within the obstruction
radius), `OPEN (obstructed)` (both). For integrators the frame object
exposes `door_open: bool`, `obstructed: bool` and `activated_tids` — see
the module docstring of `urad_mmwave.apps.automated_doors`.

Unless `--no-save` is given, five append-only text files are written to
`--output-dir` (one line per frame — frames with no data for a given file
write no line; every line ends with the host epoch timestamp in seconds):

| File | Per-frame content |
|---|---|
| `DynamicPoints.txt` | For each moving point: range (m), azimuth (deg), elevation (deg), Doppler (m/s), SNR, noise (0.1 dB steps) — then timestamp |
| `StaticPoints.txt` | For each newly added static point: x, y, z (m), Doppler (m/s, ≈0), SNR, noise — then timestamp |
| `Targets.txt` | For each track: tid, posX, posY, posZ (m), velX, velY, velZ (m/s), accX, accY, accZ (m/s²) — then timestamp |
| `TargetsIndex.txt` | Track id per dynamic point — then timestamp |
| `DoorState.txt` | door_open (0/1), obstructed (0/1), number of activating tracks — then timestamp (written every frame) |

For programmatic use, decode packets with
`urad_mmwave.apps.automated_doors.parse_frame` and apply
`DoorStateMachine.update` (see the module docstring).

### UART/TLV protocol (for integrators)

Little-endian. Each packet starts with a **44-byte** frame header: the
40-byte out-of-box header (magic word `02 01 04 03 06 05 08 07`, version,
total length, platform `0xA6843`, frame number, CPU time, number of dynamic
points, number of TLVs, subframe number — all uint32 after the 8-byte magic
word) **plus a trailing uint32 with the number of static detected points**.
Packets are padded to a 32-byte multiple; the padding bytes are
*uninitialized memory* (not a fixed value), so integrators must use the
header's TLV count — never scan for a padding pattern.

With the provided configuration (`guiMonitor -1 1 0 0 0 0 0`) the firmware
sends TLVs 1, 7, 8, 9, 10 and 11; types 2/3/6 (profiles, statistics) and
4/5 (heatmaps) appear only with other `guiMonitor` settings. TLVs 1 and 8
are sent even when empty (zero length); 7, 9, 10 and 11 are omitted when
they would be empty.

| TLV type | Content | Payload layout |
|---|---|---|
| 1 | Dynamic point cloud | Per point (16 bytes): range (m), azimuth (**rad**), elevation (**rad**), Doppler (m/s, negative = approaching) — 4 × float32 |
| 2 | Range profile | uint16 per range bin (log magnitude) |
| 3 | Noise profile | uint16 per range bin |
| 6 | Statistics | 6 × uint32: inter-frame processing time, transmit time, processing margin, chirp margin, active-frame CPU load, inter-frame CPU load |
| 7 | Dynamic side info | Per point (4 bytes): SNR int16, noise int16 (0.1 dB steps) |
| 8 | Static point cloud | Per point (16 bytes): x, y, z (m), Doppler (m/s) — 4 × float32 cartesian |
| 9 | Static side info | Per point (4 bytes): SNR int16, noise int16 (0.1 dB steps) |
| 10 | Track list | Per track (40 bytes): tid uint32, then float32 × 9 in this order: **posX, posY, velX, velY, accX, accY, posZ, velZ, accZ** |
| 11 | Track index | uint8 per dynamic point: id of the track the point was associated to |

Note the track struct field order: the GTRACK 3D build appends the Z
components after the 2D fields — it is *not* pos/vel/acc grouped by vector.

The door decision is **not** in the stream; it is recomputed by the client
(as TI's visualizer does) and, on the device, drives **GPIO2** of the EVM
(high = open), which hardware integrators can wire to a door controller
directly.

## Troubleshooting

- **No packets / client hangs after sending the configuration**: the radar
  was already configured this boot — reset it (one configuration per boot,
  see above).
- **`Serial port not found` / wrong COM port**: in Windows Device Manager
  look for two *Silicon Labs CP210x* ports — *Enhanced* is the control port
  (115200), *Standard* is the data port (921600). On Linux they enumerate
  as `/dev/ttyUSB0` (control) and `/dev/ttyUSB1` (data).
- **Garbage or no data on the data port**: check the data baud rate is
  921600 in the JSON config, and that the board is in functional mode (not
  flashing mode).
- **Door opens for people walking past**: narrow the approach zone
  (`--door-halfwidth`) or shorten `--open-time`; check the sensor is
  mounted square to the walking direction.
- **Door opens late for fast walkers**: increase `--open-time`.
- **A known obstruction is not flagged**: the object was probably present
  during the background calibration — clear the doorway and reset the
  sensor; or the object is beyond the heatmap range (`maxRangeBin`); or
  lower `heatmapDiffTH` in the chirp configuration.
- **False static detections**: raise `heatmapDiffTH` / `significantTH` in
  `staticDetectionCfg`.
- **Tracks flicker or split**: tune `gatingParam` / `allocationParam` (see
  the tuning table above); raise `--hold-frames` to debounce the door.

## Documentation and links

- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX) (browsable in
  the [TI Resource Explorer](https://dev.ti.com)) → *Industrial and
  Personal Electronics → Automated Doors and Gates*: the *Automated Doors
  User Guide* and the *Static Detection CLI Commands* document this README
  summarizes.
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf) (flashing:
  chapter 3) and [Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) —
  Python client source, configuration reference and general
  troubleshooting.

## Credits

Based on the **Automated Doors and Gates** example of the TI Radar Toolbox
**4.00.00.05**. The firmware binary is redistributed with Texas
Instruments' authorization and remains subject to the applicable TI license
terms. The uRAD client code (urad-mmwave) is released under the MIT License
by Anteral.
