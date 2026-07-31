# 09 — Manual de usuario (Numpad-20)

> Manual orientado al usuario final del **Numpad-20**. Describe el comportamiento del
> firmware actual (primera versión compilada) y, donde corresponde, marca con
> *(fase 2+)* las funciones previstas que aún no están implementadas.

---

## 1. Qué es

El Numpad-20 es un **teclado numérico auxiliar** (20 teclas) con:

- Conexión al host por **USB** (cable) y **Bluetooth BLE** (inalámbrico).
- **Pantalla OLED** que muestra estado y una **calculadora standalone**.
- **Encoder rotativo** para volumen y para cambiar la base de la calculadora.
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

### 4.1 Capa base (numpad)

```
┌──────┬─────┬─────┬─────┐
│ NumLk│  /  │  *  │  -  │
├──────┼─────┼─────┼─────┤
│  7   │  8  │  9  │  +  │
├──────┼─────┼─────┼─────┤
│  4   │  5  │  6  │  =  │
├──────┼─────┼─────┼─────┤
│  1   │  2  │  3  │ Ent │
├──────┼─────┼─────┼─────┤
│  0   │  .  │ Fn  │ Esc │
└──────┴─────┴─────┴─────┘
```

### 4.2 Capa Fn (mantené **Fn** presionada y pulsá la tecla)

| Tecla | Función (capa Fn) |
|---|---|
| NumLk | Home (acceso rápido) |
| `/` | Anterior pista |
| `*` | Siguiente pista |
| `-` | Silenciar |
| `7` | Subir volumen |
| `8` | Reproducir / Pausar |
| `9` | Detener |
| `+` | Bajar volumen |
| `4` `5` `6` `=` | Flechas ← ↓ ↑ → |
| `1` `2` `3` | Inicio · Fin · Re Pág |
| `Ent` | Av Pág |
| `0` | Tabulador |
| `.` | Punto |
| `Esc` | **Abrir/cerrar calculadora** |

## 5. Encoder rotativo (EC11)

| Acción | Función |
|---|---|
| Girar | Subir / bajar **volumen** |
| Presionar | En calculadora: cambia el modo **DEC → HEX → BIN** |

## 6. Pantalla OLED

- **Línea 1 (estado):** `BLE:Y USB:Y DEC +87%` — indica conexión BLE, conexión USB,
  modo de calculadora y batería (el `+` marca que está cargando).
- **Línea 2 (principal):** valor de la calculadora, o el nombre del dispositivo /
  indicador de capa Fn cuando la calculadora está cerrada.

## 7. Calculadora

1. Abrí la calculadora con **Fn + Esc** (tecla de la esquina inferior derecha).
2. Escribí con las teclas del numpad: dígitos, `.`, `+ − * /`, y `=` (o `Ent`) para
   el resultado.
3. `Esc` borra todo (sin Fn).
4. Con el **botón del encoder** cambiás de base: DEC → HEX → BIN.
5. Cerrá la calculadora con **Fn + Esc** de nuevo.

> *(fase 2+)* Memoria M+/M−/MR/MC, historial con el encoder y envío del resultado al
> host (pegado automático).

## 8. Batería y carga

- **Indicador de carga:** mientras está cargando, la pantalla muestra `+` junto al
  porcentaje.
- **Carga:** conectá el USB-C. La placa carga la 18650 a través del módulo TP4056
  (indicador LED en la placa: rojo cargando / azul listo).
- **Porcentaje:** se muestra en la línea de estado. Está calibrado con una curva
  Li-Ion simple (ver `docs/05-alimentacion-y-bateria.md`).

> *(fase 2+)* Aviso de batería baja (<20 %), apagado de pantalla por inactividad y
> sleep profundo.

## 9. Solución de problemas

| Problema | Solución |
|---|---|
| No aparece como teclado USB | Probá otro cable (debe ser de datos), otro puerto. Reflasheá el firmware. |
| No aparece como `Numpad-20` en BLE | Cerra la app de Bluetooth del host, olvidá el dispositivo anterior y volvé a buscar. |
| Las teclas no escriben | Verificá que la conexión esté activa (`BLE:Y` o `USB:Y` en pantalla). |
| El dispositivo no inicia | Batería descargada → conectalo a USB. |
| Se enciende pero no responde | Mantené `BOOT` presionado mientras reiniciás para entrar en modo de descarga (para flashear). |

## 10. Estado del proyecto

- **Firmware actual:** primera versión compilada con éxito (ESP-IDF 5.4). Pendiente de
  validación en hardware real.
- **Hardware:** aún no adquirido/ensamblado.
- Detalles técnicos en [docs/02-hardware.md](02-hardware.md) y
  [docs/03-firmware.md](03-firmware.md).
