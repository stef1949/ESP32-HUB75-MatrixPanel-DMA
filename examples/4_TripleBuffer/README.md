# Triple Buffer Example

This example demonstrates the triple buffering feature of the ESP32-HUB75-MatrixPanel-DMA library.

## What is Triple Buffering?

Triple buffering extends the concept of double buffering by using three frame buffers instead of two:

- **Single Buffer**: Direct rendering to the display buffer (may cause tearing)
- **Double Buffer**: Two buffers - one being displayed while the other is drawn to
- **Triple Buffer**: Three buffers - one displayed, one prepared for display, one being drawn to

## Benefits

Triple buffering provides:

1. **Smoother Animation**: More time for complex rendering operations
2. **Reduced Frame Drops**: Less synchronization pressure between render and display
3. **Better Performance**: Rendering can continue without waiting for display refresh

## Trade-offs

- **Memory Usage**: Requires 3x the frame buffer memory
- **Complexity**: More sophisticated buffer management
- **Not Always Necessary**: Simple animations may not benefit significantly

## Usage

```cpp
HUB75_I2S_CFG mxconfig;
mxconfig.triple_buff = true;  // Enable triple buffering
display = new MatrixPanel_I2S_DMA(mxconfig);
```

## Buffer Rotation

The library automatically cycles through buffers: 0 → 1 → 2 → 0 → 1 → 2 ...

## Memory Requirements

Monitor your ESP32's heap usage carefully when using triple buffering:

- **64x32 panel**: ~36KB (3 × 12KB per buffer)  
- **64x64 panel**: ~72KB (3 × 24KB per buffer)
- **Higher color depths**: Even more memory required

Use `ESP.getFreeHeap()` to monitor available memory during development.