# 04 — Catálogo de Funciones e Ideas

> Todas las funciones que se pueden sumar al numpad, **priorizadas** para decidir
> alcance. La idea es construir primero el núcleo (P0) y agregar el resto de forma
> incremental (P1, P2).

## Prioridades

- **P0 — Núcleo**: sin esto el proyecto no es un teclado. Se construye primero.
- **P1 — Alto valor**: poco esfuerzo, gran impacto de uso.
- **P2 — Extras**: gadget divertidos o de nicho, para después.

---

## Decisiones de alcance (v0.2 — re-scope)

En la v0.2 se **descartaron** estas ideas (no se planifican):

- ~~Capa Fn~~ (y con ella: media keys, atajos de productividad, multi-host por Fn).
- ~~Macros / snippets de texto~~.
- ~~Tap-hold~~.
- ~~Memoria M+/M−/MR/MC~~ e ~~historial~~ en la calculadora.

Se **agregó** en su lugar:

- **Menú de ajustes en pantalla** (modo MENÚ) operado con el encoder: `Info`,
  `Pantalla` (contraste), `Encoder` (invertir), `Sleep` (timeout). Ver `docs/03`.

El resto del catálogo queda como referencia a futuro.

---

## P0 — Núcleo

### 1. Teclado numérico HID (USB + BLE)
- Los 20 keycodes del numpad viajan por USB y/o BLE.
- Compatibilidad plug-and-play con cualquier OS.
- **Criterio**: escribir dígitos en un editor por cable y por BT, sin dobles eventos.

### 2. Capa Fn
- Mantener `Fn` activa una segunda capa con funciones encima de las mismas teclas.
- Es la base de casi todas las demás features.

### 3. Calculadora standalone
- **Tu idea original** y el "asesino" del proyecto. Funciona sin host.
- Expresión + resultado en pantalla con dígitos grandes.
- Detalle: togglear con una tecla (`CALC`) que **desacopla** las teclas del HID y las
  redirige al motor de cálculo.

### 4. Pantalla de estado (HUD)
- Batería (%), modo de conexión (USB / BLE / ambos), última tecla enviada.
- Permite debug visual del firmware en desarrollo.

### 5. Gestión de energía (sleep/wake)
- Auto-apagado de pantalla y light-sleep para estirar la 18650.
- Wake instantáneo por tecla/encoder.

### 6. Macros / snippets de texto
- Tecla única → envía una secuencia de teclas (emails, texto repetitivo, unicode).
- Configurables en una tabla en el firmware (y a futuro, desde un menú en pantalla).

---

## P1 — Alto valor

### 7. Encoder EC11 multifunción
- **Volumen** (modo normal), **scroll** (capa Fn), **zoom**, **navegación de menú**,
  **scrub de números** en la calculadora.
- Giro rápido = pasos grandes; giro lento = pasos finos (detección de velocidad).

### 8. Tap-hold en teclas clave
- `.` al tocar / `,` al mantener (separador decimal por región).
- `Enter` al tocar / `Shift` al mantener.
- `=` al tocar / `NumLock` al mantener.

### 9. Media keys
- Play/pause, next/prev, mute, vol+/- en la capa Fn.

### 10. Cambio de dispositivo (multi-host)
- Emparejar PC + tablet + teléfono y alternar con `Fn` + tecla (ej. `Fn`+`1/2/3`).
- Reutiliza el bonding BLE ya establecido.

### 11. "Pegar resultado" desde la calculadora
- Tras calcular, una tecla envía el **resultado por HID** a la PC (como si lo tipiaras).
- Ideal para planillas. Cierra el círculo calculadora ↔ host.

### 12. Atajos de productividad (capa Fn)
- `Ctrl+C/X/V/Z`, `Ctrl+S`, `Alt+Tab`, `Win+Shift+S` (screenshot), `F2` (renombrar).
- Perfil Excel: `Ctrl+1` (formato de celdas), `Ctrl+Shift+$` (moneda), etc.

---

## P2 — Extras / gadget

### 13. Modo mouse (presentaciones)
- Encoder = scroll, teclas = clic izquierdo/derecho, capa Fn = flechas.
- Útil para dar charlas con el pad a mano.

### 14. Reloj / stopwatch / pomodoro en la pantalla
- El OLED puede mostrar hora y temporizadores cuando no se usa el pad.

### 15. Underglow RGB (WS2812)
- Tira de LEDs debajo del gabinete, con efectos.
- Uso funcional: color = estado (verde = BLE ok, rojo = batería baja).

### 16. Buzzer
- Feedback acústico de teclas (toggleable), "beep" en calculadora y alarma de batería
  baja.

### 17. Conversor de unidades en la calculadora
- Longitud, peso, temperatura, moneda (tasa fija offline).
- Se selecciona con el encoder en la pantalla.

### 18. Entrada unicode / caracteres especiales
- Modo donde tecleás el código y se envía el carácter (ñ, á, €, emoji vía atajo).

### 19. App móvil de configuración (fase lejana)
- Como el repo de referencia `paul356/esp32_keyboard`: una app Android/BLE para editar
  el keymap y las macros sin re-flashear.
- El S3 ya tiene WiFi: también se podría hacer un **panel web** de configuración
  (mDNS + HTTP).

### 20. OTA (actualización por aire)
- Subir firmware nuevo por WiFi/BLE desde el menú. Requiere partición OTA.

---

## Matriz de impacto

| # | Función | Valor | Esfuerzo | Prioridad |
|---|---|---|---|---|
| 1 | Numpad HID USB+BLE | crítico | medio | P0 |
| 2 | ~~Capa Fn~~ (descartada) | — | — | — |
| 3 | Calculadora standalone | alto | medio | P0 |
| 4 | HUD de estado | medio | bajo | P0 |
| 5 | Sleep/wake | alto | bajo | P0 |
| 6 | ~~Macros de texto~~ (descartada) | — | — | — |
| 7 | Encoder multifunción | alto | medio | P1 |
| 7b | **Menú de ajustes en pantalla** | alto | medio | **P0 (implementado v0.2)** |
| 8 | ~~Tap-hold~~ (descartado) | — | — | — |
| 9 | ~~Media keys~~ (descartadas) | — | — | — |
| 10 | ~~Multi-host~~ (descartado) | — | — | — |
| 11 | Pegar resultado | alto | bajo | P1 |
| 12 | Atajos productividad | medio | bajo | P1 |
| 13 | Modo mouse | bajo | bajo | P2 |
| 14 | Reloj/stopwatch | bajo | bajo | P2 |
| 15 | RGB underglow | bajo | medio | P2 |
| 16 | Buzzer | bajo | bajo | P2 |
| 17 | Conversor de unidades | medio | medio | P2 |
| 18 | Unicode | bajo | bajo | P2 |
| 19 | App/panel de config | medio | alto | P2 |
| 20 | OTA | medio | alto | P2 |

---

## Sugerencia de alcance v1

**P0 completo + Encoder + Menú de ajustes + Pegar resultado (opcional).** Eso ya entrega
un numpad profesional, inalámbrico, con calculadora útil y ajustes en pantalla.
El resto se va sumando sin romper el núcleo.
