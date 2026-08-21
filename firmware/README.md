# Firmware

Prebuilt firmware binaries for the uRAD Industrial (IWR6843AoP) are published
as assets of this repository's [Releases](../../../releases) — they are not
stored in the git history.

| Binary | Application | Notes |
|---|---|---|
| `out_of_box_6843_aop.bin` | Out-of-box demo | Point cloud streaming; used with [urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) |
| `3D_people_track_6843_demo.bin` | 3D People Tracking (standard) | Wall mounting; from the TI Radar Toolbox |
| `overhead_3d_people_track_demo_default.bin` | 3D People Tracking (overhead) | Ceiling mounting; from the TI Radar Toolbox |
| `vital_signs_tracking_6843AOP_demo.bin` | Vital Signs | Heart and breathing rate; from the TI Radar Toolbox |
| `uRAD_LevelSensing_IWR6843AoP_921600_br.bin` | Level sensing | Data UART at 921600 baud (default) |
| `uRAD_LevelSensing_IWR6843AoP_115200_br.bin` | Level sensing | Data UART at 115200 baud |
| `uRAD_LevelSensing_IWR6843AoP_9600_br.bin` | Level sensing | Data UART at 9600 baud (for Arduino/slow hosts) |
| `area_scanner_68xx_demo_aop.bin` | Area Scanner | Moving object tracking plus newly added static objects; from the TI Radar Toolbox |
| `automated_doors_68xx_demo_aop.bin` | Automated Doors and Gates | Door trigger for approaching people; from the TI Radar Toolbox |
| `small_obstacle_detection_6843.bin` | Small Obstacle Detection | Low obstacles for mobile robots, with zone alarms; from the TI Radar Toolbox |
| `occupancy_detection_3d_68xx.bin` | CPD with Classification | In-cabin occupancy and child presence with adult/child classification; from the TI Radar Toolbox. Requires **xWR6843 ES2.0** |

## Flashing

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in flashing mode (see the
   [user manual](../docs/user-manual-en.pdf) for the SOP jumper settings).
3. Load the `.bin` as *meta image* and flash.
4. Restore the functional mode jumpers and power-cycle the board.

The firmware images are built from the Texas Instruments mmWave SDK and are
redistributed for use with uRAD hardware, subject to the applicable TI
license terms. The Level Sensing binaries are built by Anteral from the TI
High Accuracy Level Sensing demo, with the distance correction applied on
the device (see
[applications/level_sensing](../applications/level_sensing/README.md)); the
three variants differ only in the data UART baud rate (921600 standard,
115200 and 9600).
