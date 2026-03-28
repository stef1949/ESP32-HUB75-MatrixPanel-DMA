#include "wokwi-api.h"
#include <string.h>

#define MAGIC0 0x48
#define MAGIC1 0x37
#define VERSION 1
#define TYPE_CONFIG 1
#define TYPE_FRAME 2

typedef struct {
  uart_dev_t uart;
  buffer_t fb;
  uint32_t width;
  uint32_t height;

  uint8_t state;
  uint8_t header[8];
  uint8_t header_pos;
  uint32_t payload_len;
  uint32_t payload_pos;
  uint8_t payload[65535];
  uint8_t crc;
  bool debug;
} hub75_chip_t;

static hub75_chip_t chip;

static void log_debug(const char *msg) {
  if (chip.debug) {
    printf("[hub75-sim] %s\n", msg);
  }
}

static void render_rgb888_frame(uint8_t *data, uint32_t len) {
  if (len < 2) {
    return;
  }

  const uint32_t expected = chip.width * chip.height * 3 + 2;
  if (len < expected) {
    log_debug("short frame");
    return;
  }

  uint32_t rgba = 0xff000000;
  uint32_t fb_offset = 0;
  uint32_t src = 2; // skip frame sequence

  for (uint32_t y = 0; y < chip.height; y++) {
    for (uint32_t x = 0; x < chip.width; x++) {
      const uint8_t r = data[src++];
      const uint8_t g = data[src++];
      const uint8_t b = data[src++];

      rgba = ((uint32_t)0xff << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
      buffer_write(chip.fb, fb_offset, &rgba, sizeof(rgba));
      fb_offset += 4;
    }
  }
}

static void handle_packet(uint8_t type, uint8_t *payload, uint32_t payload_len) {
  if (type == TYPE_CONFIG && payload_len >= 6) {
    chip.width = payload[0] | ((uint16_t)payload[1] << 8);
    chip.height = payload[2] | ((uint16_t)payload[3] << 8);
    return;
  }

  if (type == TYPE_FRAME) {
    render_rgb888_frame(payload, payload_len);
  }
}

static void uart_rx(void *user_data, uint8_t byte) {
  (void)user_data;

  if (chip.state == 0) {
    chip.header[chip.header_pos++] = byte;
    if (chip.header_pos < 8) {
      return;
    }

    chip.header_pos = 0;
    if (chip.header[0] != MAGIC0 || chip.header[1] != MAGIC1 || chip.header[3] != VERSION) {
      return;
    }

    chip.payload_len = chip.header[4] | ((uint32_t)chip.header[5] << 8) |
                       ((uint32_t)chip.header[6] << 16) | ((uint32_t)chip.header[7] << 24);
    if (chip.payload_len > sizeof(chip.payload)) {
      chip.payload_len = 0;
      return;
    }

    chip.crc = 0;
    for (int i = 0; i < 8; i++) {
      chip.crc ^= chip.header[i];
    }

    chip.payload_pos = 0;
    chip.state = 1;
    return;
  }

  if (chip.state == 1) {
    if (chip.payload_pos < chip.payload_len) {
      chip.payload[chip.payload_pos++] = byte;
      chip.crc ^= byte;
      if (chip.payload_pos == chip.payload_len) {
        chip.state = 2;
      }
    }
    return;
  }

  if (chip.state == 2) {
    if (chip.crc == byte) {
      handle_packet(chip.header[2], chip.payload, chip.payload_len);
    }
    chip.state = 0;
    chip.payload_len = 0;
    chip.payload_pos = 0;
  }
}

void chip_init() {
  memset(&chip, 0, sizeof(chip));

  chip.debug = attr_init("debug", false)->value.boolean;

  uint32_t fb_w = 0;
  uint32_t fb_h = 0;
  chip.fb = framebuffer_init(&fb_w, &fb_h);
  chip.width = fb_w;
  chip.height = fb_h;

  const uart_config_t uart_config = {
      .rx = pin_init("RX", INPUT),
      .tx = NO_PIN,
      .baud_rate = 2000000,
      .rx_data = uart_rx,
      .write_done = NULL,
      .user_data = NULL,
  };

  chip.uart = uart_init(&uart_config);
}
