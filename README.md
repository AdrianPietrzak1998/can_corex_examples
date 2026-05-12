# CAN CoreX Examples

This repository contains example integrations for the CAN CoreX C library.

The library itself is in `can_corex/`. Board and vendor examples are kept outside
the library so they can be copied, modified, and compared without changing the
portable CAN CoreX sources.

## Repository Layout

- `can_corex/` - CAN CoreX library sources and library documentation.
- `stm32_hal/` - STM32 HAL examples.
- `stm32_hal/nucleo_g474re_classic_rxtx/` - first STM32G474RE Nucleo example for classic CAN on two FDCAN instances.

## Current STM32 Example

`stm32_hal/nucleo_g474re_classic_rxtx/` is the base project for NUCLEO-G474RE.
It uses `FDCAN1` and `FDCAN2` in classic CAN mode and assumes both CAN channels
are physically connected through CAN transceivers.

The application layer is intentionally kept in:

```text
Core/can_app/
```

The public application API is only:

```c
void can_app_init(void);
void can_app_poll(void);
```

`can_app` demonstrates:

- two independent `CCX_instance_t` instances, one for each FDCAN peripheral
- periodic TX tables
- RX parser callbacks
- timeout callback on an expected-but-missing RX frame
- standard 11-bit identifiers
- extended 29-bit identifiers
- several DLC values, not only 8-byte frames
- an intentionally unregistered frame to show the unregistered RX callback

The current traffic is:

| Direction | ID | IDE | DLC | Meaning |
| --- | ---: | --- | ---: | --- |
| CAN1 -> CAN2 | `0x101` | standard | 1 | counter |
| CAN1 -> CAN2 | `0x18FF0101` | extended | 2 | speed |
| CAN1 -> CAN2 | `0x555` | standard | 5 | unregistered demo frame |
| CAN2 -> CAN1 | `0x201` | standard | 2 | heartbeat |
| CAN2 -> CAN1 | `0x18FF0202` | extended | 4 | temperature and humidity |
| expected by CAN1 | `0x333` | standard | 1 | missing counter, used for timeout demo |

## Suggested NUCLEO-G474RE Example Projects

Use `nucleo_g474re_classic_rxtx` as the template and copy it for the next
examples. Keep each copy focused on one feature so users can inspect the diff
and understand the integration quickly.

Recommended next projects:

1. `nucleo_g474re_classic_rxtx`
   Basic classic CAN RX/TX, callbacks, several DLC values, extended IDs,
   timeout, and unregistered frame handling.

2. `nucleo_g474re_isotp_classic`
   ISO-TP over classic CAN: single frame, multi-frame TX/RX, flow control, and
   a simple request/response payload.

3. `nucleo_g474re_canfd_rxtx`
   CAN FD mode with `CCX_ENABLE_CANFD=1`: larger payloads, FD DLC mapping,
   classic-vs-FD frame format handling, and optional BRS.

4. `nucleo_g474re_canfd_isotp`
   ISO-TP over CAN FD with larger payloads and FD-specific frame lengths.

5. `nucleo_g474re_bus_monitor`
   CAN CoreX bus monitoring and statistics: bus-off/error handling, counters,
   and recovery path.

6. `nucleo_g474re_network_replication`
   CAN CoreX network replication helpers, useful for mirroring or simulating
   traffic between logical CAN nodes.

## Development Notes

Do not add a shared build system just for these examples unless the repository
starts needing it. STM32CubeIDE projects should remain easy to open and copy.

When adding a new STM32 example:

- copy an existing `stm32_hal/nucleo_g474re_*` project
- keep feature code under `Core/can_app/`
- keep `main.c` limited to HAL init plus `can_app_init()` and `can_app_poll()`
- document the frame IDs, DLC, direction, and feature being demonstrated
- prefer small, readable examples over one large project that demonstrates everything
