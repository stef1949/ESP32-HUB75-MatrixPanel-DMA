#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#ifndef HUB75_WOKWI_SIM
#error "This example requires -DHUB75_WOKWI_SIM in build flags"
#endif

HUB75_I2S_CFG::i2s_pins dummyPins = {
  .r1 = 4, .g1 = 5, .b1 = 6, .r2 = 7, .g2 = 15, .b2 = 16,
  .a = 17, .b = 18, .c = 8, .d = 3, .e = -1,
  .lat = 46, .oe = 9, .clk = 10
};

HUB75_I2S_CFG mxconfig(64, 32, 1, dummyPins);
MatrixPanel_I2S_DMA display(mxconfig);

uint32_t hue = 0;

static uint16_t rainbow565(uint16_t x, uint16_t y, uint32_t t) {
  uint8_t r = (x * 4 + (t >> 2)) & 0xFF;
  uint8_t g = (y * 6 + (t >> 1)) & 0xFF;
  uint8_t b = ((x + y) * 3 + t) & 0xFF;
  return MatrixPanel_I2S_DMA::color565(r, g, b);
}

void setup() {
  display.begin();
  display.setBrightness8(180);
  display.setTextWrap(false);
  display.setTextSize(1);
}

void loop() {
  for (int y = 0; y < display.height(); y++) {
    for (int x = 0; x < display.width(); x++) {
      display.drawPixel(x, y, rainbow565(x, y, hue));
    }
  }

  display.setCursor(1, 1);
  display.setTextColor(display.color565(255, 255, 255));
  display.print("Wokwi SIM");

  display.drawRect(0, 0, display.width(), display.height(), display.color565(0, 0, 0));
  display.flipDMABuffer();
  hue += 3;
  delay(20);
}
