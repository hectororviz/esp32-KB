# esp32-KB — Teclado Numérico Auxiliar (Numpad-20) con ESP32-S3

Proyecto de un **teclado numérico auxiliar** ("numpad") DIY basado en **ESP32-S3**, con
conexión **USB y Bluetooth (BLE)** simultáneas, batería recargable **18650** y una
**pantalla OLED** que lo convierte también en una **calculadora standalone**.

> Estado actual: **documentación completa + firmware compilando** (ESP-IDF 5.4,
> target esp32s3, build verificado en esta máquina). El hardware aún no fue adquirido;
> queda pendiente la validación en placa real.

---

## Resumen del proyecto

| Campo | Valor |
|---|---|
| Microcontrolador | ESP32-S3 (USB nativo + WiFi + BLE) |
| Conexión al host | USB HID (cable) **y** Bluetooth LE HID (inalámbrico) |
| Teclas | 20 switchs mecánicos en matriz 5×4 (layout numpad + capa Fn) |
| Extras de entrada | Encoder rotativo EC11 (con pulsador) |
| Pantalla | OLED 1.3" I2C (SH1106) |
| Alimentación | 18650 Li-Ion + módulo TP4056 + LDO 3.3 V |
| Modo especial | **Calculadora standalone** con expresión en pantalla |
| Framework | ESP-IDF 5.x (C) |

### Características previstas

- Teclado numérico completo (dígitos, operadores, Enter, NumLock).
- **Capa Fn** con teclas multimedia, navegación y atajos de productividad.
- **Encoder EC11**: volumen, scroll, zoom, edición de números, navegación.
- **Calculadora standalone** con dígitos grandes, memoria (M+/M−/MR/MC) y modos
  DEC/HEX/BIN.
- **Pantalla de estado**: batería, modo activo, última tecla enviada, reloj/stopwatch.
- **Macros / snippets** de texto (emails, texto repetitivo, caracteres especiales).
- **Gestión de energía**: auto-sleep, aviso de batería baja, wake por tecla/encoder.
- **Cambio de dispositivo**: emparejar PC / tablet / teléfono y alternar con Fn+tecla.
- Tap-hold inteligente (ej.: `.` al tocar / `,` al mantener; Enter ↔ Shift).

---

## Estructura del repositorio

```
esp32-KB/
├── README.md                     <- Este archivo (visión general)
├── .gitignore
├── docs/                         <- Documentación técnica completa
│   ├── 01-conceptos-generales.md     Conceptos, objetivos y decisiones de diseño
│   ├── 02-hardware.md                BOM, layout, matriz, GPIO map y alimentación
│   ├── 03-firmware.md                Arquitectura de firmware ESP-IDF
│   ├── 04-ideas-funciones.md         Catálogo de funciones/gadgets priorizados
│   ├── 05-alimentacion-y-bateria.md  Energía: TP4056, curva Li-Ion, medición ADC
│   ├── 06-usb-y-bluetooth.md         HID: USB + BLE, coexistencia y limitaciones
│   ├── 07-plan-de-desarrollo.md      Roadmap, fases y criterios de aceptación
│   ├── 08-referencias-y-recursos.md  Recursos y repos de referencia
│   └── 09-manual-de-usuario.md       Manual de usuario (cómo usarlo)
└── firmware/                      <- Código fuente (ESP-IDF 5.4, C)
    ├── CMakeLists.txt
    ├── sdkconfig.defaults
    ├── partitions.csv
    ├── main/                         app_main, matrix, keymap, encoder, hid_route
    └── components/                   hid_usb (TinyUSB), hid_ble (esp_hidd+GAP),
                                      display (SH1106), apps (calculadora),
                                      power (batería), keycodes
```

## Firmware (build)

Requisitos: **ESP-IDF 5.4** (`~/.espressif` / `export IDF_PATH`).

```bash
cd firmware
source ~/esp/esp-idf/export.sh   # env local ya instalado (ESP-IDF v5.4 + toolchain esp32s3)
idf.py build                     # descarga espressif/esp_tinyusb vía el component manager
idf.py flash monitor             # con la placa conectada
```

> Build **verificado** en esta máquina: `esp32_kb.bin` generado sin errores ni
> warnings de nuestro código.

Puntos a revisar antes de flashear el hardware real:

- `main/board_config.h` — GPIO map (filas, columnas, encoder, OLED, batería).
- El **divisor de la batería** (`BATTERY_DIVIDER_RATIO`) en
  `components/power/power.c` según el hardware real.
