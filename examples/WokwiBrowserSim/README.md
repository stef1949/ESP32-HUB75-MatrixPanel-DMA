# Wokwi browser example

This example is structured for the Wokwi browser editor.

It uses:

- `sketch.ino` for the ESP32 sketch
- `libraries.txt` to install the library from Wokwi's Arduino library index
- `diagram.json` for the ESP32 + custom chip wiring
- `hub75-sim.chip.c` / `hub75-sim.chip.json` for the in-project custom chip

The sketch enables the simulator backend with normal library configuration:

```cpp
cfg.use_wokwi_sim = true;
cfg.wokwi_uart_tx_pin = 17;
```

That means there is no `wokwi.toml`, no local wasm build step, and no PlatformIO requirement for the browser flow.

Notes:

- This depends on a library release that includes the runtime `use_wokwi_sim` / `wokwi_uart_tx_pin` config fields.
- If Wokwi's library index has not picked up that release yet, use the local `examples/WokwiSimUART/` project instead, or wait until the next published library release is available in Wokwi.
