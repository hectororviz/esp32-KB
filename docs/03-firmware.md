# 03 — Firmware

> Arquitectura propuesta para el firmware en **ESP-IDF 5.x (C)**. Diseñada en módulos
> (componentes ESP-IDF) para que cada parte sea testeable y reemplazable.

## 1. Visión general

El firmware es una **máquina de estados** pequeña y clara:

```
                ┌─────────────────────────────────────────────┐
                │  main loop (task, cada ~5 ms)               │
                │                                             │
                │  ┌─────────┐   ┌────────┐   ┌───────────┐   │
                │  │ matrix  │──►│ keymap │──►│ hid_route │──►│─► USB HID
                │  │ scan    │   │ (1 capa)│  │ (estado   │   │─► BLE HID
                │  │ debounce│   └────────┘   │ compartido)│  │
                │  └─────────┘                 └───────────┘   │
                │        │                                     │
                │        ▼                                     │
                │  ┌─────────────┐   encoder (giro/SW)         │
                │  │    apps     │◄──────────────┘             │
                │  │ TECLADO │   │                             │
                │  │ MENU    │   │   ┌────────┐                │
                │  │ CALC    │───│──►│ display │ (SH1106)      │
                │  └─────────────┘   └────────┘                │
                │        │            ▲                        │
                │  ┌─────────┐   ┌────────────┐                │
                │  │  power  │──►│ settings   │                │
                │  │ (ADC    │   │ (NVS)      │                │
                │  │  batería)│  └────────────┘                │
                │  └─────────┘                                 │
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
│   ├── app_main.c          ← init + main loop + despacho del encoder
│   ├── matrix.c/.h         ← scan 5×4 + debounce
│   ├── keymap.c/.h         ← keymap de 1 sola capa (keycodes HID)
│   ├── encoder.c/.h        ← EC11: cuadratura + SW con debounce
│   └── hid_route.c/.h      ← API de envío a USB y BLE
└── components/
    ├── hid_usb/            ← TinyUSB: HID keyboard
    ├── hid_ble/            ← Bluedroid esp_hidd: BLE HID keyboard
    ├── display/            ← SH1106 + primitivas de dibujo (texto)
    ├── apps/               ← modos TECLADO/MENÚ/CALC + calculadora + menú
    ├── settings/           ← persistencia en NVS (contraste, encoder, sleep)
    ├── power/              ← batería, carga, sleep, wake
    └── keycodes/           ← constantes de keycodes HID y especiales
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

El **main loop** corre un ciclo de escaneo cada 5 ms:

```c
while (1) {
    matrix_scan();                 // lee filas/columnas, aplica debounce
    // teclas nuevas → apps (calculadora) o toggle de CALC
    // encoder: giro → apps_encoder_turn() o volumen;
    //           SW → apps_encoder_press() (abre menú / confirma / base)
    keymap_build_report(pressed, &kbd);   // solo si modo TECLADO
    hid_route_update(&kbd);               // empuja el estado a USB y/o BLE
    vTaskDelay(5ms);
}
```

El **encoder** (v0.3): si el intervalo entre pasos es < 25 ms (giro rápido) el evento se
dispara **dos veces** (volumen/menú "saltan" más rápido).

## 4. `matrix` — escaneo y debounce

- **Scan**: filas en push-pull, se baja una a la vez; columnas se leen con pull-up.
  Ciclo completo cada ~10 ms.
- **Debounce**: cada tecla tiene un contador; se acepta el cambio de estado solo si se
  mantiene estable N muestras (típico 5 muestras @ 10 ms = 50 ms de estabilización,
  ajustable a ~20 ms para MX).
- Salida: `uint32_t key_state` (bit por tecla) + evento de **flanco** (press/release)
  para el loop principal (toggle de CALC, calculadora).

## 5. `keymap` — una sola capa

Estructura de datos: `uint16_t keymap[20]` (20 teclas, **1 sola capa**). Cada entrada es
un **keycode HID** o un keycode especial (`KC_CALC` abre/cierra la calculadora; nunca se
envía al host).

```c
static const uint16_t s_keymap[MATRIX_KEYS] = {
    KC_HID(HID_KP_NUMLOCK), KC_HID(HID_KP_DIVIDE),   KC_HID(HID_KP_MULTIPLY), KC_HID(HID_KP_SUBTRACT),
    KC_HID(HID_KP_7),       KC_HID(HID_KP_8),        KC_HID(HID_KP_9),        KC_HID(HID_KP_ADD),
    KC_HID(HID_KP_4),       KC_HID(HID_KP_5),        KC_HID(HID_KP_6),        KC_HID(HID_KP_EQUAL),
    KC_HID(HID_KP_1),       KC_HID(HID_KP_2),        KC_HID(HID_KP_3),        KC_HID(HID_KP_ENTER),
    KC_HID(HID_KP_0),       KC_HID(HID_KP_DOT),      KC_CALC,                 KC_HID(HID_BACKSPACE),
};
```

> Decisión de diseño (v0.2): se eliminó la **capa Fn** y el **tap-hold**. Las teclas 18/19
> pasan a ser **CALC** (toggle de calculadora) y **Backspace**.

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
- La **tecla CALC** nunca se envía al host (es local, toggle de calculadora).
- El **volumen del encoder** se envía como consumer report (evento puntual vía
  `hid_route_consumer_event()`, sin estado persistente).
- `hid_route_type_string()` tipia un string al host (USB+BLE): se usa para **pegar el
  resultado** de la calculadora (dígitos, `-`, y A–F con Shift para HEX).

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

- Driver propio por I2C (módulo SH1106/SSD1306). Expone primitivas
  (`display_begin`, `display_text`) y `display_update()` delega el render a `apps_render()`.
- Tres pantallas controladas por el modo del sistema (`apps_mode()`):

  1. **HUD / status** (modo TECLADO): batería (%), estado de carga, conexión USB/BLE.
  2. **Calculadora** (modo CALC): línea de modo (DEC/HEX/BIN) + valor.
  3. **Menú de configuración** (modo MENÚ): navegado con el encoder.

- El encoder navega menús; en calculadora su pulsador cicla la base.

## 10. `apps` — modos, calculadora y menú

Componente central que mantiene el **modo** del sistema (`APP_KEYBOARD`, `APP_MENU`,
`APP_CALC`) y el render completo (`apps_render()`).

**Calculadora** (modo CALC, abierto con la tecla `CALC`):
- Entrada: dígitos, `+ − * / =`, Enter; **Backspace = borra el último dígito**.
- **Notación estándar** (operador de dos operandos con acumulador).
- **Modos**: DEC, HEX, BIN (ciclo con el pulsador del encoder).
- **Pegar resultado** (v0.3): con la tecla **NumLk** el resultado se envía al host por
  HID (`hid_route_type_string`), tipiándolo como si fuera un teclado.
- Sin memoria ni historial (descartado en el re-scope v0.2).

**Menú de ajustes** (modo MENÚ, abierto con el pulsador del encoder):
```
Info       → batería (mV/%), carga, temp del chip, MAC BLE, versión, uptime
Pantalla   → Contraste (0–255, aplica al OLED)
Encoder    → Invertir sentido (Sí/No)
Sleep      → Timeout (Apagado / 30s / 5min / 10min)
Salir      → vuelve al modo TECLADO
```
- Giro: navegar / ajustar valor. Pulsador: entrar / confirmar / salir.
- Los cambios se guardan en NVS vía el componente `settings`.

## 11. `settings` — persistencia (NVS)

- Namespace `numpad`; claves: `contrast` (u8), `enc_inv` (u8), `sleep_to` (u8).
- `settings_load()` en el arranque; `settings_save()` al confirmar en el menú.
- `g_settings` es un struct global que leen `app_main` (inversión del encoder) y `apps`.

## 12. `power` — energía y sleep

- **Medición de batería**: ADC1_CH0 (GPIO1) vía divisor. Conversión a porcentaje con
  una **curva Li-Ion calibrada** (ver `05-alimentacion-y-bateria.md`).
- **Detección de carga**: pines `CHRG`/`STDBY` del TP4056 (activos en bajo).
- **Idle / sleep** (implementado en v0.3):
  - Al cumplirse el **timeout** del setting `Sleep` (30s / 5min / 10min) sin actividad
    → `display_off()` (la matriz sigue escaneando y enviando HID).
  - A los **+5 min** de pantalla apagada, y solo si **no hay USB ni BLE conectado**:
    **light sleep** con wake por GPIO (columnas en LOW = tecla, SW del encoder en LOW,
    VBUS en HIGH = conectar USB).
  - Al despertar: `matrix_init()` + `display_on()`.
  - Limitación: **girar solo el encoder no despierta** (no hay wake por flanco en light
    sleep); despierta una tecla, el pulsador del encoder o conectar USB.
  - **Aviso de batería baja**: `< 20 %` → `BAT!` en el HUD; `< 10 %` → la línea parpadea.

## 13. Configuración (`sdkconfig`)

Puntos clave a configurar:

```ini
CONFIG_IDF_TARGET="esp32s3"
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y          # o NimBLE en su lugar
CONFIG_BLE_SMP_ENABLE=y                # bonding/pairing
CONFIG_PARTITION_TABLE_CUSTOM=y        # partición con OTA opcional
CONFIG_FREERTOS_HZ=1000
```

## 14. Flujo de datos completo (ejemplo: tocar "7")

1. `matrix_scan()` detecta el press de la tecla 7 (fila 2, col 0).
2. Debounce confirma el cambio de estado.
3. `keymap_resolve()` resuelve la tecla → `KC_HID(HID_KP_7)`.
4. `keymap_build_report()` arma el reporte con las teclas presionadas.
5. `hid_route_update()` lo envía a USB y BLE (si conectados) → el host escribe "7".
6. La pantalla HUD muestra estado (batería, conexión).

## 15. Build y flasheo

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