- El **offset de columnas** del SH1106 (`DISPLAY_COL_OFFSET`) en
  `components/display/display.c` (típicamente 2 en paneles 1.3").

---

## Conexiones y pinout (configuración actual)

Esquema general de interconexión (es el que está configurado en
`firmware/main/board_config.h`):

```
                          ┌────────────────────────────┐
        Batería 18650 ────┤ BAT+      TP4056      BAT+ ├──── Batería
                          │ CHRG ─── GPIO2             │
                          │ STDBY── GPIO3              │
                          │ IN   ◄── USB-C 5V          │
                          └─────┬──────────────────────┘
                                │ 3.3V (vía LDO)
        ┌───────────────────────▼─────────────────────────────┐
        │                     ESP32-S3                        │
        │  R0..R4  (GPIO 6-10) ──► Filas matriz (salidas)      │
        │  C0..C3  (GPIO 11-14) ◄─ Columnas matriz (pull-up)   │
        │  A / B   (GPIO 15/16) ◄─ Encoder EC11 (cuadratura)   │
        │  SW      (GPIO 17)    ◄─ Encoder pulsador (a GND)    │
        │  SDA     (GPIO 4)     ◄─ OLED SH1106 (I2C @0x3C)     │
        │  SCL     (GPIO 5)     ─► (pull-ups externos 4.7k)    │
        │  ADC1_CH0(GPIO 1)     ◄─ Batería vía divisor 100k/100k│
        │  VBUS    (GPIO 18)    ◄─ Detección 5V USB (divisor)   │
        │  D+/D-   (GPIO 19/20) ──► USB-C (HID nativo)          │
        └───────────────────────────────────────────────────────┘
```

Tabla resumen (función → GPIO):

| Función | GPIO(s) | Tipo | Observación |
|---|---|---|---|
| Filas matriz R0–R4 | 6, 7, 8, 9, 10 | Salida push-pull | Se baja una fila a la vez |
| Columnas matriz C0–C3 | 11, 12, 13, 14 | Entrada, pull-up interno | Tecla presionada = nivel bajo |
| Encoder EC11 A / B | 15 / 16 | Entrada, pull-up | Cuadratura |
| Encoder pulsador SW | 17 | Entrada, pull-up | Pulsador a GND |
| OLED SDA / SCL | 4 / 5 | I2C (400 kHz) | Dirección `0x3C`; pull-ups externos 4.7 kΩ |
| Batería (medición) | 1 | ADC1_CH0 | Divisor 100k/100k → 3.3 V máx |
| Carga activa (TP4056 CHRG) | 2 | Entrada, activo bajo | `0` = cargando |
| Carga completa (TP4056 STDBY) | 3 | Entrada, activo bajo | `0` = carga completa |
| VBUS detect | 18 | Entrada (divisor) | `1` = conectado a USB |
| USB HID | 19 / 20 | USB nativo | D− / D+ — no usar para otra cosa |

> Notas: `GPIO0`, `GPIO45`, `GPIO46` son strapping (no usar sin cuidado); `GPIO33–37`
> suelen estar ocupados por PSRAM octal en esta placa. Detalle completo en
> [docs/02-hardware.md](docs/02-hardware.md).

---

## Guía rápida de lectura

1. Empezá por **[docs/01-conceptos-generales.md](docs/01-conceptos-generales.md)** para
   entender qué es y por qué cada decisión de diseño.
2. **[docs/02-hardware.md](docs/02-hardware.md)** tiene la lista de compras (BOM) y el
   pinout propuesto.
3. **[docs/04-ideas-funciones.md](docs/04-ideas-funciones.md)** lista todas las funciones
   pensadas, priorizadas, por si querés recortar alcance.
4. **[docs/07-plan-de-desarrollo.md](docs/07-plan-de-desarrollo.md)** es el roadmap con
   hitos verificables.
5. **[docs/09-manual-de-usuario.md](docs/09-manual-de-usuario.md)** explica cómo usar el
   teclado (conexión, capas, calculadora, solución de problemas).

---

## Próximos pasos

- [ ] Adquirir componentes (ver BOM en `docs/02-hardware.md`).
- [ ] Validar GPIO map con la placa de desarrollo elegida (PSRAM octal, strapping).
- [x] Instalar ESP-IDF 5.4 y compilar `firmware/` (build verificado).
- [ ] Construir el gabinete / PCB (fuera del alcance del firmware).
- [ ] Probar firmware en hardware real y ajustar pines/offsets.

---

## Licencia

Documentación y (futuro) firmware de diseño propio. Sin restricciones impuestas por
código de terceros (a diferencia de forkeos de firmware QMK con licencia GPL). Pendiente
de decisión; se recomienda MIT para el firmware y CC-BY-SA para la documentación.
