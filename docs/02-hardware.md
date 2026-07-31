# 02 — Hardware

> Documento de referencia para comprar componentes, cablear el prototipo y (a futuro)
> diseñar la PCB. Los GPIO propuestos son para una placa de desarrollo típica; **validá
> el pinout de tu placa** (PSRAM octal, strapping) antes de cablear.

## 1. Lista de materiales (BOM)

| # | Componente | Cant. | Notas / valor sugerido |
|---|---|---|---|
| 1 | Placa de desarrollo ESP32-S3 | 1 | Con USB nativo (ej. ESP32-S3-DevKitC-1, Waveshare S3-Zero). Que tenga USB-C. |
| 2 | Switchs mecánicos MX (o Choc) | 20 | Kailh/Gateron/Cherry, con sus keycaps |
| 3 | Diodos 1N4148 | 20 | Uno por tecla (anti-ghosting / anti-rollover) |
| 4 | Encoder rotativo EC11 | 1 | Con pulsador integrado, con perilla |
| 5 | Pantalla OLED 1.3" I2C | 1 | Controlador SH1106 (compatible SSD1306), 128×64 |
| 6 | Batería 18650 | 1 | Con protección (o módulo BMS aparte) |
| 7 | Módulo TP4056 (con protección) | 1 | Cargador USB Li-Ion, corriente configurable (0.5-1 A) |
| 8 | Regulador 3.3 V LDO | 1 | Ej. HT7833, XC6206-3.3, AMS1117-3.3 |
| 9 | Resistencias 100 kΩ | 2 | Divisor de tensión para ADC de batería |
| 10 | Resistencias 10 kΩ | 1-2 | Divider VBUS / pulldowns si hace falta |
| 11 | Condensadores 0.1 µF / 10 µF | varios | Desacople de alimentación |
| 12 | Cables dupont / protoboard | — | Prototipado |
| 13 | Interruptor deslizante | 1 | (Opcional) corte físico de alimentación |

### Opcionales (fases 2+)
- Buzzer pasivo (feedback de teclas, calculadora).
- Tira WS2812 (underglow RGB).
- Bloque de pines / placa perforada para armar la matriz.

## 2. Layout de teclas (20 teclas, todo 1×1)

```
┌────────┬──────┬──────┬──────┐
│ NumLk  │  /   │  *   │  -   │   Fila 1
├────────┼──────┼──────┼──────┤
│   7    │  8   │  9   │  +   │   Fila 2
├────────┼──────┼──────┼──────┤
│   4    │  5   │  6   │  =   │   Fila 3
├────────┼──────┼──────┼──────┤
│   1    │  2   │  3   │ Ent  │   Fila 4
├────────┼──────┼──────┼──────┤
│   0    │  .   │ CALC │BkSpc │   Fila 5
└────────┴──────┴──────┴──────┘
```

- La columna de la derecha es ideal para el **encoder** si se quiere integrado en el
  gabinete (o como tecla adicional).
- `CALC` (fila 5, col 2) abre/cierra la **calculadora**; `BkSpc` es retroceso (en la
  calculadora borra el último dígito). No hay capa Fn.

## 3. Matriz de teclas

Principio: en vez de 20 GPIO (uno por tecla), se usan **5 filas + 4 columnas = 9 GPIO**.
Cada tecla conecta una fila y una columna a través de un **diodo**.

```
   C0    C1    C2    C3          (entradas, con pull-up interno)
  ┌─┴┐  ┌─┴┐  ┌─┴┐  ┌─┴┐
R0┤▷├──┤▷├──┤▷├──┤▷├──            (filas = salidas push-pull)
R1┤▷├──┤▷├──┤▷├──┤▷├──
R2┤▷├──┤▷├──┤▷├──┤▷├──
R3┤▷├──┤▷├──┤▷├──┤▷├──
R4┤▷├──┤▷├──┤▷├──┤▷├──
```

Algoritmo de escaneo (en `03-firmware.md`):
1. Poner **todas las filas** en alta.
2. Poner **una fila** en baja.
3. Leer las 4 columnas: si una está en baja, esa tecla está presionada.
4. Repetir con la siguiente fila. Ciclo completo cada ~10 ms.

Los **diodos** evitan el "ghosting": que una tecla no presionada parezca presionada por
retorno de corriente. Son imprescindibles para **rollover confiable** (presionar varias
teclas a la vez).

## 4. Pinout propuesto (ESP32-S3)

