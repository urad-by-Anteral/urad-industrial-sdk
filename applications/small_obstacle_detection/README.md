# Small Obstacle Detection

Detection of small, low obstacles near the ground for mobile robots, using
the uRAD Industrial (IWR6843AoP) and the TI Small Obstacle Detection
firmware (Radar Toolbox, Robotics family).

## 1. What it does

The radar is mounted low on a moving platform (robotic lawn mower, AGV,
delivery robot, cleaning robot) facing forward, and detects obstacles that
are hard to see for cameras and easy to miss for 2D lidars: rocks, fallen
fruit, toys, curbs, small animals such as hedgehogs. Two mechanisms work
together:

- **Tracking** — every obstacle that produces a consistent point cloud is
  tracked in 3D (position, velocity, acceleration), so the robot knows
  *where* the obstacle is and whether it is moving.
- **Zone occupancy alarms** — up to two configurable 3D zones (e.g. a far
  "slow down" zone and a near "stop" zone) are monitored by an occupancy
  state machine on the device. Each frame the firmware reports a simple
  occupied/clear flag per zone, debounced over several frames, that can
  drive the robot's safety logic directly — no host-side point cloud
  processing needed.

Because it is radar, detection works in darkness, rain, fog, dust and tall
grass, where optical sensors degrade.

Realistic use cases: obstacle avoidance for robotic mowers on lawns,
collision warning for warehouse AGVs, curb/step detection for delivery
robots, protective stop zones in front of slow-moving industrial vehicles.

## 2. How it works

The firmware is derived from the TI 3D People Tracking demo with three
additions aimed at small, mostly static objects (per the TI user guide):

1. **Enhanced static detection.** The detection layer runs both a dynamic
   and a static CFAR (`dynamicRACfarCfg` / `staticRACfarCfg`), so
   zero-Doppler returns from stationary obstacles are kept instead of
   being filtered out as clutter.
2. **Persistent point cloud.** Small obstacles yield sparse point clouds
   that are hard to distinguish from noise. The firmware overlays the
   point clouds of the last 3 frames before feeding the tracker: real
   objects reinforce (they appear every frame in the same place) while
   noise appears intermittently. This makes tracks on small objects far
   more stable.
3. **Occupancy state machine.** Each configured zone (`zoneDef`) counts
   the points inside it and their average SNR per frame. A zone becomes
   *occupied* after `frameEntryThreshold` consecutive frames with at least
   `pointsEntryThreshold` points of average SNR ≥ `snrEntryThreshold`
   (a tracked object inside the zone also occupies it immediately), and
   returns to *clear* only after `frameExitThreshold` frames below the
   maintain/exit thresholds — so the alarm neither flickers nor releases
   too early. The result is streamed every frame as a bit mask (TLV 1030).

Processing chain: range FFT → static + dynamic CFAR in the range-azimuth
domain → elevation estimation → persistent point cloud (3 frames) →
group tracker (gtrack, 3D) → zone occupancy state machine → UART output.

## 3. Required firmware

| Binary | Device |
|---|---|
| `small_obstacle_detection_6843.bin` ([Releases](../../../../releases)) | uRAD Industrial (IWR6843AoP) |

From the TI Radar Toolbox 4.00.00.05 (`prebuilt_binaries/`). Flash it with
[TI UniFlash](https://www.ti.com/tool/UNIFLASH):

1. Put the board in **flashing mode** with the DIP switches / SOP jumpers —
   see chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf).
2. In UniFlash select the IWR6843AoP, load the `.bin` as *meta image* and
   flash over the control COM port.
3. Restore **functional mode** and power-cycle the board.

## 4. Chirp configurations

Three variants are provided in [`chirp_config/`](chirp_config), all for
the AoP (antenna-on-package) module used by the uRAD Industrial. They share
the same chirp profile (60.75 GHz start, ~3.9 GHz bandwidth, 20 fps) and
differ in detection sensitivity and state machine tuning:

| File | Use it when |
|---|---|
| `AOP_3d_Tracking_Small_Obstacle_Indoor.cfg` | Default starting point. Balanced CFAR thresholds for indoor/controlled environments. |
| `AOP_3d_Tracking_Small_Obstacle_High_Detection.cfg` | You miss small obstacles: lower static CFAR threshold and a very fast zone entry (1 frame), at the cost of more false alarms. Zones stay occupied longer (25-frame exit). |
| `AOP_3d_Tracking_Small_Obstacle_Low_Noise.cfg` | You get false alarms: higher dynamic CFAR threshold and smaller static CFAR windows for noisy/outdoor scenes. |

