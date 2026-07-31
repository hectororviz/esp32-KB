# esp32-KB — Teclado Numérico Auxiliar (Numpad-20) con ESP32-S3

Proyecto de un **teclado numérico auxiliar** ("numpad") DIY basado en **ESP32-S3**, con
conexión **USB y Bluetooth (BLE)** simultáneas, batería recargable **18650** y una
**pantalla OLED** que lo convierte también en una **calculadora standalone**.

> Estado actual: **fase de diseño y documentación**. El hardware aún no fue adquirido.
> Este repositorio contiene la documentación completa del proyecto (conceptos, ideas,
> especificaciones y plan de desarrollo) para servir como blueprint antes de comprar
> componentes y escribir el firmware.

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
└── docs/                         <- Documentación técnica completa
    ├── 01-conceptos-generales.md     Conceptos, objetivos y decisiones de diseño
    ├── 02-hardware.md                BOM, layout, matriz, GPIO map y alimentación
    ├── 03-firmware.md                Arquitectura de firmware ESP-IDF
    ├── 04-ideas-funciones.md         Catálogo de funciones/gadgets priorizados
    ├── 05-alimentacion-y-bateria.md  Energía: TP4056, curva Li-Ion, medición ADC
    ├── 06-usb-y-bluetooth.md         HID: USB + BLE, coexistencia y limitaciones
    ├── 07-plan-de-desarrollo.md      Roadmap, fases y criterios de aceptación
    └── 08-referencias-y-recursos.md  Recursos y repos de referencia
```

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

---

## Próximos pasos

- [ ] Adquirir componentes (ver BOM en `docs/02-hardware.md`).
- [ ] Validar GPIO map con la placa de desarrollo elegida (PSRAM octal, strapping).
- [ ] Configurar ESP-IDF 5.4 y probar el ejemplo USB HID.
- [ ] Construir el gabinete / PCB (fuera del alcance del firmware).
- [ ] Escribir el firmware siguiendo `docs/03-firmware.md`.

---

## Licencia

Documentación y (futuro) firmware de diseño propio. Sin restricciones impuestas por
código de terceros (a diferencia de forkeos de firmware QMK con licencia GPL). Pendiente
de decisión; se recomienda MIT para el firmware y CC-BY-SA para la documentación.