> ⚠️ Reglas del S3 a respetar:
> - **GPIO19/20 = USB D−/D+**: NO usar (van al conector USB-C).
> - **Strapping**: GPIO0 (boot), GPIO45 (voltaje de flash), GPIO46 (ROM msg): evitar o
>   tratar con cuidado.
> - **GPIO33-37** suelen estar ocupados por **flash/PSRAM octal** en placas con PSRAM.
> - ADC1 = GPIO1..GPIO10 (el ADC2 comparte hardware con WiFi → usar ADC1 para batería).

| Función | GPIO(s) | Observación |
|---|---|---|
| Filas matriz R0-R4 | 6, 7, 8, 9, 10 | Salidas push-pull (GPIO9/10 son ADC1, OK como GPIO) |
| Columnas matriz C0-C3 | 11, 12, 13, 14 | Entradas con pull-up interno |
| Encoder A / B / SW | 15 / 16 / 17 | Interrupciones GPIO, pulsador a GND |
| OLED SDA / SCL | 4 / 5 | I2C, pull-ups externos 4.7 kΩ |
| Batería (ADC) | 1 | ADC1_CH0, vía divisor 100k/100k |
| Carga activa (TP4056 `CHRG`) | 2 | Entrada digital (activo bajo) |
| Carga completa (TP4056 `STDBY`) | 3 | Entrada digital (activo bajo) |
| VBUS detect (5 V del USB) | 18 | Vía divisor, entrada digital |

**Total GPIO usados: 21** (matriz 9 + encoder 3 + I2C 2 + batería 1 + carga 2 + VBUS 1 +
USB fijos 2). Si la placa no tiene PSRAM octal, sobran GPIO33-37 para expandir.

### 4.1 Conexiones de alimentación

```
USB-C ──► TP4056 (IN)      USB-C D+/D- ──► GPIO19/20 (datos HID)
          TP4056 (BAT+) ──► 18650 (+)
          TP4056 (BAT-) ──► GND
          TP4056 (BAT+) ──► LDO 3.3V IN ──► 3.3V a todo el sistema
          TP4056 CHRG  ──► GPIO2
          TP4056 STDBY ──► GPIO3
Batería (+) ──► divisor 100k/100k ──► GPIO1 (ADC)
```

Consideraciones:
- El **TP4056 regula la carga**, pero su salida es la **tensión de la batería**
  (2.8-4.2 V), no 3.3 V. Por eso hace falta el **LDO 3.3 V** después.
- Usar batería **con protección** (o un TP4056 con protección incorporada): evita
  sobre-descarga y cortocircuito.
- Los **datos USB** van directos al S3; el S3 los maneja como dispositivo HID. El mismo
  conector USB-C que carga, también comunica.
- Añadir condensador de desacople de 10-100 µF cerca del LDO (picos de corriente del
  radio BLE).

## 5. Pantalla OLED

- **SH1106** (1.3") o **SSD1306** (0.96"): drivers casi idénticos a nivel software.
- Bus **I2C** (SDA/SCL) + 3.3 V + GND. Son los típicos módulos de 4 pines.
- El firmware usa dígitos grandes (fuente ~24-32 px) para la calculadora y un modo
  compacto para el HUD de estado.

## 6. Encoder EC11

- Pines **A y B** (cuadratura) → GPIO15/16 con interrupciones.
- Pulsador integrado **SW** → GPIO17 (a GND, con pull-up interno).
- Usos: volumen, scroll, zoom, scrub de números, navegación de menú en pantalla.

## 7. Diseño mecánico (ideas para el gabinete)

- **Placa inferior + placa superior**: 3D print (PLA) o corte láser de acrílico.
- Layout de switchs estándar MX: separación 19.05 mm (0.75").
- El encoder se monta en la columna derecha (reemplazando o junto a `=`).
- Ranura para la 18650 + acceso al USB-C y al interruptor de alimentación.
- Ángulo de inclinación leve para ergonomía (pies plegables).

## 8. Checklist de compra

- [ ] Placa ESP32-S3 con USB nativo (verificar que tenga los GPIO del pinout libre).
- [ ] 20 switchs + 20 keycaps.
- [ ] 20 diodos 1N4148.
- [ ] Encoder EC11 con perilla.
- [ ] OLED 1.3" I2C.
- [ ] 18650 (con protección) + módulo TP4056.
- [ ] LDO 3.3 V + capacitores.
- [ ] Resistencias 100k/100k (ADC), 4.7k (I2C).
- [ ] Protoboard / PCB / placa perforada + cables.
