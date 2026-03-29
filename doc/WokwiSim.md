# Wokwi simulation backend

## Scope and intent

This mode is a **simulation backend** for Wokwi. It is intentionally separate from the real HUB75 DMA backend.

- **Real hardware path:** ESP32 DMA + I2S/LCD parallel peripheral timing.
- **Wokwi sim path:** software RGB888 framebuffer + UART packet stream + Wokwi custom chip render.

This is intentionally honest: it does **not** claim electrical/timing emulation of the real scan engine.

## Enable

Set the runtime config on `HUB75_I2S_CFG` before creating `MatrixPanel_I2S_DMA`:

```cpp
HUB75_I2S_CFG cfg(64, 32, 1, pins);
cfg.use_wokwi_sim = true;
cfg.wokwi_uart_tx_pin = 17;

MatrixPanel_I2S_DMA display(cfg);
```

When enabled, `MatrixPanel_I2S_DMA` routes these operations to the sim backend:

- `begin()`
- `updateMatrixDMABuffer(x,y,r,g,b)`
- `updateMatrixDMABuffer(r,g,b)`
- `setBrightness()`
- `flipDMABuffer()`

When `use_wokwi_sim` is `false`, all behavior remains the standard hardware DMA path.

Legacy compatibility: defining `HUB75_WOKWI_SIM=1` still sets the default value of `cfg.use_wokwi_sim`, but it is no longer required.

## Transport

Transport uses `Serial1` at 2,000,000 baud.

In the shipped Wokwi examples, the sim backend maps `Serial1` TX to GPIO `17` and the Wokwi `diagram.json` connects `esp:17` to the custom chip `RX` pin.

### Packet framing

Each packet:

1. 8-byte header
2. payload (`uint32_t` little-endian length)
3. 1-byte XOR CRC over header + payload

Header layout:

| Byte | Meaning |
|---|---|
| 0 | `0x48` (`'H'`) |
| 1 | `0x37` (`'7'`) |
| 2 | packet type |
| 3 | version (`1`) |
| 4..7 | payload length (LE `uint32_t`) |

### Packet types

- `TYPE_CONFIG = 1`
  - payload: `width_u16_le`, `height_u16_le`, `chain_u8`, `brightness_u8`
- `TYPE_FRAME = 2`
  - payload: `frame_seq_u16_le`, `rgb888[width*height*3]`

## Fidelity limits

Preserved:

- Adafruit GFX-compatible drawing API surface
- panel width/height/chaining dimensions
- brightness metadata

Approximated / omitted:

- precise DMA descriptor timing behavior
- OE/LAT cycle-level effects and BCM scan timing
- pixel-shift/ghosting behavior tied to physical panel electricals

## Performance notes

- Full-frame streaming is used (currently no dirty-rectangle protocol).
- Present is throttled to ~30 FPS in the backend.
- Large panel chains may be UART-bandwidth limited in Wokwi.

## Example projects

### Browser Wokwi project

See `examples/WokwiBrowserSim/` for a browser-oriented Wokwi project:

- `sketch.ino`
- `libraries.txt`
- `diagram.json`
- in-project custom chip C/JSON

This is the simplest path when you want to use the library directly from the Wokwi browser editor.

### VS Code / CLI project

See `examples/WokwiSimUART/` for a local Wokwi setup:

- sketch
- `diagram.json`
- `wokwi.toml`
- custom chip JSON/C source
- `Makefile`

### Recommended build flow

From `examples/WokwiSimUART/`:

```bash
make sim-ready
```

This rebuilds both:

- the custom chip wasm (`chips/hub75-sim.chip.wasm`)
- the firmware (`.pio/build/esp32-s3-devkitc-1/firmware.bin`)

Use `make` if you only changed the custom chip implementation, or `~/.platformio/penv/bin/pio run` if you only changed the firmware.

Note: this example intentionally uses the checked-in `Makefile` for custom chip builds. The plain `wokwi-cli chip compile ...` path can produce wasm memory import settings that are incompatible with the Wokwi runtime used by this example.
