# High Accuracy Level Sensing

Millimeter-accuracy distance measurement (12–150 m) for the uRAD Industrial
radar, using the dedicated Level Sensing firmware
(`uRAD_LevelSensing_IWR6843AoP*.bin`, see [Releases](../../../../releases)).

Typical use cases: fluid/tank level sensing, silo fill monitoring,
displacement and position measurement in robotics and machinery, and any
application needing millimeter-level range accuracy at a distance.

## How it works

The firmware (based on the TI High Accuracy Level Sensing demo) uses one Tx
and one Rx antenna and the **Zoom-FFT** technique: a coarse 1D range FFT
first locates the spectrum peaks, then a second zoomed-in FFT re-analyzes a
small window around each peak at much higher resolution, yielding
millimeter-level range accuracy. The **multi-peak** feature measures the
**three strongest peaks** in the range profile independently — that is why
every measurement reports three ranges (strongest first). Appropriate
mechanical mounting (and optionally lensing) may be required to actually
reach millimeter accuracy.

## Firmware variants

Unlike the other applications, these binaries are **built by Anteral** from
the TI High Accuracy Level Sensing demo: TI's original firmware reported a
raw distance that their visualizer corrected with a multiplication factor
in software; the uRAD firmware applies that correction on the device, so
the distances reported over UART are real distances and no factor must be
applied by the client.

The three variants differ only in the data UART baud rate — pick the one
your host supports:

| Firmware ([Releases](../../../../releases)) | Data UART baud rate | Use case |
|---|---|---|
| `uRAD_LevelSensing_IWR6843AoP_921600_br.bin` | 921600 (standard) | Default; used by the Python client |
| `uRAD_LevelSensing_IWR6843AoP_115200_br.bin` | 115200 | Hosts without high-speed UART |
| `uRAD_LevelSensing_IWR6843AoP_9600_br.bin` | 9600 | Arduino and other slow hosts |

The control UART is always 115200 baud.

> **No one-configuration-per-boot limit:** unlike the TI Radar Toolbox
> application firmwares, the uRAD Level Sensing firmware can be
> reconfigured repeatedly without resetting the board — the client stops
> the sensor (`sensorStop`) when it finishes and a new run simply sends a
> new configuration.

### Flashing

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in **flashing mode** with the DIP/SOP switches — see
   chapter 3 of the [uRAD Industrial user manual](../../docs/user-manual-en.pdf)
   and [`firmware/README.md`](../../firmware/README.md).
3. Load the `.bin` as *meta image 1* and flash.
4. Restore the **functional mode** switch position and power-cycle.

## Measurement parameters

There is no `.cfg` chirp file for this application: the client generates
the radar configuration from the measurement parameters (same code for
Industrial at 60 GHz and Automotive at 77 GHz):

| Parameter (CLI flag) | Units / range | Effect |
|---|---|---|
| Maximum distance (`--max-distance`) | m, 12–150 (default 12) | Sets the chirp slope (steeper slope for shorter max distance = finer resolution). Use the smallest value that covers your scene. |
| Range of interest (`--range MIN MAX`) | m (default 0–10) | Maps to the firmware `RangeLimitCfg` command: peaks are only searched inside this window — use it to mask spurious reflections (e.g. tank walls). |
| Offset (`--offset`) | m (default 0) | Constant added to every reported range (mounting/reference-plane compensation). |
| Measurements (`-n`) | frames (default 5) | The printed result is the mean of N frames. |
| Sampling rate (`--sampling-rate`) | Hz, 2–20 (default 20; 10 with `--spectrum`) | Radar frame rate. |

## Python

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK — no code in
this folder is needed:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12 --range 0 10 -n 5
```

Add `--gui` (requires `pip install urad-mmwave[gui]`) for a live plot of
the three ranges over time, or `--spectrum` for the full FFT range
profile computed from the raw ADC samples with the detected peaks and
their amplitudes marked. For Raspberry Pi single-UART setups add
`--gpio-reset-pin 6` and omit `--data-port`. Run
`urad-level-sensing --help` for all options, or use the API directly
(`urad_mmwave.apps.level_sensing.measure`, or `.stream` for continuous
readings).

### CLI reference

| Flag | Description |
|---|---|
| `--model {AWR,IWR}` | **Required.** `IWR` = uRAD Industrial (60 GHz), `AWR` = uRAD Automotive (77 GHz) |
| `--control-port` PORT | **Required.** Control serial port (115200 baud) |
| `--data-port` PORT | Data serial port (921600 baud); omit for single-UART setups (Raspberry Pi) |
| `--max-distance` M | Maximum measurable distance in meters, 12–150 (default: 12) |
| `--range` MIN MAX | Range of interest in meters (default: 0 10) |
| `--offset` M | Offset added to every measured range (default: 0) |
| `-n`, `--measurements` N | Frames averaged per measurement (default: 5) |
| `--interval` SECONDS | Repeat every N seconds (default: measure once and exit) |
| `--gpio-reset-pin` PIN | BCM pin to reset the chip before measuring (Raspberry Pi) |
| `--gui` | Live plot of the three ranges over time (requires the `gui` extra) |
| `--spectrum` | Live FFT range profile from the raw ADC samples, peaks marked (requires the `gui` extra) |
| `--sampling-rate` HZ | Frames per second, 2–20 (default: 20, or 10 with `--spectrum`) |
| `-v`, `--verbose` | Debug logging |
| `--version` | Print the client version |

Example command lines:

```bash
# Windows, USB, single averaged measurement
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12

