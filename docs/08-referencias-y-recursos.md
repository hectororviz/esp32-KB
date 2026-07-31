# 08 — Referencias y Recursos

> Repositorios, ejemplos y documentación que sirvieron (o servirán) de base para este
> proyecto. Incluye el análisis del repo de referencia consultado.

## 1. Repo de referencia analizado: `paul356/esp32_keyboard`

**URL**: https://github.com/paul356/esp32_keyboard

**Qué es**: firmware de un teclado **60% completo** para ESP32-S3, con pantalla LCD,
RGB, encoder y app Android de configuración. Porta `tmk_core` + `quantum` (QMK) y
funciona con **USB HID (TinyUSB) + BLE HID (Bluedroid `esp_hidd`) simultáneos**.

**Lo que aporta a este proyecto**:
- **Prueba de concepto de coexistencia USB+BLE en el S3** (patrón `hid_route` de
  `03-firmware.md`).
- Patrones de **lectura de batería** (`components/miscs`), **detección de carga** y de
  **power management** (idle, BLE adv speed).
- Patrón de **GUI de pantalla** con menús (referencia para la calculadora/HUD).
- Documento `docs/POWER_OPTIMIZATION_PLAN.md` con ideas de optimización energética.

**Por qué NO se usa como base** (resumen del análisis):
- Es muy pesado para un numpad de 20 teclas (port de QMK, ~350 commits, muchos
  componentes: web, wifi, led, ota, app android).
- Requiere un **fork parcheado de ESP-IDF 5.4** (`v5.4-with-multi-hidd-conn`) solo para
  su feature de **múltiples conexiones BLE simultáneas**; para conectar a un
  dispositivo a la vez alcanza ESP-IDF estándar (a verificar).
- Está atado a un hardware específico (matriz 60 teclas, pinout, LCD y encoder
  propios).
- Licencia **GPL-3.0** (forkear obliga a abrir el código).

**Uso recomendado**: leerlo como referencia de arquitectura; no clonarlo.

## 2. Documentación oficial de Espressif

- **ESP-IDF** (framework): https://docs.espressif.com/projects/esp-idf/en/latest/
- **TinyUSB device (USB HID)**: ejemplo en ESP-IDF →
  `examples/peripherals/usb/device/tinyusb` (incluye `hid_device`).
- **BLE HID device (Bluedroid)**: ejemplo →
  `examples/bluetooth/bluedroid/ble/esp_hid_device`.
- **BLE HID device (NimBLE, más liviano)**: ejemplo →
  `examples/bluetooth/nimble/...` (usar si Bluedroid no entra en RAM).
- **API HID (esp_hid)**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_hid.html
- **API de ADC / calibración**: `esp_adc_cal` en la referencia de ESP-IDF.
- **Power management**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html
- **Tabla de keycodes USB HID**: `hal/usb_hid_usage.h` del ESP-IDF (o la especificación
  oficial USB HID: https://www.usb.org/hid).

## 3. Proyectos y librerías relacionados

- **ESP32-BLE-Keyboard** (T-vK): librería Arduino de teclado BLE HID
  (referencia de comportamiento BLE). https://github.com/T-vK/ESP32-BLE-Keyboard
- **TinyESP32-S3-USB-HID**: ejemplo Arduino del S3 como teclado USB.
  https://github.com/AndreCouture/Tiny-ESP32-S3-USB-HID
- **Adafruit — USB to BLE keyboard adapter** (S3 como bridge):
  https://learn.adafruit.com/esp32-s3-usb-to-ble-keyboard-adapter
- **QMK Firmware** (referencia de keymaps/tap-hold): soporte parcial de S3, sin BLE.
  https://github.com/qmk/qmk_firmware
- **ZMK**: no soporta ESP32 (nRF), solo como referencia conceptual de capas.
  https://github.com/zmkfirmware/zmk

## 4. Hardware / BOM (para comprar)

- Placa ESP32-S3 con USB nativo: DevKitC-1, Waveshare S3-Zero, etc.
- Switchs: Gateron/Kailh/Cherry MX (o Choc low-profile).
- Diodos 1N4148, encoder EC11, OLED 1.3" I2C (SH1106).
- TP4056 (con protección), LDO 3.3 V (HT7833/XC6206), 18650 protegida.
- Ver BOM completo y GPIO en `02-hardware.md`.

## 5. Hilos / foros útiles

- Espressif Forums: https://esp32.com/ (búsqueda "BLE HID device").
- Hackaday (ejemplos S3 como teclado): categoría Peripherals Hacks.
- Tutoriales "teclado mecánico DIY" para el tema de switches y matrices.

## 6. Licencia del proyecto

- Firmware y documentación de diseño propio → libres de restricciones ajenas.
- Recomendación: **MIT** para firmware, **CC-BY-SA** para documentación.
- Si algún día se incorpora código de `paul356/esp32_keyboard` o QMK, ese módulo queda
  bajo **GPL** y el resto debe separarse para no contaminar la licencia.
