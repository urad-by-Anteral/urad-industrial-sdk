# Vital Signs

Contactless heart rate and breathing rate measurement for the uRAD
Industrial, using the TI Vital Signs with People Tracking firmware. The
radar tracks one person and estimates their vital signs from the phase of
the radar return.

| Firmware ([Releases](../../../../releases)) | Chirp configuration |
|---|---|
| `vital_signs_tracking_6843AOP_demo.bin` | [`chirp_config/`](chirp_config) |

Two chirp configurations are provided: `vital_signs_AOP_2m.cfg` (default,
up to 2 m — seated or in-bed scenarios) and `vital_signs_AOP_6m.cfg`
(up to 6 m).

## Usage

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
urad-vital-signs --config config_radar.json --data-port COM7 --control-port COM8
```

> **Note — one configuration per boot:** this firmware accepts only one
> configuration per boot. Before running the client again, reset the radar:
> unplug and replug the USB cable, press the physical reset button, or
> drive the reset pin on the board connector (on Raspberry Pi setups the
> client can do this for you via `gpio_reset_pin` in the configuration).

Edit [`config_radar.json`](config_radar.json) (or use the CLI overrides) to
select your serial ports and chirp configuration. Per frame the tool
reports the patient status (measuring / present / holding breath), the
median-smoothed heart rate (bpm) and the breathing rate (rpm), and writes
`VitalSigns.txt`, `PointCloud.txt` and `Targets.txt` to the output
directory (`--output-dir`, default `./output`).

For programmatic use, decode packets with
`urad_mmwave.apps.vital_signs.parse_frame` — see the module docstring for
the TLV reference (vital signs plus the shared people tracking TLVs).

## Documentation

The TI user guide and implementation notes are available in the
[TI Resource Explorer](https://dev.ti.com) (Radar Toolbox, Vital
Signs).
