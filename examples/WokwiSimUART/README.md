# Wokwi simulation backend (UART + custom chip)

This example demonstrates the **simulation backend** (`HUB75_WOKWI_SIM`) for ESP32-HUB75-MatrixPanel-DMA.

## Why this exists

The library's default backend drives HUB75 panels using ESP32 DMA + I2S/LCD peripherals. Wokwi does not model that low-level DMA scanout path for this library, so this example uses an explicit simulation backend that:

1. Keeps the normal `MatrixPanel_I2S_DMA` drawing API.
2. Stores RGB pixels in a software framebuffer.
3. Streams frames over UART1 to a Wokwi custom chip.
4. The custom chip renders pixels via Wokwi's framebuffer API.

## Build flags

Enable simulation mode:

```ini
build_flags =
  -DHUB75_WOKWI_SIM=1
```

Without this flag, the library remains on the original hardware DMA backend.

## Files

- `WokwiSimUART.ino`: demo sketch
- `diagram.json`: ESP32-S3 + custom chip wiring
- `wokwi.toml`: custom chip registration
- `chips/hub75-sim.chip.json`: custom chip pinout + framebuffer control
- `chips/hub75-sim.chip.c`: custom chip implementation

## Build custom chip WASM

From this folder, compile the chip (requires `wokwi-cli` >= 0.20):

```bash
wokwi-cli chip compile chips/hub75-sim.chip.c -o chips/hub75-sim.chip.wasm
```

Then run the project in VS Code Wokwi extension or Wokwi CLI.

## Switching back to real hardware

- Remove `-DHUB75_WOKWI_SIM=1` from build flags.
- Use normal HUB75 GPIO mapping and real panel wiring.
- No sketch drawing API changes are required.
