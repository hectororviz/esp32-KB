# 05 — Alimentación y Batería

> Conceptos y cálculos para alimentar el numpad desde una **18650 Li-Ion** de forma
> segura y eficiente.

## 1. Cadena de alimentación

```
USB-C (5 V)
   │
   ▼
┌─────────┐   BAT+   ┌────────┐         ┌───────────┐
│ TP4056  ├─────────►│ 18650  ├────────►│ LDO 3.3 V ├──► 3.3 V (ESP32-S3, OLED, etc.)
│ (carga) │          └────────┘         └───────────┘
└─────────┘
   │  CHRG / STDBY → GPIO (estado de carga)
   │  (con protección de batería, integrada o externa)
```

- El **USB-C** cumple doble rol: **cargar** (vía TP4056) y **comunicar** (D+/D− al S3).
- La **18650** alimenta todo el sistema vía el LDO de 3.3 V.
- Si no hay USB, el numpad funciona igual por BLE (modo inalámbrico).

## 2. Química Li-Ion (conceptos)

| Parámetro | Valor típico 18650 |
|---|---|
| Tensión nominal | 3.6-3.7 V |
| Tensión plena carga | 4.20 V |
| Tensión de corte | 2.8-3.0 V (protección) |
| Capacidad | 2000-3500 mAh |
| Rango útil para el firmware | ~3.3-4.2 V |

Reglas de oro:
- **Nunca** superar 4.2 V (riesgo de incendio) → el TP4056 lo corta solo.
- **Nunca** descargar por debajo de ~3.0 V (daño irreversible) → protección del pack o
  corte en firmware.
- Temperatura: no cargar bajo 0 °C ni sobre ~45 °C.

## 3. El módulo TP4056

- Cargador lineal por USB de **500 mA-1 A** (configurable con la resistencia de
  programa, típicamente 1.2 kΩ = 1 A).
- Indicadores de salida:
  - **`CHRG`**: activo (bajo) mientras carga.
  - **`STDBY`**: activo (bajo) cuando carga completa.
  - Ambos activos = batería ausente / defecto (comportamiento según versión).
- **Sin fuel gauge**: no reporta el porcentaje. El firmware lo estima por **voltaje**
  (ver §5).
- Algunas versiones incluyen **protección DW01** (sobre-descarga, sobre-corriente).
  Usar una con protección o una batería protegida.

## 4. El LDO de 3.3 V

| Opción | Caída (dropout) | Corriente | Eficiencia | Ideal |
|---|---|---|---|---|
| AMS1117-3.3 | ~1.1 V | 1 A | baja (lineal) | prototipos |
| HT7833 | ~0.3 V | 500 mA | media | buena opción general |
| XC6206-3.3 | ~0.1 V | 200-300 mA | media | consumo bajo |
| Buck (ej. MP1584) | — | 1+ A | alta (~90 %) | máxima autonomía |

- Con una 18650 en 3.6 V, un LDO lineal desperdicia ~ (3.6-3.3)/3.6 ≈ **8-16 %** según
  el estado de carga. Para un numpad con uso intermitente, un **LDO de bajo dropout**
  (HT7833/XC6206) es suficiente y mucho más simple.
- Un **buck** gana autonomía si el consumo promedio es alto (pantalla siempre prendida +
  BLE). Es un upgrade de fase 2 si se quiere.

## 5. Medición del nivel de batería (por voltaje)

El firmware lee el **voltaje de la batería** y lo convierte a porcentaje con una
**curva de descarga** (curva característica Li-Ion).

```
  4.20 V ─── 100 %
  4.00 V ─── ~90 %
  3.80 V ─── ~70 %
  3.60 V ─── ~45 %
  3.40 V ─── ~20 %
  3.30 V ─── ~10 %  (alerta)
  3.20 V ─── ~5 %   (crítico)
  3.00 V ─── 0 %    (corte)
```

### 5.1 Circuito divisor

