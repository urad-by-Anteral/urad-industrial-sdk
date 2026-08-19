# High Accuracy Level Sensing

Millimeter-accuracy distance measurement (12–150 m) for the uRAD Industrial
radar, using the dedicated Level Sensing firmware
(`uRAD_LevelSensing_IWR6843AoP*.bin`, see [Releases](../../../../releases)).

## Python

The Python client is part of the shared
[urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core) SDK — no code in
this folder is needed:

```bash
pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12 --range 0 10 -n 5
```

For Raspberry Pi single-UART setups add `--gpio-reset-pin 6` and omit
`--data-port`. Run `urad-level-sensing --help` for all options, or use the
API directly (`urad_mmwave.apps.level_sensing.measure`).

## Other platforms

- [`cpp/level_sensing_UART.cpp`](cpp/level_sensing_UART.cpp) — C++
  reference implementation (single UART).
- [`arduino/uRAD_LevelSensing.ino`](arduino/uRAD_LevelSensing.ino) — Arduino
  sketch (use the `9600_br` firmware variant).

## Documentation

[`docs/`](docs) contains the user manual (EN/ES), application notes (EN/ES)
and the performance report (EN).
