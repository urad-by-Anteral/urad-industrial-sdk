# 3D People Tracking

People detection, counting and tracking for the uRAD Industrial (IWR6843AoP),
using the TI 3D People Tracking firmware (formerly named "People Counting";
renamed by TI in the Radar Toolbox). Two mounting variants share this
application — only the firmware binary and chirp configuration differ:

| Variant | Firmware ([Releases](../../../../releases)) | Chirp configuration | Mounting |
|---|---|---|---|
| Standard (wall) | `3D_people_track_6843_demo.bin` | [`chirp_config/standard/`](chirp_config/standard) | Wall / tripod, 1.5–2.5 m high, ~10–15° downtilt |
| Overhead (ceiling) | `overhead_3d_people_track_demo_default.bin` | [`chirp_config/overhead/`](chirp_config/overhead) | Ceiling, 2.8–3 m high, facing straight down |

Typical use cases: occupancy counting for building automation and smart
lighting, retail footfall analytics, presence detection for HVAC/energy
saving, safety zone monitoring around machinery, and tracking people even
after they sit or lie down and remain stationary.

## How it works

The full processing chain runs on the radar chip; the host only receives
results over UART. Two layers run per frame:

1. **Detection layer** (DSP): range FFT, static clutter handling, CFAR
   detection in range/azimuth/elevation and Doppler estimation produce a 3D
   **point cloud** — each point has range, azimuth, elevation, radial
   velocity (Doppler) and SNR.
2. **Tracker layer** (group tracker, Arm core): an Extended Kalman Filter
   with a 3D constant-acceleration motion model groups the point cloud into
   **tracks** (one per person). Each frame it runs: point-cloud tagging
   (points outside the configured boundaries are discarded) → predict →
   associate (gating with Mahalanobis distance) → allocate (clustering of
   unassociated points into new track candidates) → update → presence
   detection → track maintenance (DETECT / ACTIVE / FREE state machine).

The output stream contains the compressed point cloud, the target list
(position/velocity/acceleration per person), per-point track association,
per-track height estimates and, when configured, an early presence flag.

