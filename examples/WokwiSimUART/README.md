# Wokwi simulation backend (VS Code / CLI project)

This example demonstrates the Wokwi simulation backend for ESP32-HUB75-MatrixPanel-DMA using the local VS Code / CLI flow.

## Why this exists

The library's default backend drives HUB75 panels using ESP32 DMA + I2S/LCD peripherals. Wokwi does not model that low-level DMA scanout path for this library, so this example uses an explicit simulation backend that:

1. Keeps the normal `MatrixPanel_I2S_DMA` drawing API.
2. Stores RGB pixels in a software framebuffer.
3. Streams frames over UART1 to a Wokwi custom chip.
4. The custom chip renders pixels via Wokwi's framebuffer API.

## Enable

This example enables simulation in the sketch config:

```cpp
cfg.use_wokwi_sim = true;
cfg.wokwi_uart_tx_pin = 17;
```

The legacy `-DHUB75_WOKWI_SIM=1` build flag still works as a default, but it is no longer required.

## Files

- `WokwiSimUART.ino`: demo sketch
- `diagram.json`: ESP32-S3 + custom chip wiring
- `wokwi.toml`: custom chip registration
- `chips/hub75-sim.chip.json`: custom chip pinout + framebuffer control
- `chips/hub75-sim.chip.c`: custom chip implementation

## Build custom chip WASM

From this folder, build the chip with `make` (requires `wokwi-cli` >= 0.20 once to install WASI-SDK, or an existing `~/.wokwi/wasi-sdk`):

```bash
make
```

This example uses explicit WASM linker memory settings in the generated `Makefile`, because the default `wokwi-cli chip compile ...` output can request a larger imported memory size than the Wokwi runtime provides for custom chips.

## Build firmware

Build the firmware with PlatformIO:

```bash
~/.platformio/penv/bin/pio run
```

Or build both the chip and firmware together:

```bash
make sim-ready
```

## Run in Wokwi

1. Run `make` after every custom chip change.
2. Run `~/.platformio/penv/bin/pio run` after every firmware change, or just run `make sim-ready`.
3. Start the project from the VS Code Wokwi extension or Wokwi CLI.

If Wokwi still appears to use an old chip binary, stop the simulation completely and reload the VS Code window before starting it again.

## Switching back to real hardware

- Set `cfg.use_wokwi_sim = false` or remove the sim config from the sketch.
- Use normal HUB75 GPIO mapping and real panel wiring.
- No sketch drawing API changes are required.