```
 Batería (+) ──[ 100 kΩ ]──┬──[ 100 kΩ ]── GND
                           │
                         GPIO1 (ADC1_CH0)
```

- Factor de división: 1/2 → 4.2 V/2 = **2.1 V** máximo en el ADC (dentro del rango
  seguro del ADC del S3, 0-2.5 V recomendado con atenuación de 11 dB).
- Elegir **ADC1** (no ADC2): el ADC2 comparte hardware con WiFi/BLE y da lecturas
  inconsistentes cuando el radio está activo.
- **Calibración**: el ADC del ESP32-S3 es **no lineal**. Calibrar con 2-3 puntos
  (medir con multímetro y ajustar offset/ganancia) o usar la curva de calibración de
  fábrica (`esp_adc_cal`).

### 5.2 Errores a evitar

- **Medir mientras carga**: la tensión está inflada por la corriente de carga. Leer
  solo cuando `CHRG` está inactivo, o promediar varias muestras y aplicar una tabla
  de compensación.
- **Voltaje ≠ porcentaje bajo carga**: la curva se usa **en reposo** (sin BLE enviando
  y sin carga). Bajo uso intenso, muestrear en los momentos de inactividad.
- **Ruido**: promediar 16-32 muestras ADC y filtrar con media móvil.

## 6. Consumo estimado y autonomía

| Escenario | Corriente típica |
|---|---|
| Light sleep (pantalla off, BLE idle) | ~0.1-1 mA |
| Pantalla on, BLE conectado, idle | ~25-40 mA |
| BLE enviando + matriz + pantalla | ~50-80 mA |
| Pico (radio BLE TX) | ~150-350 mA (muy breve) |

**Autonomía estimada con 18650 de 3000 mAh:**

| Uso | Corriente media | Autonomía |
|---|---|---|
| Uso intermitente (sleep la mayor parte) | ~5 mA | **~25 días** |
| Uso intenso continuo (pantalla + BLE) | ~60 mA | **~50 h** |

Conclusión: la 18650 da una **autonomía muy holgada**; el sleep es más un capricho de
eficiencia que una necesidad, pero igual se implementa (ver `03-firmware.md`).

## 7. Estado de carga y detección de USB

| Señal | Origen | GPIO | Lógica |
|---|---|---|---|
| `CHRG` | TP4056 | 2 | Bajo = cargando |
| `STDBY` | TP4056 | 39 | Bajo = carga completa (GPIO3 es strapping; se usa 39) |
| VBUS | USB-C 5 V (vía divisor) | 18 | Alto = cable conectado a algo |

Combinaciones útiles:

| VBUS | CHRG | STDBY | Significado |
|---|---|---|---|
| 0 | — | — | Solo batería (modo BLE) |
| 1 | 1 | 1 | USB conectado, batería llena |
| 1 | 0 | 1 | USB conectado, cargando |
| 1 | 1 | 0 | USB conectado, batería llena |

El firmware muestra el estado en el HUD y puede **apagar la pantalla al cargar a plena**
para ahorrar.

## 8. Protecciones y seguridad

- [ ] Batería con protección o TP4056 con DW01.
- [ ] Fusible opcional (500 mA-1 A) en serie con la batería.
- [ ] Conector de batería con polaridad marcada (o JST reverso).
- [ ] Verificar que el LDO soporte el pico de corriente del BLE sin caerse.
- [ ] No usar la placa con la 18650 suelta sin protección (riesgo de corto).

## 9. Optimizaciones de energía (firmware)

- **Pantalla**: apagar después de 30 s de inactividad; despertar con tecla/encoder.
- **BLE**: usar parámetros de conexión "lentos" (intervalo de conexión 100-200 ms) en
  idle y "rápidos" (20-40 ms) solo al tipear — patrón que ya usa el repo de referencia.
- **CPU**: `CONFIG_PM_ENABLE=y` (Dynamic Frequency Scaling), bajar frecuencia en idle.
- **RF**: desactivar el advertising cuando ya hay conexión (o pasar a slow-advertise).