# Continuous logging every 2 s, range of interest 0.5-6 m
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --range 0.5 6 --interval 2

# Raspberry Pi, single UART with automatic chip reset
urad-level-sensing --model IWR --control-port /dev/serial0 --gpio-reset-pin 6
```

## Output and UART protocol

Each measurement prints the three ranges in meters (mean of `-n` frames,
strongest peak first):

```
1.2534 3.8917 0.4823
```

For integrators reading the UART directly (data port; little-endian): each
frame starts with the standard 36-byte header — magic word
`02 01 04 03 06 05 08 07` followed by version, total packet length,
platform, frame number, CPU time, number of detected objects and number of
TLVs (7 × uint32) — then the TLVs:

| TLV type | Content | Payload layout |
|---|---|---|
| 1 | Three high-accuracy ranges | Descriptor (numDetectedObj uint16, xyzQFormat uint16), then r1_low uint16, r3_low uint16, r2_low uint16, r1 int16, r2 int16, r3 int16. Range *i* in meters = `((r_i << 16) + r_i_low) / 2^20`. The distance correction is already applied on the device — no extra factor. |
| 2 | Raw ADC samples (only when requested, e.g. `--spectrum`) | Interleaved float32 `I0 Q0 I1 Q1 …` (512 complex samples per frame) |

## Troubleshooting

- **`No valid level sensing frames received`**: check the data port and
  its baud rate — it must match the flashed firmware variant (the Python
  client expects the 921600 variant); check the board is in functional
  mode.
- **Wrong COM port**: control port is the *Silicon Labs CP210x Enhanced*
  port (115200), data port the *Standard* one; on Linux `/dev/ttyUSB0`
  (control) and `/dev/ttyUSB1` (data).
- **A range reads a fixed nonsense value**: the corresponding peak does
  not exist in the scene (fewer than three real targets) — use `--range`
  to restrict the search window to your target, and read only the ranges
  you need.
- **Reflections from tank walls / mounting structure dominate**: narrow
  `--range`, improve the mounting so the antenna boresight is
  perpendicular to the surface.

## Other platforms

- [`cpp/level_sensing_UART.cpp`](cpp/level_sensing_UART.cpp) — portable
  desktop C++17 reference client (Windows and Linux, no dependencies;
  dual UART). Build with
  `g++ -std=c++17 -O2 -o level_sensing level_sensing_UART.cpp` or
  `cl /std:c++17 /O2 /EHsc level_sensing_UART.cpp`, then run
  `level_sensing COM5 COM4` (`--baud` must match the firmware variant).
- [`arduino/uRAD_LevelSensing.ino`](arduino/uRAD_LevelSensing.ino) — Arduino
  sketch (single UART: configures at 115200, then reopens for data).
  Set the `RadarDataBaudRate` define to the flashed firmware variant;
  the default 9600 matches the recommended `9600_br` binary.

## Documentation

[`docs/`](docs) contains the user manual (EN/ES), application notes (EN/ES)
and the performance report (EN). See also:

- [TI Radar Toolbox](https://www.ti.com/tool/RADAR-TOOLBOX) (browsable in
  the [TI Resource Explorer](https://dev.ti.com)) → *Industrial and
  Personal Electronics → Level Sensing → High Accuracy* user guide
  (Zoom-FFT theory, multi-peak feature, `RangeLimitCfg`).
- [uRAD Industrial user manual](../../docs/user-manual-en.pdf) (flashing:
  chapter 3) and [Raspberry Pi adapter guide](../../docs/raspberry-pi-adapter-en.pdf).
- [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) —
  Python client source, API reference and general troubleshooting.

## Credits

Based on the **High Accuracy Level Sensing** example of the TI Radar
Toolbox **4.00.00.05**. The firmware binaries are built by Anteral from the
TI demo sources (with the on-device distance correction and per-variant
data baud rates) and redistributed with Texas Instruments' authorization,
subject to the applicable TI license terms. The uRAD client code
(urad-mmwave) is released under the MIT License by Anteral.
