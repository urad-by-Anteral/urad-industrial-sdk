# Firmware

Prebuilt firmware binaries for the uRAD Industrial (IWR6843AoP) are published
as assets of this repository's [Releases](../../../releases) — they are not
stored in the git history.

| Binary | Application | Notes |
|---|---|---|
| `out_of_box_6843_aop.bin` | Out-of-box demo | Point cloud streaming; used with [urad-mmwave](https://github.com/<org>/urad-mmwave-core) |
| `3D_people_count_68xx_demo.bin` | 3D People Counting (standard) | Wall mounting |
| `overhead_3d_people_count_demo_default.bin` | 3D People Counting (overhead) | Ceiling mounting |
| `uRAD_LevelSensing_IWR6843AoP.bin` | Level sensing | Data UART at 921600 baud (default) |
| `uRAD_LevelSensing_IWR6843AoP_115200_br.bin` | Level sensing | Data UART at 115200 baud |
| `uRAD_LevelSensing_IWR6843AoP_9600_br.bin` | Level sensing | Data UART at 9600 baud (for Arduino/slow hosts) |

## Flashing

1. Install [TI UniFlash](https://www.ti.com/tool/UNIFLASH).
2. Put the board in flashing mode (see the
   [user manual](../docs/user-manual-en.pdf) for the SOP jumper settings).
3. Load the `.bin` as *meta image* and flash.
4. Restore the functional mode jumpers and power-cycle the board.

The firmware images are built from the Texas Instruments mmWave SDK and are
redistributed for use with uRAD hardware, subject to the applicable TI
license terms.
