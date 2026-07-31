# 09 — Manual de usuario (Numpad-20)

> Manual orientado al usuario final del **Numpad-20**. Describe el comportamiento del
> firmware actual (v0.2, compilado) y, donde corresponde, marca con *(fase 2+)* las
> funciones previstas que aún no están implementadas.

---

## 1. Qué es

El Numpad-20 es un **teclado numérico auxiliar** (20 teclas) con:

- Conexión al host por **USB** (cable) y **Bluetooth BLE** (inalámbrico).
- **Pantalla OLED** que muestra estado, una **calculadora standalone** y un **menú de
  ajustes**.
- **Encoder rotativo** para volumen, navegación del menú y cambio de base de la
  calculadora.
- Batería recargable **18650** con carga por USB-C.

## 2. Primer uso

1. El dispositivo se enciende al **conectarlo por USB** o al **instalar la batería**
   (pila recargable cargada).
2. La pantalla muestra el estado: `BLE:- USB:Y 100%` (por ejemplo).
3. Listo para usar como teclado.

## 3. Conexión al host

### 3.1 Por USB (cable)

- Conectá el cable USB-C. El sistema operativo lo reconoce como **teclado** sin
  instalar nada (plug & play).
- Las teclas funcionan de inmediato.

### 3.2 Por Bluetooth (BLE)

1. Desde el host (PC, tablet o celular) abrí **Bluetooth → Agregar dispositivo**.
2. Buscá **`Numpad-20`** y conectate.
3. Si el host pide **confirmar un número** (numeric comparison), confirmá en ambos
   lados. En algunos sistemas se genera un vínculo (bonding) para que se reconecte
   solo la próxima vez.
4. Desconectado: el teclado vuelve a ser visible para reconexión automática.

> El Numpad puede estar conectado **a USB y BLE a la vez**; la misma tecla se envía a
> ambos hosts sin duplicarse.

## 4. Las teclas

El teclado tiene **una sola capa** (sin Fn):

```
┌──────┬─────┬─────┬──────┐
│ NumLk│  /  │  *  │  -   │
├──────┼─────┼─────┼──────┤
│  7   │  8  │  9  │  +   │
├──────┼─────┼─────┼──────┤
│  4   │  5  │  6  │  =   │
├──────┼─────┼─────┼──────┤
│  1   │  2  │  3  │ Ent  │
├──────┼─────┼─────┼──────┤
│  0   │  .  │CALC │BkSpc │
└──────┴─────┴─────┴──────┘
```

- **CALC** (inferior izquierda): abre y cierra la **calculadora**.
- **BkSpc** (inferior derecha): retroceso en el host; en la calculadora **borra el
  último dígito**.

## 5. Encoder rotativo (EC11)

| Acción | Modo teclado | Modo menú | Modo calculadora |
|---|---|---|---|
| **Girar** | Volumen | Navegar / ajustar valor | Volumen |
| **Presionar (SW)** | Abre el menú | Seleccionar / confirmar | Cambia base DEC → HEX → BIN |

> En el menú de ajustes (Pantalla → Encoder) podés **invertir el sentido** del giro.
> Si girás el encoder **rápido**, el paso es doble (el volumen o el menú avanzan de a dos).

## 6. Pantalla OLED

- **Modo teclado (HUD):** `BLE:Y USB:Y 87%` — conexión BLE, conexión USB y batería
  (el `+` marca que está cargando).
- **Modo calculadora:** `Calc DEC` + el valor; el SW del encoder cicla la base.
- **Modo menú:** lista de opciones; el cursor `>` indica la selección.

## 7. Calculadora

1. Abrí la calculadora con la tecla **CALC**.
2. Escribí con las teclas del numpad: dígitos, `+ − * /`, y `=` (o `Ent`) para el
   resultado.
3. **BkSpc** borra el último dígito.
4. Con el **botón del encoder** cambiás de base: DEC → HEX → BIN.
5. Con la tecla **NumLk** enviás el **resultado al host** (se tipía solo: dígitos, `-`, y
   A–F para HEX).
6. Cerrá la calculadora con **CALC** de nuevo (al reabrir, queda en cero).

> Cuando la calculadora está abierta, las teclas **no** se envían al host: solo
> alimentan el motor de cálculo.

## 8. Menú de ajustes

1. Presioná el **encoder** (en modo teclado) para abrir el menú.
2. **Girá** para mover el cursor, **presioná** para entrar/seleccionar.
3. Opciones:

| Ítem | Qué hace |
|---|---|
| **Info** | Datos del dispositivo: batería (mV y %), estado de carga, temperatura del chip, MAC BLE, versión de firmware y uptime. |
| **Pantalla → Contraste** | Brillo del OLED (0–255). Aplicá y confirmá con el SW. |
| **Encoder → Invertir** | Invierte el sentido del giro (No/Sí). |
| **Sleep → Timeout** | Apagado / 30s / 5min / 10min. Al cumplirse el tiempo sin uso, se **apaga la pantalla**; a los 5 min adicionales (sin USB ni BLE conectados) entra en **sleep** y se despierta con cualquier tecla, el pulsador del encoder o al conectar USB. |
| **Salir** | Vuelve al modo teclado. |

Los ajustes quedan **guardados** (se restauran al reiniciar).

## 9. Batería y carga

- **Indicador de carga:** mientras está cargando, la pantalla muestra `+` junto al
  porcentaje.
- **Carga:** conectá el USB-C. La placa carga la 18650 a través del módulo TP4056
  (indicador LED en la placa: rojo cargando / azul listo).
- **Porcentaje:** se muestra en la línea de estado. Está calibrado con una curva
  Li-Ion simple (ver `docs/05-alimentacion-y-bateria.md`).
- **Batería baja:** por debajo de 20 % el HUD muestra `BAT!`; por debajo de 10 % la
  línea de estado parpadea.
- **Sleep:** configurable en el menú (ver arriba). Con el timeout en "Apagado", la
  pantalla nunca se apaga sola.

> *(fase 2+)* Avisos más finos de carga, historial de batería y sleep con consumo medido.

## 10. Solución de problemas

| Problema | Solución |
|---|---|
| No aparece como teclado USB | Probá otro cable (debe ser de datos), otro puerto. Reflasheá el firmware. |
| No aparece como `Numpad-20` en BLE | Cerra la app de Bluetooth del host, olvidá el dispositivo anterior y volvé a buscar. |
| Las teclas no escriben | Verificá que la conexión esté activa (`BLE:Y` o `USB:Y` en pantalla). Si el menú o la calculadora están abiertos, las teclas no se envían al host: cerrálos. |
| El dispositivo no inicia | Batería descargada → conectalo a USB. |
| Se enciende pero no responde | Mantené `BOOT` presionado mientras reiniciás para entrar en modo de descarga (para flashear). |

## 11. Estado del proyecto

- **Firmware actual:** v0.3 compilado con éxito (ESP-IDF 5.4): sin capa Fn, menú de
  ajustes, sleep por inactividad y "pegar resultado". Pendiente de validación en
  hardware real.
- **Hardware:** aún no adquirido/ensamblado (ver lista de compra en `docs/02`).
- Detalles técnicos en [docs/02-hardware.md](02-hardware.md) y
  [docs/03-firmware.md](03-firmware.md).
