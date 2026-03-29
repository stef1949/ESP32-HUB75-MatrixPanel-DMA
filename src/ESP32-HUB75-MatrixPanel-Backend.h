#pragma once

#include <cstddef>
#include <cstdint>

struct HUB75_I2S_CFG;

class MatrixPanelBackend {
public:
  virtual ~MatrixPanelBackend() = default;

  virtual bool begin(const HUB75_I2S_CFG &cfg) = 0;
  virtual void drawPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual void fillScreen(uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual void setBrightness(uint8_t b) = 0;
  virtual void flipBuffer() = 0;
};
