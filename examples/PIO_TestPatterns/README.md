# Test Patterns

Simple solid colors, gradients and test line patterns, could be used to test matrices for proper operation, flickering and estimate fillrate timings.

It is also used in CI test builds to check different build flags scenarios.

Should be build and uploaded as a [platformio](https://platformio.org/) project

## Toolchain download troubleshooting

If your environment blocks `dl.platformio.org` / `dl.registry.platformio.org`
(common in locked-down CI/proxy networks), toolchain installation may fail with:

```
HTTPClientError
```

This example's `platformio.ini` pins Xtensa/RISC-V toolchains and GDB packages to
Espressif GitHub release assets to avoid those blocked domains.

For first-time setup in CI/containers, run:

```bash
./tools/setup_platformio_env.sh
```

The repository also contains local fallback PlatformIO package manifests under
`tools/pio-packages/` for tools that are frequently blocked in restricted
environments.


To build with Arduino's framework use
```
pio run -t upload
```

To build using ESP32 IDF with arduino's component use
```
pio run -t upload -e idfarduino
```
