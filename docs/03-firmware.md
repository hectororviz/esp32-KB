# 03 — Firmware

> Arquitectura propuesta para el firmware en **ESP-IDF 5.x (C)**. Diseñada en módulos
> (componentes ESP-IDF) para que cada parte sea testeable y reemplazable.

## 1. Visión general

El firmware es una **máquina de estados** pequeña y clara:

```
                ┌─────────────────────────────────────────────┐
                │  main loop (task, cada ~5-10 ms)             │
                │                                             │
  [interrupción]│  ┌─────────┐   ┌────────┐   ┌───────────┐   │
  GPIO / timer  │  │ matrix  │──►│ keymap │──►│ hid_route │──►│─► USB HID
  encoder IRQ ──┼─►│ scan    │   │ layers │   │ (estado   │   │─► BLE HID
                │  │ debounce│   │ taphold│   │ compartido)│  │
                │  └─────────┘   └────────┘   └───────────┘   │
                │        │            │                       │
                │        ▼            ▼                       │
                │  ┌─────────┐   ┌────────────┐               │
                │  │  apps   │◄──│  display   │               │
                │  │(calc...)│   │  UI/HUD    │               │
                │  └─────────┘   └────────────┘               │
                │        │            │                       │
                │  ┌─────────┐   ┌────────────┐               │
                │  │  power  │──►│ (ADC batería,│              │
                │  │  mgmt   │   │  sleep)     │              │
                │  └─────────┘   └────────────┘               │
                └─────────────────────────────────────────────┘
```

## 2. Estructura de componentes ESP-IDF

```
esp32-KB-fw/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c          ← init + main loop
│   ├── matrix.c/.h         ← scan 5×4 + debounce
│   ├── keymap.c/.h         ← capas, tap-hold, keycodes
│   └── hid_route.h         ← API de envío de teclas
└── components/
    ├── hid_usb/            ← TinyUSB: HID keyboard + consumer
    ├── hid_ble/            ← Bluedroid esp_hidd: BLE HID keyboard
    ├── display/            ← SH1106 + UI (HUD, calculadora, menú)
    ├── apps/               ← framework de apps + calculadora
    └── power/              ← batería, carga, sleep, wake
```

### Por qué componentes separados
- Cada componente tiene su `CMakeLists.txt` y sus headers públicos (`include/`).
- Se pueden probar de forma aislada y deshabilitar por `sdkconfig`.
- Reutilizables en otros proyectos.

## 3. `main` — bucle principal

Tareas FreeRTOS:

| Tarea | Prioridad | Stack | Función |
|---|---|---|---|
| `main_loop` | 3 | 4096 | Escaneo de matriz, debounce, keymap, envío HID |
| `display_task` | 1 | 4096 | Actualización de pantalla (I2C, ~30 FPS) |
| `power_task` | 2 | 2048 | Lectura ADC de batería, detección de carga, sleep |
| `usb_task` (TinyUSB) | — | — | Callbacks USB (provee IDF/TinyUSB) |
| `ble_task` (Bluedroid) | — | — | Stack BLE (provee IDF) |

El **main loop** corre un ciclo de escaneo cada 5-10 ms:

```c
while (1) {
    matrix_scan();        // lee filas/columnas, aplica debounce
    keymap_process();     // resuelve capas/tap-hold → lista de keycodes
    hid_route_report();   // empuja el estado compartido a USB y/o BLE
    vTaskDelay(5ms);
}
```

## 4. `matrix` — escaneo y debounce

- **Scan**: filas en push-pull, se baja una a la vez; columnas se leen con pull-up.
  Ciclo completo cada ~10 ms.
- **Debounce**: cada tecla tiene un contador; se acepta el cambio de estado solo si se
  mantiene estable N muestras (típico 5 muestras @ 10 ms = 50 ms de estabilización,
  ajustable a ~20 ms para MX).
- Salida: `uint32_t key_state` (bit por tecla) + evento de **flanco** (press/release)
  para el encoder y tap-hold.

## 5. `keymap` — capas y tap-hold

Estructura de datos: `uint16_t keymap[2][20]` (2 capas × 20 teclas). Cada entrada es un
**keycode HID** (o un keycode especial para Fn/tap-hold/macro).

- **Capa 0 (base)**: layout numpad.
- **Capa 1 (Fn)**: media keys, navegación, atajos.
- **Tap-hold**: una tecla puede tener doble comportamiento según se toque o se mantenga
  (ej. `.` al tocar, `,` al mantener — útil para cambiar separador decimal por región).

Ejemplo de tabla de keycodes (fragmento):

```c
static const uint16_t keymap[2][20] = {
    // Capa 0 (base)
    { KC_NUM, KC_KP_SLASH, KC_KP_ASTERISK, KC_KP_MINUS,
      KC_KP_7, KC_KP_8,    KC_KP_9,        KC_KP_PLUS,
      KC_KP_4, KC_KP_5,    KC_KP_6,        KC_EQL,
      KC_KP_1, KC_KP_2,    KC_KP_3,        KC_KP_ENTER,
      KC_KP_0, KC_PDOT,    KC_FN,          KC_ESC },
    // Capa 1 (Fn)
    { KC_MSEL, KC_MPRV,    KC_MNXT,        KC_MUTE,
      KC_VOLD, KC_VOLU,    KC_MSTP,        KC_MPLY,
      KC_LEFT, KC_DOWN,    KC_UP,          KC_RGHT,
      KC_HOME, KC_END,     KC_PGUP,        KC_PGDN,
      KC_TAB,  KC_DOT,     KC_NO,          KC_CALC },
};
```

