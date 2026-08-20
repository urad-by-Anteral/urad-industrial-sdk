# uRAD Industrial SDK

**SDK oficial del radar Industrial de [uRAD](https://urad.es) (Anteral)** —
placa de evaluación mmWave de 60 GHz basada en el **IWR6843AoP** de Texas
Instruments (antena en el encapsulado).

*Read this in [English](README.md).*

## Estructura del repositorio

| Directorio | Contenido |
|---|---|
| [`docs/`](docs) | Manual de usuario y guía del adaptador para Raspberry Pi (EN/ES) |
| [`mechanical/`](mechanical) | Modelo 3D de la placa (STEP) |
| [`firmware/`](firmware) | Guía de flasheo; los binarios están en [Releases](../../releases) |
| [`applications/`](applications) | Aplicaciones del producto (people tracking, level sensing, vital signs, area scanner, puertas automáticas, detección de obstáculos pequeños, CPD con clasificación) |

## Inicio rápido (demo out-of-box)

1. Flashea el firmware out-of-box (`out_of_box_6843_aop.bin`, en
   [Releases](../../releases)) — véase [`firmware/README.md`](firmware/README.md).
2. Instala el SDK Python [urad-mmwave](https://github.com/urad-by-Anteral/urad-mmwave-core):

   ```bash
   pip install git+https://github.com/urad-by-Anteral/urad-mmwave-core.git
   ```

3. Ejecuta la demo con el perfil de este producto (identifica antes tus
   puertos COM):

   ```bash
   urad-mmwave --config profiles/industrial/config_radar.json --data-port COM7 --control-port COM8
   ```

   Añade `--gui` para el visor de nube de puntos en tiempo real. La
   referencia completa de configuración está en el README de
   [urad-mmwave-core](https://github.com/urad-by-Anteral/urad-mmwave-core).

## Aplicaciones

### 3D People Tracking (estándar y overhead)

Detección y seguimiento de personas con firmware dedicado de TI (antes
llamado "People Counting"). Las dos variantes de montaje (pared y techo)
comparten el mismo cliente:

```bash
urad-people-tracking --config applications/people_tracking/config_radar.json
```

Véase [`applications/people_tracking/`](applications/people_tracking).

### Level Sensing de alta precisión

Medida de distancia con precisión milimétrica (12–150 m) con firmware
dedicado:

```bash
urad-level-sensing --model IWR --control-port COM8 --data-port COM7 --max-distance 12
```

Véase [`applications/level_sensing/`](applications/level_sensing).

### Vital Signs

Medida sin contacto del ritmo cardíaco y respiratorio con firmware dedicado
de TI:

```bash
urad-vital-signs --config applications/vital_signs/config_radar.json
```

Véase [`applications/vital_signs/`](applications/vital_signs).

### Area Scanner

Seguimiento de personas y objetos con detección de objetos estáticos y
zonas de seguridad configurables (crítica / aviso), para seguridad
industrial:

```bash
urad-area-scanner --config applications/area_scanner/config_radar.json
```

Véase [`applications/area_scanner/`](applications/area_scanner).

### Puertas y portones automáticos (Automated Doors and Gates)

Disparo de puertas automáticas basado en tracking: zona de aproximación,
estimación de tiempo hasta la puerta y detección de obstrucciones
estáticas:

```bash
urad-automated-doors --config applications/automated_doors/config_radar.json
```

Véase [`applications/automated_doors/`](applications/automated_doors).

### Detección de obstáculos pequeños (Small Obstacle Detection)

Detección de obstáculos pequeños a ras de suelo para robots móviles, con
alarmas de ocupación por zonas:

```bash
urad-small-obstacle --config applications/small_obstacle_detection/config_radar.json
```

Véase [`applications/small_obstacle_detection/`](applications/small_obstacle_detection).

### CPD con clasificación

Detección de ocupación de cabina por zonas de asiento configurables con
clasificación adulto/niño (Child Presence Detection):

```bash
urad-cpd --config applications/cpd_with_classification/config_radar.json
```

Véase [`applications/cpd_with_classification/`](applications/cpd_with_classification).

## Recursos de Texas Instruments

La documentación de TI que antes acompañaba a este SDK está disponible en TI:
la guía del [mmWave SDK](https://www.ti.com/tool/MMWAVE-SDK), las guías de
people tracking y vital signs en el
[TI Resource Explorer](https://dev.ti.com) (Radar Toolbox) y la
[página del IWR6843AoP](https://www.ti.com/product/IWR6843AOP).

## Licencia

El código y la documentación de Anteral se publican bajo licencia
[MIT](LICENSE). El firmware y la documentación de Texas Instruments siguen
sujetos a sus respectivas licencias de TI.
