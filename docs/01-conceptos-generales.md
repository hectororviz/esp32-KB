# 01 — Conceptos Generales

## 1. ¿Qué es este proyecto?

Un **teclado numérico auxiliar** ("numpad") independiente y autocontenido, construido
alrededor de un **ESP32-S3**. Un numpad es la tira de teclas a la derecha de los teclados
de tamaño completo (dígitos 0-9, operadores, Enter). Muchos teclados modernos (60%,
TKL) ya no lo incluyen, por lo que un numpad **separado y programable** es un periférico
muy útil para:

- Entrada de números de forma rápida y ergonómica (planillas, facturación, CAD, datos).
- Teclas de atajo programables (capas, macros, multimedia).
- Aprender y practicar diseño de hardware + firmware embebido.

Este proyecto, además, agrega dos características que lo separan de un numpad comercial:

1. **Doble conexión**: puede conectarse **por USB** (cable) y **por Bluetooth LE**
   (inalámbrico) al mismo tiempo, con el mismo host o alternando entre dispositivos.
2. **Es una calculadora por sí mismo**: gracias a la pantalla OLED, funciona como
   calculadora **standalone**, sin depender de la PC.

## 2. Objetivos

- Diseñar un periférico HID (Human Interface Device) funcional, reproducible y
  documentado.
- Dominar el flujo completo: especificación → diseño → firmware → integración → gabinete.
- Lograr **coexistencia USB + BLE HID** en un mismo micro (patrón no trivial).
- Extender el numpad a una **plataforma multiuso** (calculadora, menú de ajustes,
  encoder) de forma priorizada y sostenible.

## 3. Decisiones de diseño clave (y por qué)

| Decisión | Opción elegida | Alternativas descartadas | Motivo |
|---|---|---|---|
| MCU | **ESP32-S3** | ESP32, ESP32-C3, RP2040, nRF52 | USB nativo + BLE en un solo chip, gran ecosistema, memoria suficiente |
| Framework | **ESP-IDF (C)** | Arduino, QMK, ZMK, KMK | Control fino de USB+BLE, ejemplos oficiales, power management |
| Conexión inalámbrica | **BLE HID** | Bluetooth Classic | El S3 solo tiene BLE; compatible con OS modernos |
| N° de teclas | **20** (5×4) | 17 (numpad puro), macropad custom | Numpad completo + Fn/media en una matriz limpia |
| Pantalla | **OLED 1.3" I2C (SH1106)** | TFT SPI color, sin pantalla | Simple, legible, bajo consumo, suficiente para calculadora |
| Batería | **18650 + TP4056** | LiPo, AA, USB-only | Capacidad, disponibilidad, simplicidad del módulo cargador |

### 3.1 Por qué ESP32-S3 y no otro chip

- **USB nativo (USB-OTG)**: puede actuar como **dispositivo HID** (teclado) directamente
  por el conector USB-C, sin puente USB-UART. Es el único ESP32 con USB nativo *y*
  Bluetooth a la vez (el S2 tiene USB pero **no** BT; el ESP32 original tiene BT Classic
  pero **no** USB nativo).
- **BLE integrado**: suficiente para HID inalámbrico.
- **Flash/RAM**: 8-16 MB de flash y 8 MB de PSRAM (según placa), espacio de sobra para
  la pantalla, la calculadora y el firmware.
- **Ecosistema**: ESP-IDF maduro, TinyUSB integrado, ejemplos oficiales de `esp_hid`
  (BLE HID), enorme comunidad.

> ⚠️ **Limitación importante**: el ESP32-S3 **no tiene Bluetooth Classic (BR/EDR)**.
> Solo BLE. Esto alcanza para Windows 10+, macOS, Linux, Android e iOS 13+, pero *no*
> para dispositivos antiguos que solo hablan BT 2.x.

## 4. Terminología esencial

| Término | Descripción |
|---|---|
| **HID** | Human Interface Device. Protocolo estándar para teclados/mouse/joysticks. |
| **HID Report** | Estructura de datos que describe qué teclas están presionadas. |
| **USB HID Device** | El ESP32 se comporta como teclado USB al enchufarlo a una PC. |
| **BLE HID** | Misma idea pero sobre Bluetooth Low Energy (servicio HID GATT). |
| **Matriz de teclas** | Técnica para leer muchas teclas con pocos GPIO (filas × columnas). |
| **Debounce** | Filtrado del rebote mecánico del switch al presionarlo. |
| **Capa (layer)** | Conjunto alterno de asignaciones de teclas activable con Fn. |
| **Keycode** | Código que identifica una tecla lógica (ej. `KC_KP_1`, `KC_VOLU`). |
| **NKRO** | N-Key Rollover: detectar N teclas a la vez sin conflictos (requiere diodos). |
| **TP4056** | Módulo cargador de Li-Ion por USB de bajo costo (hasta 1 A). |
| **Fuel gauge** | IC que mide carga de batería con precisión (el TP4056 no lo incluye). |
| **Strapping pin** | Pin del chip cuyo estado en el arranque define el modo de boot. |

## 5. Alcance

### Incluido (fase 1)
- Documentación completa de diseño (este repo).
- Firmware mínimo: USB HID + BLE HID + matriz + pantalla + batería + calculadora.

### Incluido (fases posteriores)
- Encoder, menú de ajustes en pantalla, auto-sleep. *(Descartadas en v0.2: capa Fn,
  macros, media keys — ver `04`.)*

### Fuera del alcance del firmware (hardware físico)
- Diseño de PCB, gabinete/case, placa de expansión de la matriz.
- App móvil de configuración (se puede agregar después, ver ideas en `04`).

## 6. Criterios de éxito

- El numpad escribe correctamente en una PC por USB y por BLE, sin dobles eventos.
- Se puede usar de forma inalámbrica por horas con una sola carga de 18650.
- La calculadora funciona sin ningún host conectado.
- El proyecto queda lo suficientemente documentado como para reproducirlo.
