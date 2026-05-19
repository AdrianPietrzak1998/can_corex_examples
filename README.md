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
- Timer:
  - `TIM17` is used as the 1 MHz high-resolution tick source in ISO-TP and bus monitoring examples

### NUCLEO-G491RE

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
- Timer:
  - `TIM17` is used as the 1 MHz high-resolution tick source in ISO-TP and bus monitoring examples

### NUCLEO-F767ZI

- CAN1:
  - RX: `PD0`
  - TX: `PD1`
  - nominal bitrate: `500 kbit/s`
- CAN2:
  - RX: `PB5`
  - TX: `PB6`
  - nominal bitrate: `500 kbit/s`
- UART:
  - peripheral: `USART3`
  - baudrate: `115200`
- Timer:
  - `TIM13` is used as the 1 MHz high-resolution tick source in ISO-TP and bus monitoring examples

## Examples

### STM32 HAL

G4 examples use the STM32 FDCAN HAL. F7 examples use the STM32 bxCAN HAL.

| Example | Description |
| --- | --- |
| `nucleo_g474re_classic_rxtx` | Classic CAN RX/TX, callbacks, timeout, unregistered frame. |
| `nucleo_g474re_classic_isotp` | ISO-TP over classic CAN, two sessions. |
| `nucleo_g474re_fd_rxtx` | CAN FD RX/TX, classic/FD/FD+BRS frames. |
| `nucleo_g474re_fd_isotp` | ISO-TP over CAN FD, FD+BRS, large payload. |
| `nucleo_g474re_fd_bus_monitoring` | CAN FD with bus monitoring, error counters, recovery logs. |
| `nucleo_g491re_classic_rxtx` | Classic CAN RX/TX, callbacks, timeout, unregistered frame. |
| `nucleo_g491re_classic_isotp` | ISO-TP over classic CAN, two sessions. |
| `nucleo_g491re_fd_rxtx` | CAN FD RX/TX, classic/FD/FD+BRS frames. |
| `nucleo_g491re_fd_isotp` | ISO-TP over CAN FD, FD+BRS, large payload. |
| `nucleo_g491re_fd_bus_monitoring` | CAN FD with bus monitoring, error counters, recovery logs. |
| `nucleo_f767zi_classic_rxtx` | bxCAN classic CAN RX/TX, callbacks, timeout, unregistered frame. |
| `nucleo_f767zi_classic_isotp` | ISO-TP over bxCAN classic CAN, two sessions, high-resolution STmin timing. |
| `nucleo_f767zi_classic_bus_monitoring` | bxCAN classic CAN with software bus monitoring, ESR error counters, recovery logs. |
| `nucleo_f767zi_freertos_lwip_classic_slcan_tcp_server` | bxCAN classic CAN bridged to SLCAN over TCP using FreeRTOS and LwIP. |

## SLCAN TCP Server Example

`nucleo_f767zi_freertos_lwip_classic_slcan_tcp_server` exposes virtual CAN CoreX instances over TCP while keeping the physical bxCAN buses active.

- `CAN1` is bridged to `can1_eth` on TCP port `7001`.
- `CAN2` is bridged to `can2_eth` on TCP port `7002`.
- The board acts as the TCP server.
- The TCP payload format is classic SLCAN text frames, for example `t3011AA\r`.
- Frames received from a physical CAN bus are forwarded to the matching TCP client.
- Frames transmitted by the local CAN CoreX instance are also forwarded to the matching TCP client.
- Frames received from TCP are injected into the virtual instance and forwarded to the matching physical CAN bus.

The virtual `can*_eth` instances do not use a physical CAN peripheral. They are connected to the physical CAN instances through `can_corex_net`.
