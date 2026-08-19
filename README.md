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
| [`applications/`](applications) | Product applications (people counting, level sensing) |

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

### 3D People Counting (standard and overhead)

People detection and tracking with dedicated TI firmware. Both mounting
variants (wall and ceiling) share the same client:

```bash
urad-people-counting --config applications/people_counting/config_radar.json
```

See [`applications/people_counting/`](applications/people_counting).

### High Accuracy Level Sensing

Millimeter-accuracy distance measurement (12–150 m) with dedicated firmware:

```bash
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12
```

See [`applications/level_sensing/`](applications/level_sensing).

## Texas Instruments resources

The TI documentation previously bundled with this SDK is available from TI:
the [mmWave SDK](https://www.ti.com/tool/MMWAVE-SDK) user guide, the 3D
people counting guides in the
[TI Resource Explorer](https://dev.ti.com/tir/) (Industrial Toolbox), and
the [IWR6843AoP product page](https://www.ti.com/product/IWR6843AOP).

## License

Code and documentation authored by Anteral are released under the
[MIT License](LICENSE). Texas Instruments firmware and documentation remain
subject to their respective TI licenses.