All three configure **one zone**: x −1.8…1.8 m, y 0.2…1.5 m,
z −0.5…1.5 m (radar-relative; the default mount is ~0.5 m above the
ground, so z = −0.5 m is ground level). The `6432`/`ISK` variants shipped
by TI are for other hardware and are intentionally not included.

Key parameters you may tune (from the TI tuning guide):

| Command | Parameters | Notes |
|---|---|---|
| `zoneDef` | `zoneIdx xmin xmax ymin ymax zmin zmax` (m) | One line per zone, max 2 zones. `xmin/xmax`: platform width (device centered: ±half width). `ymin/ymax`: where the zone starts/ends in front of the sensor — for two zones make them non-overlapping (e.g. stop zone 0.2–1.0 m, slow zone 1.0–2.5 m). `zmin`: as high as possible while still catching the obstacles (sensor 0.35 m above ground → −0.3 m suggested); `zmax`: up to the platform height. Keep `maxY` realistic — too far causes false alarms. |
| `occStateMach` | `numZones pointsEntry snrEntry frameEntry pointsMaintain snrMaintain pointsExit frameExit` | Obstacles not triggering the zone → lower `pointsEntryThreshold`, `snrEntryThreshold`, `frameEntryThreshold`. Zone state flickers → lower `pointsMaintainThreshold`/`snrMaintainThreshold`. Zone stays occupied too long → raise `pointsExitThreshold` or lower `frameExitThreshold`. Counts are frames at 20 fps (50 ms each). |
| `staticRACfarCfg` / `dynamicRACfarCfg` | CFAR windows and thresholds (dB) | Point cloud too sparse on small objects → lower the threshold values (10th/11th parameters); too much noise → raise them. |
| `sensorPosition` | `height azimuthTilt elevationTilt` | Set the real mounting height (m). TI recommends mounting low with a slight uptilt to minimize ground reflections. |
| `boundaryBox` / `staticBoundaryBox` | `xmin xmax ymin ymax zmin zmax` (m) | Tracker limits; keep them enclosing your zones. |

> After editing a configuration, keep `sensorStop` … `sensorStart` intact
> and mind that the parser is strict about the number of arguments.