## 6. `hid_route` — estado compartido USB+BLE

**Concepto central del proyecto**: una **única lista** de teclas presionadas (máx. 6
+ modificadores, o NKRO) se escribe en **ambos** transportes.

```c
typedef struct {
    uint8_t modifiers;
    uint8_t keys[6];      // teclas presionadas (6KRO estándar)
} hid_report_t;

void hid_route_report(const hid_report_t *rpt);
// → hid_usb_send(rpt)  si hay host USB
// → hid_ble_send(rpt)  si hay conexión BLE
```

- Cada transporte mantiene su propio buffer de reporte (TinyUSB y BLE no comparten
  buffers de forma segura), pero **el origen de verdad es único**: el estado de teclas.
- Si ambos están activos, se envía a los dos → sin dobles eventos en el host.
- La **tecla Fn** nunca se envía al host (es local).
- Soporte opcional de **media keys** (consumer report) para la capa Fn y el encoder.

## 7. `hid_usb` — TinyUSB

- Componente **oficial** del ESP-IDF (`tinyusb`, subcomponente `tusb_hid`).
- Report descriptor: keyboard (6KRO) + consumer control (media).
- Ejemplo base: `examples/peripherals/usb/device/tinyusb` → `hid_device`.
- No requiere driver en el host: es plug-and-play.

## 8. `hid_ble` — BLE HID (Bluedroid `esp_hidd`)

- Usa el componente `esp_hid` del ESP-IDF en modo **device** (BLE).
- Publica el **HID Service (GATT 0x1812)** con el report keyboard.
- Nombre de advertising: `Numpad-20`.
- Maneja **pairing/bonding** (Just Works). Guarda los bonds en NVS para reconexión
  automática.
- Ejemplo base: `examples/bluetooth/bluedroid/ble/esp_hid_device`.
- **Nota de RAM**: si con la pantalla activa Bluedroid consume demasiada RAM, se puede
  migrar el mismo componente a **NimBLE** (más liviano), manteniendo la API `esp_hid`.

## 9. `display` — SH1106 + UI

- Driver propio por I2C (módulo SH1106/SSD1306).
- Tres pantallas controladas por el estado del sistema:

  1. **HUD / status**: batería (%), modo (USB/BLE/ambos), última tecla enviada,
     reloj/stopwatch. Es la pantalla por defecto.
  2. **Calculadora**: dígitos grandes (~32 px), línea de operación, resultado,
     memoria. Se activa al togglear a modo calculadora.
  3. **Menú de configuración** (opcional, fase 2+): brillo, separador decimal,
     modo de sleep.

- El encoder navega menús y la calculadora edita números (scrub).

## 10. `apps` — calculadora standalone

Framework de "apps" simple: cada app recibe eventos de teclas y produce una pantalla.

**Calculadora**:
- Entrada: dígitos, `+ − * / =`, punto, `Esc` (borra), Enter.
- **Notación estándar** (con operador de dos operandos y acumulador), igual a una
  calculadora de escritorio.
- **Modos**: DEC, HEX, BIN (cambio de base en la calculadora).
- **Memoria**: M+, M−, MR, MC (persistida en RAM; opcional NVS).
- **Historial**: últimas N operaciones (scroll con encoder).
- El resultado puede **enviarse al host** con una tecla (pegado automático vía HID)
  — muy útil para "tipiar" el resultado en una planilla.

## 11. `power` — energía y sleep

- **Medición de batería**: ADC1_CH0 (GPIO1) vía divisor. Conversión a porcentaje con
  una **curva Li-Ion calibrada** (ver `05-alimentacion-y-bateria.md`).
- **Detección de carga**: pines `CHRG`/`STDBY` del TP4056 (activos en bajo).
- **Idle / sleep**:
  - 30 s sin actividad → apagar pantalla.
  - 5 min sin actividad → **light sleep** (µA de consumo).
  - Wake por cualquier tecla o giro/pulsación del encoder (GPIO wake).
  - Al volver de sleep, se re-inicializa USB/BLE para reconexión rápida.
- **Aviso de batería baja**: < 20 % → icono en pantalla; < 10 % → parpadeo + opcional
  beep.

## 12. Configuración (`sdkconfig`)

Puntos clave a configurar:

```ini
CONFIG_IDF_TARGET="esp32s3"
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y          # o NimBLE en su lugar
CONFIG_BLE_SMP_ENABLE=y                # bonding/pairing
CONFIG_PARTITION_TABLE_CUSTOM=y        # partición con OTA opcional
CONFIG_FREERTOS_HZ=1000
```

## 13. Flujo de datos completo (ejemplo: tocar "7")

1. `matrix_scan()` detecta el press de la tecla 7 (fila 2, col 0).
2. Debounce confirma el cambio de estado.
3. `keymap_process()` resuelve la tecla → `KC_KP_7` (o `KC_7` según preferencia).
4. `hid_route_report()` agrega 7 a la lista de teclas presionadas.
5. TinyUSB envía el report → la PC escribe "7".
6. BLE envía el report → el teléfono escribe "7" (si está conectado).
7. La pantalla HUD muestra el último keycode enviado.

## 14. Build y flasheo

```bash
export IDF_PATH=~/esp/esp-idf   # ESP-IDF 5.x
get_idf
idf.py set-target esp32s3
idf.py menuconfig                # ajustar pines/features
idf.py build
idf.py flash monitor
```

> Nota: el ESP32-S3 entra en modo de descarga manteniendo **BOOT en bajo** al
> conectar/ reiniciar.
