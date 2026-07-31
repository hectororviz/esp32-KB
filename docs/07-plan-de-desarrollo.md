# 07 — Plan de Desarrollo

> Roadmap por fases, con **criterios de aceptación verificables** en cada una. Se puede
> avanzar de a una fase; cada fase deja el proyecto en un estado usable.

## Línea de tiempo estimada

| Fase | Qué se logra | Tiempo estimado |
|---|---|---|
| 0 | Entorno listo | 0.5 día |
| 1 | Teclado por USB | 1-2 días |
| 2 | Teclado por BLE (y coexistencia) | 2-4 días |
| 3 | Matriz 5×4 + keymap + Fn | 2-3 días |
| 4 | Pantalla + calculadora | 2-3 días |
| 5 | Batería y sleep | 1-2 días |
| 6 | Extras (encoder, macros, media) | 2-4 días |
| 7 | Gabinete / pulido | variable |

> Los tiempos asumen prototipado en protoboard con una placa de desarrollo. El diseño
> de PCB y gabinete es adicional.

---

## Fase 0 — Entorno de desarrollo

**Objetivo**: poder compilar y flashear un "hola mundo" en el S3.

Tareas:
- [ ] Instalar ESP-IDF 5.x (esp-idf + herramientas).
- [ ] Configurar `idf.py set-target esp32s3`.
- [ ] Comprobar drivers del USB (modo descarga con BOOT bajo).
- [ ] Blink LED por GPIO (verifica toolchain + placa).

**Criterio de aceptación**: `idf.py flash monitor` imprime logs del boot y el LED
parpadea.

---

## Fase 1 — USB HID (teclado por cable)

**Objetivo**: el ESP32-S3 se comporta como teclado por USB (ejemplo oficial).

Tareas:
- [ ] Correr el ejemplo `hid_device` de TinyUSB con el report keyboard.
- [ ] Verificar que la PC lo detecta como teclado.
- [ ] Enviar keycodes fijos desde un loop de prueba (ej. tipear "1234").

**Criterio de aceptación**: al conectar el cable, la PC escribe los keycodes de prueba
en cualquier editor. Uso del módulo `hid_usb` según `03-firmware.md`.

**Riesgo**: el puerto USB usado para programar es el mismo que envía HID → usar el
monitor por UART (u otro puerto) durante el desarrollo.

---

## Fase 2 — BLE HID + coexistencia

**Objetivo**: teclado inalámbrico por BLE y coexistencia con USB.

Tareas:
- [ ] Correr el ejemplo `esp_hid_device` (Bluedroid) → empareja con PC/teléfono.
- [ ] Configurar nombre `Numpad-20`, advertising y bonding (guardar en NVS).
- [ ] Implementar `hid_route`: estado único de teclas → USB y BLE.
- [ ] Probar ambos conectados: escribir en PC (cable) y teléfono (BT) a la vez.

**Criterio de aceptación**: el host escribe por cable y por BT sin dobles eventos; el
teléfono se reconecta automáticamente tras apagar/prender el pad.

**Riesgo alto**: coexistencia USB+BLE (RAM de Bluedroid). Mitigación: medir RAM; si
falta, migrar a NimBLE. Prueba de concepto ya existe en `paul356/esp32_keyboard`.

---

## Fase 3 — Matriz y keymap

**Objetivo**: las 20 teclas reales y el encoder funcionan con el layout propuesto.

Tareas:
- [ ] Cablear matriz 5×4 con diodos.
- [x] Implementar scan + debounce (`matrix.c`).
- [x] Definir `keymap[20]` de una sola capa (tecla 18 = `CALC`, 19 = `Backspace`).
- [x] Integrar el encoder (A/B/SW) como entrada: giro = volumen, SW = menú/base.
- [ ] Probar ghosting presionando varias teclas a la vez.

**Criterio de aceptación**: cada tecla envía su keycode correcto por USB y BLE; el
encoder genera eventos de giro y pulsación.

---

## Fase 4 — Pantalla, calculadora y menú

**Objetivo**: OLED con HUD de estado + calculadora standalone + menú de ajustes.