## 5. Running the client

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
```

Windows (USB, two COM ports — check the device manager; the *enhanced*
port is control, the *standard* port is data):

```bash
urad-small-obstacle --config config_radar.json --control-port COM8 --data-port COM7
urad-small-obstacle --config config_radar.json --gui
urad-small-obstacle --config config_radar.json --chirp chirp_config/AOP_3d_Tracking_Small_Obstacle_High_Detection.cfg
```

Raspberry Pi (single UART through the uRAD adapter, with automatic chip
reset before configuring):

```bash
urad-small-obstacle --config config_radar.json --single-port /dev/serial0 --gpio-reset-pin 18
```

Edit [`config_radar.json`](config_radar.json) to set your default ports
(control 115200 baud, data 921600 baud) and chirp configuration; every CLI
flag overrides the file.

Full CLI reference:

| Flag | Meaning |
|---|---|
| `-c`, `--config` | Path to the JSON configuration file (required) |
| `--control-port` | Override the control serial port |
| `--data-port` | Override the data serial port |
| `--single-port PORT` | One physical UART for control and data (Raspberry Pi) |
| `--chirp` | Override the chirp configuration file path |
| `--gpio-reset-pin PIN` | BCM pin to reset the chip before configuring (Raspberry Pi) |
| `--output-dir` | Directory for the output files (default `./output`) |
| `--no-save` | Disable all file output |
| `--gui` | Live viewer (requires `pip install urad-mmwave[gui]`) |
| `--duration SECONDS` | Stop after N seconds |
| `--max-frames N` | Stop after N frames |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

## 6. GUI

`--gui` opens two synchronized views:

- **Top view (X-Y):** the point cloud with marker **color encoding
  height** — near-ground points are drawn warm (red/orange), higher points
  cool (blue) — so low obstacles stand out immediately; marker size
  encodes SNR. Tracked obstacles appear as numbered circles. Each
  occupancy zone from the chirp configuration is drawn as a rectangle:
  green thin outline while clear, **thick red while occupied** (label
  `zone N: OCCUPIED`). The tracker `boundaryBox` is dashed grey.
- **Side view (Y-Z):** the same points against the zone height limits, to
  read exactly how high above the ground an obstacle sits.

The window title summarizes the frame: occupied zones, number of tracked
obstacles and points.

## 7. Output

Console, one line per frame (20 per second):

```
zones: 0:OCCUPIED  obstacles: 1  points: 34
```

Files (disable with `--no-save`; every row ends with the host epoch
timestamp of the frame):

| File | Row content |
|---|---|
| `PointCloud.txt` | `range_m azimuth_deg elevation_deg doppler_ms snr` × N points |
| `Obstacles.txt` | `tid x y z vx vy vz ax ay az` × N tracked obstacles |
| `ZoneOccupancy.txt` | occupancy bit mask (integer; bit N set = zone N occupied) |

UART/TLV protocol (for integrators). Framing is the standard Radar Toolbox
packet: magic word `0x0102 0x0304 0x0506 0x0708`, 40-byte header
(`version, totalPacketLen, platform=0xA6843, frameNumber, timeCpuCycles,
numDetectedObj, numTLVs, subFrameNumber`), packet padded to a 32-byte
multiple. TLVs (all little-endian, 8-byte type+length header):

| TLV | Content | Layout |
|---|---|---|
| 1010 | Target list | Per track (112 B): `uint32 tid`, `float posX/Y/Z, velX/Y/Z, accX/Y/Z`, `float ec[16]`, `float g`, `float confidenceLevel` |
| 1011 | Target index | `uint8` track id per point of the *previous* frame (253 not associated, 254 weak SNR, 255 noise) |
| 1020 | Compressed point cloud | 20 B of units (`float elevationUnit, azimuthUnit, dopplerUnit, rangeUnit, snrUnit`), then 8 B per point: `int8 elevation, azimuth`, `int16 doppler`, `uint16 range, snr` (multiply by the units; angles in radians) |
| 1021 | Presence indication | `uint32` (sent when `presenceBoundaryBox` is configured) |
| 1030 | **Zone occupancy** | `uint32` bit mask — bit N set = zone N occupied. Sent every frame, even with no detections. |

TLV 1030 is the only addition over the 3D People Tracking output format.
For programmatic use decode packets with
`urad_mmwave.apps.small_obstacle.parse_frame` — obstacle positions are in
`frame.obstacles` and the alarms in `frame.occupied_zones()` /
`frame.zone_occupied(n)`.

## 8. Important — one configuration per boot

TI Radar Toolbox firmwares accept **only one configuration per boot**.
Before running the client again, reset the radar: unplug and replug the
USB cable, press the physical reset button, or drive the RESET pin on the
board connector (on Raspberry Pi setups the client can do this for you via
`--gpio-reset-pin` or `gpio_reset_pin` in the JSON configuration).
If the client reports that the radar did not respond to any configuration
command, this is almost always the cause.

## 9. Troubleshooting

- **No detections / zone never occupied.** Check the mounting (low,
  facing forward, slight uptilt). Verify the target is inside the zone —
  remember z is radar-relative (ground ≈ −sensor height). Try the
  `High_Detection` configuration, then lower `pointsEntryThreshold` /
  `snrEntryThreshold`, or lower the CFAR thresholds for a denser cloud.
- **Zone always occupied.** Ground reflections: raise `zmin`, reduce
  `maxY`, or switch to the `Low_Noise` configuration.
- **Wrong COM port / no response to configuration.** The control port is
  the *enhanced* COM port (115200 baud); the data port is the *standard*
  one (921600 baud). Swapped ports produce exactly this symptom — as does
  a radar that was already configured since its last reset (section 8).
- **Garbled or no data.** Confirm the data port baudrate (921600) in
  `config_radar.json` and that no other program (TI visualizer) holds the
  port open.
- **LEDs.** After flashing and power-cycling in functional mode, a steady
  power LED and blinking status LED indicate the firmware is chirping —
  see chapter 3 of the user manual for the exact LED map of your board.

Links: [TI Radar Toolbox](https://dev.ti.com/tirex/explore/node?node=A__AC7VOFHAujuTjB9uRhwYqg__radar_toolbox__1AslXXD__LATEST)
(Small Obstacle Detection example: user guide under `docs/` in the example
folder) · [uRAD Industrial user manual](../../docs/user-manual-en.pdf) ·
[urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) ·
[TI UniFlash](https://www.ti.com/tool/UNIFLASH).

## 10. Credits

Based on the **Small Obstacle Detection** example of the TI Radar Toolbox
**4.00.00.05** (Texas Instruments). The firmware binary is redistributed
with TI authorization for use with TI devices, subject to the applicable
TI license terms. The uRAD Python client code is MIT licensed
([urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core)).
