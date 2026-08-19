# 3D People Counting

People detection and tracking for the uRAD Industrial, using the TI 3D
People Counting firmware. Two mounting variants share this application —
only the firmware binary and chirp configuration differ:

| Variant | Firmware ([Releases](../../../../releases)) | Chirp configuration |
|---|---|---|
| Standard (wall) | `3D_people_count_68xx_demo.bin` | [`chirp_config/standard/`](chirp_config/standard) |
| Overhead (ceiling) | `overhead_3d_people_count_demo_default.bin` | [`chirp_config/overhead/`](chirp_config/overhead) |

Each variant also provides a `chirp_config_fineMotion.cfg` tuned for subtle
movements.

## Usage

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
urad-people-counting --config config_radar.json --data-port COM7 --control-port COM8
```

Edit [`config_radar.json`](config_radar.json) (or use the CLI overrides) to
select your serial ports and the chirp configuration variant. Per frame the
tool reports tracked targets and detected points, and writes
`PointCloud.txt`, `Targets.txt`, `TargetsIndex.txt` and `TargetsHeight.txt`
to the output directory (`--output-dir`, default `./output`).

For programmatic use, decode packets with
`urad_mmwave.apps.people_counting.parse_frame` — see the module docstring
for the TLV reference (target list, target index, target height, compressed
point cloud, presence).

## Documentation

[`docs/`](docs) contains the uRAD user guides for both variants (EN). The TI
implementation and tuning guides are available in the
[TI Resource Explorer](https://dev.ti.com/tir/) (Industrial Toolbox).