For the algorithm internals see the TI implementation guides listed in
[Documentation](#documentation-and-links).

## Required firmware

Flash **one** of the two binaries (from this repository's
[Releases](../../../../releases)) with TI UniFlash:

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in **flashing mode** with the DIP/SOP switches — see
   chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
   and [`firmware/README.md`](../../firmware/README.md).
3. Load the `.bin` as *meta image 1* and flash.
4. Restore the **functional mode** switch position and power-cycle.

The same standard binary serves all standard chirp configurations; the same
overhead binary serves all overhead configurations. Never mix a standard
chirp configuration with the overhead firmware or vice versa.

## Physical setup and calibration

### Standard (wall) variant

- Height: **1.5–2.5 m**, above the heads of the people to be tracked.
- Downtilt: **~10–15°**. Too much downtilt increases ground clutter and
  shrinks the covered area; no downtilt degrades tracking when one person
  walks in line behind another.
- Aim the 120° antenna field of view at the region where people enter the
  scene.

### Overhead (ceiling) variant

- Height: **2.8–3 m**, sensor facing the ground, directly above the area of
  interest (covered area ≈ 3 m radial distance from the sensor).
  Mounting lower increases ground clutter; mounting higher lowers signal
  quality — both degrade tracking.
- If a tripod with an extension arm is used, project the board at least
  30 cm away from the metallic stem to avoid boresight reflections.

### Sensor geometry in the chirp configuration

After mounting, make the `sensorPosition` command in the chosen `.cfg` match
the real geometry (values in the World frame, origin on the floor below the
sensor):

```
sensorPosition <height m> <azimuthTilt deg> <elevationTilt deg>
```

Standard configs ship with `sensorPosition 2 0 15` (2 m high, 15° downtilt);
overhead configs with `sensorPosition 2.9 0 90` (2.9 m high, pointing down).
Also adjust the boundary boxes (next section) to the room dimensions.

### Antenna calibration (recommended for overhead)

For best performance TI recommends running the out-of-box demo *Range Bias
and Rx Channel Gain/Phase Measurement and Compensation* procedure once per
board (with a corner reflector) and replacing the default
`compRangeBiasAndRxChanPhase 0 1 0 1 ...` line in the chirp configuration
with the measured coefficients. This is optional for a first evaluation but
noticeably improves detection and tracking accuracy, especially in the
overhead variant. The procedure is described in the TI mmWave SDK
out-of-box demo documentation (Calibration section).

## Chirp configurations provided

| File | Variant | Use |
|---|---|---|
| [`standard/AOP_6m_default.cfg`](chirp_config/standard/AOP_6m_default.cfg) | Standard | Default, ranges up to ~6 m |
| [`standard/AOP_6m_staticRetention.cfg`](chirp_config/standard/AOP_6m_staticRetention.cfg) | Standard | Keeps stationary people tracked much longer (larger static thresholds) |
| [`standard/AOP_9m_default.cfg`](chirp_config/standard/AOP_9m_default.cfg) | Standard | Extended range up to ~9 m |
| [`standard/AOP_9m_sensitive.cfg`](chirp_config/standard/AOP_9m_sensitive.cfg) | Standard | 9 m with higher detection sensitivity (lower thresholds; more prone to false detections) |
| [`overhead/pt_6843_3d_aop_overhead_3m_radial.cfg`](chirp_config/overhead/pt_6843_3d_aop_overhead_3m_radial.cfg) | Overhead | Default, 3 m radial coverage |
| [`overhead/pt_6843_3d_aop_overhead_3m_radial_low_bw.cfg`](chirp_config/overhead/pt_6843_3d_aop_overhead_3m_radial_low_bw.cfg) | Overhead | <500 MHz RF bandwidth (regulatory-restricted regions; lower range resolution). TI pairs this with a dedicated low-bandwidth firmware build — ask Anteral if you need it |
| [`overhead/pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg`](chirp_config/overhead/pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg) | Overhead | Static retention for the overhead variant |

Typical chirp parameters of the standard 6 m configuration: start frequency
60.6 GHz, bandwidth ≈ 2.25 GHz, range resolution 8.4 cm, maximum range
7.2 m, maximum radial velocity 8.38 m/s, velocity resolution 0.17 m/s,
frame period 50–55 ms, 3 Tx × 4 Rx.

### Tracker parameters and tuning

Every `.cfg` contains the tracker layer commands below (values in meters,
World coordinates, floor at Z = 0, sensor at X = 0, Y = 0). These are the
parameters to tune for your room; the detection layer parameters
(`dynamicRACfarCfg`, `staticRACfarCfg`, …) rarely need changes.

| Command | Parameters | Meaning |
|---|---|---|
| `boundaryBox` | Xmin Xmax Ymin Ymax Zmin Zmax | Volume where tracks may exist. Points outside are ignored — set it to the room walls to kill multipath ghosts. |
| `staticBoundaryBox` | Xmin Xmax Ymin Ymax Zmin Zmax | Zone where people are allowed to become static and stay tracked. Keep it smaller than `boundaryBox` so exits are detected quickly. For wall mount keep Ymin ≥ 2 m. |
| `presenceBoundaryBox` | Xmin Xmax Ymin Ymax Zmin Zmax | Zone of the early presence detector (drives the presence TLV). May be larger than `boundaryBox`. |
| `sensorPosition` | height azimuthTilt elevationTilt | Real mounting geometry (m, deg, deg). Wall: e.g. `2 0 15`; overhead: `2.9 0 90`. |
| `gatingParam` | gain width depth height velocity | Gate around each track for point association. Gain ≈ 3 ("three-sigma"); limits (m, m, m, m/s) ≈ person size and agility. Too small → one person splits into several tracks; too large → several people merge into one. |
| `allocationParam` | snrThre snrThreObscured velocityThre pointsThre maxDistanceThre maxVelThre | Criteria for creating a **new** track from unassociated points: minimum total SNR (and when occluded by another track), minimum centroid radial velocity (m/s), minimum number of points, maximum squared distance (m²) and velocity difference (m/s) to join the cluster. Lower thresholds detect people earlier/farther but increase false tracks. |
| `stateParam` | det2act det2free active2free static2free exit2free sleep2free | Frame counters of the track state machine: HITs to confirm a track, MISSes to drop it (normal / static-in-zone / exiting / maximum static lifespan). Static retention configs raise `static2free`/`sleep2free`. |
| `maxAcceleration` | X Y Z | Allowed acceleration change (m/s²) between frames. Larger = trust the motion model less (helps with abrupt turns). |
| `trackingCfg` | enable configIdx maxPoints maxTracks maxRadialVel(0.1 m/s) velResolution(mm/s) frameTime(ms) [boresightFilter] | Tracker enable and memory/velocity setup; must match the chirp (frame period, max/res velocity). The final flag enables boresight filtering (overhead only). |

Quick tuning recipes (from the TI tracker layer tuning guide):

- **Too few people tracked** → lower `allocationParam` snrThre /
  pointsThre / velocityThre; check `boundaryBox` covers the ranges.
- **Ghost/false tracks** → tighten `boundaryBox` to the physical room;
  raise snrThre and pointsThre; raise `det2actThre`; lower the `*2free`
  thresholds.
- **Two people merge into one track** → lower `gatingParam` limits and
  `allocationParam` maxDistanceThre / maxVelThre.
- **A person who stops moving is lost** → confirm they are inside
  `staticBoundaryBox`; raise `static2freeThre` / `sleep2freeThre` (or use
  the `staticRetention` configuration).
- **A moving person is lost** → raise `maxAcceleration` and/or the
  `gatingParam` limits.

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
urad-people-tracking --config config_radar.json --control-port COM8 --data-port COM7

# Overhead variant: select the matching chirp configuration
urad-people-tracking --config config_radar.json --chirp chirp_config/overhead/pt_6843_3d_aop_overhead_3m_radial.cfg

# Live GUI (requires: pip install urad-mmwave[gui])
urad-people-tracking --config config_radar.json --gui

# Raspberry Pi (single UART + GPIO reset — see below)
urad-people-tracking --config config_radar_rpi.json
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
| `--gui` | Live top view (requires the `gui` extra) |
| `--duration` SECONDS | Stop after this many seconds (default: run until Ctrl+C; ignored with `--gui`) |
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

`--gui` opens a live 2D top view (X/Y in meters, sensor at the origin):

- **Point cloud**: gray dots; the marker size encodes the point SNR.
- **Tracked people**: numbered circles, one stable color per track id. The
  number is the track id reported in the output data.
- **Tracker zones** read from the chirp configuration: `boundaryBox` (gray
  dashed), `staticBoundaryBox` (blue dashed), `presenceBoundaryBox` (green
  dotted).
- The window title shows the live count of tracks and points, and the
  presence state when the firmware reports it.

The console output and file writing remain active while the GUI runs; close
the window to stop.

## Output

Per frame the client prints one console line:

```
targets: 2  points: 87  presence: 1
```

(`presence` appears only when the firmware sends the presence TLV, i.e.
when the configuration defines a `presenceBoundaryBox`: 1 = presence
detected inside the presence zone, 0 = empty.)

Unless `--no-save` is given, four append-only text files are written to
`--output-dir` (one line per frame — frames with no data for a given file
write no line; every line ends with the host epoch timestamp in seconds):

| File | Per-frame content |
|---|---|
| `PointCloud.txt` | For each point: range (m), azimuth (deg), elevation (deg), Doppler (m/s, signed), SNR — then timestamp |
| `Targets.txt` | For each track: tid, posX, posY, posZ (m), velX, velY, velZ (m/s), accX, accY, accZ (m/s²) — then timestamp |
| `TargetsIndex.txt` | Track id per point of the **previous** frame's cloud (253 = weak SNR, 254 = outside boundary, 255 = noise) — then timestamp |
| `TargetsHeight.txt` | For each track: tid, maxZ, minZ (m) — then timestamp |

For programmatic use, decode packets with
`urad_mmwave.apps.people_tracking.parse_frame` (see the module docstring).

### UART/TLV protocol (for integrators)

Little-endian. Each packet starts with the 40-byte out-of-box frame header
(magic word `02 01 04 03 06 05 08 07`, version, total length, platform,
frame number, CPU time, number of detected points, number of TLVs, subframe
number — all uint32 after the 8-byte magic word), followed by the TLVs.
Packets are padded to a 32-byte multiple with `0xBE` bytes.

| TLV type | Content | Payload layout |
|---|---|---|
| 1020 | Compressed point cloud | 5 × float32 decompression units (elevation, azimuth, doppler, range, snr), then per point: elevation int8, azimuth int8, **doppler int16 (signed)**, range uint16, snr uint16 (8 bytes/point). Multiply each field by its unit: angles in rad, doppler in m/s, range in m. |
| 1010 | Target list | Per target (112 bytes): tid uint32, posX/Y/Z, velX/Y/Z, accX/Y/Z (9 × float32), error covariance 16 × float32, gating gain float32, confidence float32 |
| 1011 | Target index | uint8 per point of the *previous* frame (values 0–249 = track id; 253/254/255 = not associated) |
| 1012 | Target height | Per target (12 bytes): tid uint8, 3 padding bytes, maxZ float32, minZ float32 |
| 1021 | Presence indication | uint32: 1 = presence in the presence boundary box, 0 = none |

Note on Doppler sign: the point cloud Doppler is a **signed** int16 (the
legacy uRAD scripts decoded it as unsigned; this client follows the TI
implementation guide). The sign distinguishes approaching from receding
motion.

## Troubleshooting

- **No detections / client hangs after sending the configuration**: the
  radar was already configured this boot — reset it (one configuration per
  boot, see above). Also confirm the firmware variant matches the chirp
  configuration (standard vs overhead).
- **`Serial port not found` / wrong COM port**: in Windows Device Manager
  look for two *Silicon Labs CP210x* ports — *Enhanced* is the control
  port (115200), *Standard* is the data port (921600). On Linux they
  enumerate as `/dev/ttyUSB0` (control) and `/dev/ttyUSB1` (data).
- **Garbage or no data on the data port**: check the data baud rate is
  921600 in the JSON config, and that the board is in functional mode (not
  flashing mode).
- **Tracks appear outside the room, or people are missed**: tune the
  tracker parameters (see [tuning](#tracker-parameters-and-tuning)) and
  verify `sensorPosition` matches the real mounting height and tilt.
- **Poor accuracy in overhead mode**: run the antenna calibration and check
  the mounting height is within 2.8–3 m.

## Documentation and links

- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX) (browsable in
  the [TI Resource Explorer](https://dev.ti.com)) → *Industrial and
  Personal Electronics → People Tracking*: the *3D People Tracking* and
  *Overhead 3D People Tracking* user guides, plus the *Detection Layer* and
  *Tracker Layer* tuning guides and the implementation guide referenced
  above.
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf) (flashing:
  chapter 3) and [Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) —
  Python client source, configuration reference and general
  troubleshooting.

## Credits

Based on the **3D People Tracking** and **Overhead 3D People Tracking**
examples of the TI Radar Toolbox **4.00.00.05**. The firmware binaries are
redistributed with Texas Instruments' authorization and remain subject to
the applicable TI license terms. The uRAD client code (urad-mmwave) is
released under the MIT License by Anteral.
