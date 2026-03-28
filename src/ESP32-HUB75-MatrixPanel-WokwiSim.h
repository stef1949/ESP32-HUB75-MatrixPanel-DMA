#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ESP32-HUB75-MatrixPanel-Backend.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

struct HUB75_I2S_CFG;

class WokwiSimBackend : public MatrixPanelBackend {
public:
  WokwiSimBackend();

  bool begin(const HUB75_I2S_CFG &cfg) override;
  void drawPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override;
  void fillScreen(uint8_t r, uint8_t g, uint8_t b) override;
  void setBrightness(uint8_t b) override;
  void flipBuffer() override;

private:
  void sendConfig();
  void presentIfDue(bool force = false);
  void sendPacket(uint8_t type, const uint8_t *payload, uint32_t payload_len);
  void buildFramePayload(std::vector<uint8_t> &payload);

  uint16_t width = 0;
  uint16_t height = 0;
  uint8_t chain = 1;
  uint8_t brightness = 128;
  uint16_t frame_seq = 0;
  bool dirty = false;
#ifdef ARDUINO
  uint32_t last_present_ms = 0;
#endif
  std::vector<uint8_t> rgb888;
};
