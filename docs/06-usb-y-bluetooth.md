# 06 — USB y Bluetooth (HID)

> Conceptos de HID, cómo se comporta el ESP32-S3 como teclado por **USB** y por
> **Bluetooth LE**, y cómo conviven ambos.

## 1. ¿Qué es HID?

**HID** (Human Interface Device) es la clase de dispositivo USB definida por la
especificación USB para teclados, mouse, joysticks, etc. La misma idea se trasladó a
Bluetooth:

- **USB HID**: el dispositivo (el numpad) presenta una *interfaz HID* al host. El host
  (Windows/macOS/Linux) lo reconoce automáticamente, sin drivers.
- **BLE HID**: Bluetooth LE expone un **servicio GATT HID** (UUID 0x1812). El host
  BLE lo ve como teclado y se conecta/parea igual que un teclado inalámbrico.

En ambos casos, el teclado envía **reports** (estructuras de datos) periódicamente con
las teclas presionadas.

## 2. El report de teclado

El report clásico de un teclado USB es **6KRO** (6 teclas simultáneas + modificadores):

```
┌──────────┬───────────────┬───────────────────────────────────┐
│ Modifiers│ Reserved (1B) │ Keycodes 1..6 (6 × 1 B)           │
│ (1 B)    │ (1 B)         │                                    │
└──────────┴───────────────┴───────────────────────────────────┘
```

- Los **modifiers** son bits: Ctrl, Shift, Alt, GUI (Win/Cmd).
- Los **keycodes** son códigos de la tabla USB HID (ej. `0x59` = tecla 1 del keypad).
- Para un numpad, **6KRO sobra** (una persona no presiona más de 6 teclas a la vez en
  un numpad). **NKRO** (más teclas / todas) requiere diodos en la matriz y reportes
  más grandes; es opcional.
- Existe un report aparte de **consumer control** (media keys: volumen, play/pause)
  que se usa para la capa Fn y el encoder.

> Tabla de keycodes oficial: `usb_hid_usage_t` en el ESP-IDF
> (`tinyusb` / `hal/usb_hid_usage.h`) o la tabla pública de la especificación USB HID.

## 3. USB: el ESP32-S3 como dispositivo HID

- El **USB-OTG** del S3 puede funcionar como *device*. Los pines son fijos:
  **GPIO19 (D−) y GPIO20 (D+)**.
- El ESP-IDF integra **TinyUSB**, que implementa el stack USB. El ejemplo
  `hid_device` ya incluye teclado + consumer.
- Ventajas:
  - **Plug-and-play**: conectar el cable y funciona.
  - Alimenta vía USB (el mismo cable carga la batería).
- Limitaciones:
  - Un solo rol a la vez en el mismo conector (device aquí; no se puede usar como
    host para conectar un teclado USB externo con el mismo puerto, sin OTG switching).
  - Requiere que el cable transporte **datos** (no solo carga).

## 4. Bluetooth: el ESP32-S3 como teclado BLE

- El S3 tiene **BLE** (Bluetooth Low Energy) integrado. **No tiene Bluetooth Classic**.
- Para HID por BLE se usa el componente `esp_hid` del ESP-IDF en modo *device*:
  - Publica el **HID Service** (0x1812) con los reportes.
  - Se anuncia como `Numpad-20` en el advertising.
- **Pairing/Bonding**:
  - Método "Just Works" (sin PIN) para simplicidad.
  - Los **bonds** se guardan en **NVS**: el host se empareja una vez y luego se
    reconecta automáticamente.
  - Para "cambiar de dispositivo" (multi-host) basta guardar varios bonds y alternar
    la conexión activa.

### 4.1 Compatibilidad BLE HID con sistemas operativos

| OS | Soporte BLE HID | Notas |
|---|---|---|
| Windows 10/11 | ✅ | Perfecto |
| macOS | ✅ | Perfecto |
| Linux (BlueZ ≥ 5.49) | ✅ | Generalmente bien |
| Android | ✅ | Perfecto |
| iOS / iPadOS | ✅ (desde iOS 13) | OK para un solo teclado |
| Equipos con BT 2.x (solo Classic) | ❌ | No los verá |

> El repo de referencia (`paul356/esp32_keyboard`) menciona "iOS not supported" por su
> modo de **múltiples conexiones simultáneas** (broadcast). Para un numpad que se
> conecta a un dispositivo a la vez, iOS 13+ funciona sin problema.

## 5. Coexistencia USB + BLE (el corazón del proyecto)

### 5.1 Cómo funciona

- USB y BLE usan **hardware distinto** (USB PHY + radio BLE), por lo que pueden operar
  **al mismo tiempo** sin interferencia.
- La clave está en el **software**: ambos transportes leen un **único estado compartido
  de teclas presionadas** (ver `03-firmware.md`, módulo `hid_route`).
- Cada transporte envía su propio reporte con **el mismo contenido** → el host nunca
  ve teclas duplicadas (porque solo hay una fuente de verdad).

### 5.2 Escenarios de uso

| Escenario | USB | BLE | Resultado |
|---|---|---|---|
| Conectado por cable a la PC | ✅ | — | Teclado cableado |
| Solo batería, cerca del teléfono | — | ✅ | Teclado inalámbrico |
| Cable a la PC **y** BLE al teléfono | ✅ | ✅ | Escribe en ambos a la vez |
| Conectado al cargador (sin host) | — | ✅ | Sigue funcionando por BLE |

> Nota: si el USB está conectado a un **cargador** (no a una PC), no hay host USB → el
> numpad funciona solo por BLE. El firmware debe detectar esto (VBUS presente pero sin
> enumeración) y no bloquear el envío BLE.

### 5.3 Espejo del estado HID

```
       ┌────────────────────┐
 teclas│  hid_route_report  │
──────►│  (estado único)    ├──► TinyUSB ──► USB host
       │                    ├──► esp_hidd ──► BLE host
       └────────────────────┘
```

## 6. Consideraciones de implementación

### 6.1 Reportes en cada stack

- **TinyUSB**: se define un *report descriptor* y se escribe con `tud_hid_keyboard_report()`.
- **esp_hidd**: se configuran los *report maps* del servicio HID y se envían con
  `esp_hid_device_keyboard_send()` (o la API correspondiente).

### 6.2 RAM y stacks

- **Bluedroid** (stack BLE clásico de Espressif) consume más RAM que **NimBLE**.
- Con pantalla + BLE + USB, si Bluedroid queda justo, migrar a **NimBLE** manteniendo
  la API `esp_hid` (el ESP-IDF lo permite casi sin cambiar el código de la app).
- El repo de referencia usa Bluedroid y logra la coexistencia en el S3 (prueba de
  concepto).

### 6.3 Re-conexión y sleep

- Al salir de **light sleep**, el radio BLE y el USB se re-inicializan; el bonding se
  mantiene en NVS para reconexión automática sin re-parear.
- En idle prolongado se puede **pausar el advertising** (o usar slow-advertising) para
  ahorrar energía sin perder la conexión activa.

## 7. Referencias técnicas

- Especificación USB HID: `https://www.usb.org/hid`.
- Espressif — TinyUSB device: `examples/peripherals/usb/device/tinyusb`.
- Espressif — BLE HID device: `examples/bluetooth/bluedroid/ble/esp_hid_device`
  (y variante NimBLE en `examples/bluetooth/nimble/...`).
- Repo de referencia del análisis: `paul356/esp32_keyboard` (coexistencia USB+BLE
  probada en S3; su fork de ESP-IDF solo es necesario para múltiples hosts BLE a la vez).
