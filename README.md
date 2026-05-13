# CAN CoreX Examples

This repository contains example integrations for the CAN CoreX C library.

The library sources and library documentation are in `can_corex/`. Board examples are in `stm32_hal/`.

## Boards

### NUCLEO-G474RE

- CAN1:
  - RX: `PB8`
  - TX: `PB9`
  - nominal bitrate: `500 kbit/s`
  - data bitrate: `2 Mbit/s` in CAN FD examples
- CAN2:
  - RX: `PB5`
  - TX: `PB6`
  - nominal bitrate: `500 kbit/s`
  - data bitrate: `2 Mbit/s` in CAN FD examples
- UART:
  - peripheral: `USART2`
  - baudrate: `115200`

## Examples

| Example | Description |
| --- | --- |
| `nucleo_g474re_classic_rxtx` | Classic CAN RX/TX, callbacks, timeout, unregistered frame. |
| `nucleo_g474re_classic_isotp` | ISO-TP over classic CAN, two sessions. |
| `nucleo_g474re_fd_rxtx` | CAN FD RX/TX, classic/FD/FD+BRS frames. |
| `nucleo_g474re_fd_isotp` | ISO-TP over CAN FD, FD+BRS, large payload. |
| `nucleo_g474re_fd_bus_monitoring` | CAN FD with bus monitoring, error counters, recovery logs. |
