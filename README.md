# uRAD Industrial SDK

**Official SDK for the [uRAD](https://urad.es) Industrial radar by Anteral** —
a 60 GHz mmWave evaluation board based on the Texas Instruments **IWR6843AoP**
(antenna-on-package).

*Leer en [español](README.es.md).*

## Repository layout

| Directory | Contents |
|---|---|
| [`docs/`](docs) | User manual and Raspberry Pi adapter guide (EN/ES) |
| [`mechanical/`](mechanical) | 3D model of the board (STEP) |
| [`firmware/`](firmware) | Firmware flashing guide; binaries are in [Releases](../../releases) |
| [`applications/`](applications) | Product applications (people tracking, level sensing, vital signs, area scanner, automated doors, small obstacle detection, CPD with classification) |

## Quick start (out-of-box demo)

1. Flash the out-of-box firmware (`out_of_box_6843_aop.bin` from
   [Releases](../../releases)) — see [`firmware/README.md`](firmware/README.md).
2. Install the [urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) Python
   SDK:

   ```bash
   pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
   ```

3. Run the demo with this product's profile (identify your COM ports first):

   ```bash
   urad-mmwave --config profiles/industrial/config_radar.json --data-port COM7 --control-port COM8
   ```

   Add `--gui` for the live point cloud viewer. The full configuration
   reference and troubleshooting live in the
   [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core) README.

## Applications

### 3D People Tracking (standard and overhead)

People detection and tracking with dedicated TI firmware (formerly named
"People Counting"). Both mounting variants (wall and ceiling) share the
same client:

```bash
urad-people-tracking --config applications/people_tracking/config_radar.json
```

See [`applications/people_tracking/`](applications/people_tracking).

### High Accuracy Level Sensing

Millimeter-accuracy distance measurement (12–150 m) with dedicated firmware:

```bash
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12
```

See [`applications/level_sensing/`](applications/level_sensing).

### Vital Signs

Contactless heart rate and breathing rate measurement with dedicated TI
firmware:

```bash
urad-vital-signs --config applications/vital_signs/config_radar.json
```

See [`applications/vital_signs/`](applications/vital_signs).

### Area Scanner

People and object tracking with static object detection and configurable
safety zones (critical / warning), for industrial safety:

```bash
urad-area-scanner --config applications/area_scanner/config_radar.json
```

See [`applications/area_scanner/`](applications/area_scanner).

### Automated Doors and Gates

Track-based trigger for automated doors: approach zone, time-to-door
estimate and static obstruction detection:

```bash
urad-automated-doors --config applications/automated_doors/config_radar.json
```

See [`applications/automated_doors/`](applications/automated_doors).

### Small Obstacle Detection

Detection of small, low-lying obstacles for mobile robots, with zone
occupancy alarms:

```bash
urad-small-obstacle --config applications/small_obstacle_detection/config_radar.json
```

See [`applications/small_obstacle_detection/`](applications/small_obstacle_detection).

### CPD with Classification

In-cabin occupancy detection over configurable seat zones with adult/child
classification (Child Presence Detection):

```bash
urad-cpd --config applications/cpd_with_classification/config_radar.json
```

See [`applications/cpd_with_classification/`](applications/cpd_with_classification).

## Texas Instruments resources

The TI documentation previously bundled with this SDK is available from TI:
the [mmWave SDK](https://www.ti.com/tool/MMWAVE-SDK) user guide, the people
tracking and vital signs guides in the
[TI Resource Explorer](https://dev.ti.com) (Radar Toolbox), and
the [IWR6843AoP product page](https://www.ti.com/product/IWR6843AOP).

## License

Code and documentation authored by Anteral are released under the
[MIT License](LICENSE). Texas Instruments firmware and documentation remain
subject to their respective TI licenses.
