# 3D People Tracking

People detection and tracking for the uRAD Industrial, using the TI 3D
People Tracking firmware (formerly named "People Counting"; renamed by TI
in the Radar Toolbox). Two mounting variants share this application — only
the firmware binary and chirp configuration differ:

| Variant | Firmware ([Releases](../../../../releases)) | Chirp configuration |
|---|---|---|
| Standard (wall) | `3D_people_track_6843_demo.bin` | [`chirp_config/standard/`](chirp_config/standard) |
| Overhead (ceiling) | `overhead_3d_people_track_demo_default.bin` | [`chirp_config/overhead/`](chirp_config/overhead) |

The standard variant provides configurations for 6 m and 9 m ranges,
including a `staticRetention` variant that keeps stationary people tracked
and a `sensitive` variant for higher detection sensitivity. The overhead
variant provides 3 m radial configurations (default, low bandwidth and
static retention).

## Usage

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
urad-people-tracking --config config_radar.json --data-port COM7 --control-port COM8
```

> **Note — one configuration per boot:** application firmwares accept only
> one configuration per boot. Before running the client again, reset the
> radar: unplug and replug the USB cable, press the physical reset button,
> or drive the reset pin on the board connector (on Raspberry Pi setups the
> client can do this for you via `gpio_reset_pin` in the configuration).

Edit [`config_radar.json`](config_radar.json) (or use the CLI overrides) to
select your serial ports and the chirp configuration variant. Per frame the
tool reports tracked targets and detected points, and writes
`PointCloud.txt`, `Targets.txt`, `TargetsIndex.txt` and `TargetsHeight.txt`
to the output directory (`--output-dir`, default `./output`).

For programmatic use, decode packets with
`urad_mmwave.apps.people_tracking.parse_frame` — see the module docstring
for the TLV reference (target list, target index, target height, compressed
point cloud, presence).

## Documentation

[`docs/`](docs) contains the uRAD user guides for both variants (EN). The TI
implementation and tuning guides are available in the
[TI Resource Explorer](https://dev.ti.com/tir/) (Radar Toolbox, People
Tracking).
