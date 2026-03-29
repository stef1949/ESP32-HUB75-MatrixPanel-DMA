#include "ESP32-HUB75-MatrixPanel-WokwiSim.h"

#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

#ifdef ARDUINO
#include <HardwareSerial.h>
#endif

namespace {
constexpr uint8_t kMagic0 = 0x48; // 'H'
constexpr uint8_t kMagic1 = 0x37; // '7'
constexpr uint8_t kVersion = 1;
constexpr uint8_t kPacketConfig = 1;
constexpr uint8_t kPacketFrame = 2;
constexpr uint32_t kMaxFps = 30;
constexpr uint32_t kFrameIntervalMs = 1000 / kMaxFps;
#ifdef ARDUINO
constexpr int8_t kSimUartRxPin = -1;
#endif
} // namespace

WokwiSimBackend::WokwiSimBackend() = default;

bool WokwiSimBackend::begin(const HUB75_I2S_CFG &cfg) {
  width = cfg.mx_width * cfg.chain_length;
  height = cfg.mx_height;
  chain = static_cast<uint8_t>(cfg.chain_length);

  const size_t fb_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
  rgb888.assign(fb_size, 0);

#ifdef ARDUINO
  const int8_t tx_pin = (cfg.wokwi_uart_tx_pin >= 0) ? cfg.wokwi_uart_tx_pin
                                                     : HUB75_WOKWI_SIM_TX_PIN_DEFAULT;
  Serial1.begin(2000000, SERIAL_8N1, kSimUartRxPin, tx_pin);
  delay(20);
#endif

  sendConfig();
  presentIfDue(true);
  return true;
}

void WokwiSimBackend::drawPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
  if (x >= width || y >= height || rgb888.empty()) {
    return;
  }

  const size_t offset = (static_cast<size_t>(y) * width + x) * 3;
  rgb888[offset + 0] = r;
  rgb888[offset + 1] = g;
  rgb888[offset + 2] = b;
  dirty = true;
  presentIfDue(false);
}

void WokwiSimBackend::fillScreen(uint8_t r, uint8_t g, uint8_t b) {
  for (size_t i = 0; i < rgb888.size(); i += 3) {
    rgb888[i + 0] = r;
    rgb888[i + 1] = g;
    rgb888[i + 2] = b;
  }

  dirty = true;
  presentIfDue(true);
}

void WokwiSimBackend::setBrightness(uint8_t b) {
  brightness = b;
  sendConfig();
  presentIfDue(true);
}

void WokwiSimBackend::flipBuffer() {
  presentIfDue(true);
}

void WokwiSimBackend::sendConfig() {
  uint8_t payload[6] = {
      static_cast<uint8_t>(width & 0xff),
      static_cast<uint8_t>((width >> 8) & 0xff),
      static_cast<uint8_t>(height & 0xff),
      static_cast<uint8_t>((height >> 8) & 0xff),
      chain,
      brightness,
  };

  sendPacket(kPacketConfig, payload, sizeof(payload));
}

void WokwiSimBackend::presentIfDue(bool force) {
  if (!dirty && !force) {
    return;
  }

#ifdef ARDUINO
  const uint32_t now = millis();
  if (!force && (now - last_present_ms) < kFrameIntervalMs) {
    return;
  }
  last_present_ms = now;
#endif

  std::vector<uint8_t> payload;
  buildFramePayload(payload);
  sendPacket(kPacketFrame, payload.data(), static_cast<uint32_t>(payload.size()));
  dirty = false;
}

void WokwiSimBackend::buildFramePayload(std::vector<uint8_t> &payload) {
  payload.clear();
  payload.reserve(2 + rgb888.size());
  payload.push_back(static_cast<uint8_t>(frame_seq & 0xff));
  payload.push_back(static_cast<uint8_t>((frame_seq >> 8) & 0xff));
  payload.insert(payload.end(), rgb888.begin(), rgb888.end());
  ++frame_seq;
}

void WokwiSimBackend::sendPacket(uint8_t type, const uint8_t *payload, uint32_t payload_len) {
#ifdef ARDUINO
  uint8_t header[8] = {
      kMagic0,
      kMagic1,
      type,
      kVersion,
      static_cast<uint8_t>(payload_len & 0xff),
      static_cast<uint8_t>((payload_len >> 8) & 0xff),
      static_cast<uint8_t>((payload_len >> 16) & 0xff),
      static_cast<uint8_t>((payload_len >> 24) & 0xff),
  };

  uint8_t crc = 0;
  for (const uint8_t b : header) {
    crc ^= b;
  }
  for (uint32_t i = 0; i < payload_len; ++i) {
    crc ^= payload[i];
  }

  Serial1.write(header, sizeof(header));
  if (payload_len > 0) {
    Serial1.write(payload, payload_len);
  }
  Serial1.write(&crc, 1);
#endif
}
