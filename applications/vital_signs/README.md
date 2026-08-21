# Vital Signs

Contactless heart rate and breathing rate measurement for the uRAD
Industrial (IWR6843AoP), using the TI **Vital Signs with People Tracking**
firmware. The radar tracks people with the same on-chip detection and
tracking layers as the [3D People Tracking](../people_tracking) application
and estimates the vital signs of the tracked person from the phase of the
radar return at their location.

| Firmware ([Releases](../../../../releases)) | Chirp configuration |
|---|---|
| `vital_signs_tracking_6843AOP_demo.bin` | [`chirp_config/`](chirp_config) |

Typical use cases: patient monitoring in bed (sleep/respiration), elderly
care and fall-adjacent monitoring, driver or seat occupant monitoring, and
wellness applications — all without wearables or cameras.

Measurement conditions (from the TI user guide): the vital signs are only
accurate when the tracked person **stays still for at least ~20 seconds**,
seated or lying down, with the sensor pointed at their chest, at up to
~5 m distance.

## How it works

1. The **detection layer** produces a 3D point cloud and the **group
   tracker** (Extended Kalman Filter) localizes and follows the person —
   identical processing to the 3D People Tracking firmware, so the tracker
   boundary boxes and tuning described in the
   [people tracking README](../people_tracking/README.md#tracker-parameters-and-tuning)
   apply here too.
2. The **vital signs layer** selects the range bin where the tracked person
   sits (or a fixed range bin, see `VSRangeIdxCfg`) and extracts the chest
   displacement waveform from the phase of the radar return at that range.
   Band-pass filtering separates the breathing motion (~0.1–0.5 Hz) from
   the heartbeat (~1–2 Hz); the rates are estimated from both spectra.
3. Every N frames (the `vitalsign` refresh count, default 15 frames ≈
   1.35 s) the firmware outputs the rates, a breathing deviation metric and
   the last 15 samples of both waveforms.

This TI lab is distributed as a **binary only** — TI provides no source
code for the vital signs layer.

## Required firmware

Flash `vital_signs_tracking_6843AOP_demo.bin` (from this repository's
[Releases](../../../../releases)) with TI UniFlash:

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in **flashing mode** with the DIP/SOP switches — see
   chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
   and [`firmware/README.md`](../../firmware/README.md).
3. Load the `.bin` as *meta image 1* and flash.
4. Restore the **functional mode** switch position and power-cycle.

## Physical setup

- To monitor a **seated person**: mount the radar at ~1–1.5 m height with
  0–15° downtilt, facing the person's chest.
- To monitor a **person in bed**: mount it on the ceiling pointing down at
  the torso.
- Keep `sensorPosition <height> <azimuthTilt> <elevationTilt>` in the chirp
  configuration consistent with the real mounting (the provided configs
  ship with `sensorPosition 2 0 15`).

## Chirp configurations provided

| File | Use |
|---|---|
| [`vital_signs_AOP_2m.cfg`](chirp_config/vital_signs_AOP_2m.cfg) | Default — boundary box limited to ~2 m: seated or in-bed scenarios with the subject close to the radar |
| [`vital_signs_AOP_6m.cfg`](chirp_config/vital_signs_AOP_6m.cfg) | Boundary box up to 6 m for larger rooms (vital signs remain most accurate below ~5 m) |

Typical chirp parameters (both configurations): start frequency 60.75 GHz,
slope 200 MHz/µs, 96 samples per chirp, bandwidth 1.78 GHz, range
resolution 8.4 cm, maximum unambiguous range 7.2 m, maximum radial velocity
8.38 m/s, frame period 90 ms, 3 Tx × 4 Rx.

### Application-specific parameters

Besides the tracker commands shared with people tracking (`boundaryBox`,
`staticBoundaryBox`, `presenceBoundaryBox`, `sensorPosition`,
`gatingParam`, `allocationParam`, `stateParam`, `maxAcceleration`,
`trackingCfg` — see the
[tuning section](../people_tracking/README.md#tracker-parameters-and-tuning)
of the people tracking README), the vital signs firmware accepts two extra
commands:

| Command | Parameters | Meaning |
|---|---|---|
| `vitalsign` | refreshFrames windowSize | `refreshFrames`: number of frames between vital signs outputs on the UART (default 15; increase for more stable readings, decrease for faster refresh). `windowSize`: debug setting — keep at 300. |
| `VSRangeIdxCfg` | enable rangeBin | `enable = 0` (default): the measurement range follows the tracked person's position. `enable = 1`: fixed range mode — vital signs are measured at `rangeBin × range resolution` (e.g. bin 20 × 8.4 cm ≈ 1.68 m) regardless of the tracker. |

The provided configurations use `vitalsign 15 300` and `VSRangeIdxCfg 0 21`.

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
urad-vital-signs --config config_radar.json --control-port COM8 --data-port COM7

# Live GUI (requires: pip install urad-mmwave[gui])
urad-vital-signs --config config_radar.json --gui

# 6 m configuration
urad-vital-signs --config config_radar.json --chirp chirp_config/vital_signs_AOP_6m.cfg
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
| `--gui` | Live waveform view (requires the `gui` extra) |
| `--duration` SECONDS | Stop after this many seconds (default: run until Ctrl+C; ignored with `--gui`) |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

On Raspberry Pi (with the uRAD Raspberry Pi adapter) set both
`control_serial.port` and `data_serial.port` to `/dev/serial0` in the JSON
file, and optionally add `"gpio_reset_pin": <BCM pin>` so the client resets
the radar automatically before configuring it (requires
`pip install urad-mmwave[rpi]`). See the
[Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).

> **Important — one configuration per boot:** TI Radar Toolbox application
> firmwares accept only **one** chirp configuration per boot. Before running
> the client again, reset the radar: unplug and replug the USB cable, press
> the reset button, or drive the RESET pin on the board connector (on
> Raspberry Pi the client does this automatically via `gpio_reset_pin`).

## GUI

`--gui` opens two scrolling plots with the live **heart waveform** (red)
and **breathing waveform** (blue) — each vital signs TLV appends its 15
newest samples; the window keeps the last ~450 samples of each. The window
title shows the patient status, the median-smoothed heart rate (bpm) and
the breathing rate (rpm). Console output and file writing remain active
while the GUI runs; close the window to stop.

## Output

Per frame the client prints one console line:

```
patient: present  heart: 64.2 bpm  breath: 14.8 rpm  (deviation: 0.113)
```

The patient status replicates the TI visualizer logic, based on the
breathing deviation reported by the firmware (recomputed every refresh over
the last 40 breathing waveform values):

| Status | Condition |
|---|---|
| `no patient` | No active track |
| `measuring` | Track present but no valid measurement yet (deviation = 0) |
| `holding breath` | Breathing deviation below 0.02 |
| `present` | Valid measurement; rates displayed |

The printed heart rate is the **median of the last 10 valid measurements**
(same smoothing as the TI visualizer). Note that the firmware keeps
measuring at the last locked range bin even if the tracker drops a person
who sits perfectly still, so valid readings can continue with zero active
tracks.

Unless `--no-save` is given, three append-only text files are written to
`--output-dir` (one line per frame with data; every line ends with the host
epoch timestamp in seconds):

| File | Per-frame content |
|---|---|
| `VitalSigns.txt` | target id, range bin, breathing deviation, heart rate (bpm), breathing rate (rpm) — then timestamp |
| `PointCloud.txt` | For each point: range (m), azimuth (deg), elevation (deg), Doppler (m/s, signed), SNR — then timestamp |
| `Targets.txt` | For each track: tid, posX, posY, posZ (m), velX, velY, velZ (m/s) — then timestamp |

For programmatic use, decode packets with
`urad_mmwave.apps.vital_signs.parse_frame` — see the module docstring.

### UART/TLV protocol (for integrators)

The packet framing is identical to the 3D People Tracking firmware
(40-byte header with magic word `02 01 04 03 06 05 08 07`, little-endian
TLVs, `0xBE` padding to a 32-byte multiple — see the
[people tracking protocol table](../people_tracking/README.md#uarttlv-protocol-for-integrators)):

| TLV type | Content |
|---|---|
| 1020 | Compressed point cloud (same layout as people tracking; Doppler int16 **signed**) |
| 1010 | Target list (112 bytes per target) |
| 1011 | Target index (uint8 per point of the previous frame) |
| 1021 | Presence indication (uint32) |
| **1040** | **Vital signs** (136 bytes): target id uint16, range bin uint16, breathing deviation float32, heart rate float32 (bpm), breathing rate float32 (rpm), heart waveform 15 × float32, breathing waveform 15 × float32 |

## Troubleshooting

- **`patient: measuring` forever**: the person must remain still for
  ~20 s; check they are inside the `boundaryBox` of the chirp
  configuration and their chest faces the sensor.
- **Erratic heart rate**: any body movement corrupts the phase
  measurement — wait for the person to settle; prefer the 2 m
  configuration at short distances; increase the `vitalsign` refresh count
  for more smoothing.
- **No detections / client hangs after configuration**: the radar was
  already configured this boot — reset it (one configuration per boot).
- **Wrong COM port / no data**: control port is the *Silicon Labs CP210x
  Enhanced* port (115200), data port the *Standard* one (921600); on Linux
  `/dev/ttyUSB0` and `/dev/ttyUSB1`.

## Documentation and links

- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX) (browsable in
  the [TI Resource Explorer](https://dev.ti.com)) → *Industrial and
  Personal Electronics → Vital Signs → Vital Signs With People Tracking*
  user guide; the tracker layers are documented under *People Tracking*
  (detection and tracker layer tuning guides).
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf) (flashing:
  chapter 3) and [Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) —
  Python client source, configuration reference and general
  troubleshooting.

## Credits

Based on the **Vital Signs With People Tracking** example of the TI Radar
Toolbox **4.00.00.05** (binary-only TI lab). The firmware binary is
redistributed with Texas Instruments' authorization and remains subject to
the applicable TI license terms. The uRAD client code (urad-mmwave) is
released under the MIT License by Anteral.