Tareas:
- [x] Driver SH1106/SSD1306 por I2C (fuente 5×7).
- [x] Pantalla HUD: batería, modo de conexión.
- [x] Toggle a **modo calculadora** (tecla `CALC`): teclas → motor de cálculo.
- [x] Motor de cálculo: entrada, `+ − * / =`, punto, Backspace (borra dígito).
- [x] Modos DEC/HEX/BIN (ciclo con el pulsador del encoder).
- [x] **Menú de ajustes** (modo MENÚ, encoder): Info, Pantalla (contraste), Encoder
      (invertir), Sleep (timeout); persistido en NVS.
- [ ] "Pegar resultado" por HID (opcional).

> Re-scope v0.2: se descartaron **memoria M+/M−/MR/MC**, **historial** y el borrado con
> `Esc` (no hay tecla Esc en el layout).

**Criterio de aceptación**: sin host conectado, se puede operar la calculadora en la
pantalla; el menú guarda y restaura sus ajustes al reiniciar.

---

## Fase 5 — Energía y sleep

**Objetivo**: batería confiable + auto-sleep.

Tareas:
- [ ] Leer ADC1 (GPIO1) con divisor; calibrar con multímetro (2-3 puntos).
- [ ] Mapear voltaje → % con curva Li-Ion (no medir durante carga activa).
- [ ] Detectar `CHRG`/`STDBY`/VBUS y mostrar estado en HUD.
- [ ] Idle 30 s → apagar pantalla; 5 min → light sleep; wake por tecla/encoder.
- [ ] Alertas de batería baja (< 20 %, < 10 %).

**Criterio de aceptación**: el % coincide ±5 puntos con el multímetro en reposo; el pad
despierta de sleep al tocar cualquier tecla; la autonomía en uso normal supera varios
días (ver cálculos en `05`).

---

## Fase 6 — Extras de valor

**Objetivo**: completar el alcance v1 sugerido en `04-ideas-funciones.md`.

Tareas:
- [ ] Encoder multifunción (velocidad de giro, scroll/zoom opcionales).
- [ ] "Pegar resultado" desde la calculadora (envío HID).
- [ ] Ajustes avanzados en el menú (calibración de batería, sleep configurable).

> Re-scope v0.2: se descartaron **media keys (capa Fn)**, **macros/snippets** y
> **tap-hold**.

**Criterio de aceptación**: las funciones del catálogo P1 operativas y estables en uso
diario.

---

## Fase 7 — Gabinete y pulido

- [ ] Diseño 3D (o acrílico) para el case.
- [ ] Integrar 18650 + TP4056 + switch de alimentación.
- [ ] Pruebas de caída/fatiga, test de uso continuo.
- [ ] Documentar BOM final, fotos, esquema.
- [ ] (Opcional) Diseño de PCB para producción.

---

## Riesgos y mitigaciones

| Riesgo | Probabilidad | Impacto | Mitigación |
|---|---|---|---|
| Coexistencia USB+BLE consume mucha RAM | Media | Alto | Migrar a NimBLE; recortar features |
| ADC del S3 no lineal / ruidoso | Alta | Medio | Calibración multipunto + promediado |
| Lectura de batería errónea durante carga | Alta | Medio | No medir con `CHRG` activo |
| Pines ocupados por PSRAM octal | Media | Medio | Validar pinout de la placa antes de cablear |
| iOS no empareja | Baja | Bajo | Usar iOS 13+; single connection |
| BLE HID en algunos Linux viejos | Baja | Bajo | Actualizar BlueZ; usar USB |
| Bluedroid no soporta reinicio limpio tras sleep | Media | Medio | Deinit/init del stack al despertar |

## Definición de "hecho" del proyecto

- [ ] Escribe correctamente por USB y BLE, sin dobles eventos.
- [ ] Calculadora standalone funcional.
- [ ] Autonomía real ≥ 1 semana de uso típico.
- [ ] Documentación y BOM finales publicados en este repo.
- [ ] (Opcional) Gabinete impreso y ensamblado.
