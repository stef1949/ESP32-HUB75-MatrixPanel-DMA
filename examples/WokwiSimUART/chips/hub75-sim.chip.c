#include "wokwi-api.h"
#include <stdlib.h>
#include <string.h>

#define MAGIC0 0x48
#define MAGIC1 0x37
#define VERSION 1
#define TYPE_CONFIG 1
#define TYPE_FRAME 2
#define MAX_LOGICAL_WIDTH 128u
#define MAX_LOGICAL_HEIGHT 64u
#define MAX_FRAME_BYTES ((MAX_LOGICAL_WIDTH * MAX_LOGICAL_HEIGHT * 3u) + 2u)

typedef struct {
  uart_dev_t uart;
  buffer_t fb;
  uint32_t logical_width;
  uint32_t logical_height;
  uint32_t fb_width;
  uint32_t fb_height;

  uint8_t state;
  uint8_t header[8];
  uint8_t header_pos;
  uint32_t payload_len;
  uint32_t payload_pos;
  uint8_t *payload;
  uint32_t payload_capacity;
  uint32_t *scanline;
  uint32_t scanline_capacity;
  uint8_t crc;
  bool debug;
} hub75_chip_t;

static hub75_chip_t chip;

static void log_debug(const char *msg) {
  (void)msg;
}

static bool ensure_payload_capacity(uint32_t width, uint32_t height) {
  const uint64_t required64 = (uint64_t)width * (uint64_t)height * 3u + 2u;
  if (required64 == 0 || required64 > MAX_FRAME_BYTES) {
    log_debug("frame too large");
    return false;
  }

  const uint32_t required = (uint32_t)required64;
  if (required <= chip.payload_capacity && chip.payload != NULL) {
    return true;
  }

  uint8_t *new_payload = realloc(chip.payload, required);
  if (new_payload == NULL) {
    log_debug("payload alloc failed");
    return false;
  }

  chip.payload = new_payload;
  chip.payload_capacity = required;
  return true;
}

static bool ensure_scanline_capacity(uint32_t width) {
  if (width == 0) {
    return false;
  }

  if (width <= chip.scanline_capacity && chip.scanline != NULL) {
    return true;
  }

  uint32_t *new_scanline = realloc(chip.scanline, width * sizeof(uint32_t));
  if (new_scanline == NULL) {
    log_debug("scanline alloc failed");
    return false;
  }

  chip.scanline = new_scanline;
  chip.scanline_capacity = width;
  return true;
}

static void render_rgb888_frame(uint8_t *data, uint32_t len) {
  if (len < 2) {
    return;
  }

  const uint32_t expected = chip.logical_width * chip.logical_height * 3 + 2;
  if (len < expected) {
    log_debug("short frame");
    return;
  }

  uint32_t scale_x = chip.fb_width / chip.logical_width;
  uint32_t scale_y = chip.fb_height / chip.logical_height;
  if (scale_x == 0) {
    scale_x = 1;
  }
  if (scale_y == 0) {
    scale_y = 1;
  }

  if (!ensure_scanline_capacity(chip.fb_width)) {
    return;
  }

  uint32_t src = 2; // skip frame sequence

  for (uint32_t y = 0; y < chip.logical_height; y++) {
    uint32_t scan_x = 0;
    for (uint32_t x = 0; x < chip.logical_width; x++) {
      const uint8_t r = data[src++];
      const uint8_t g = data[src++];
      const uint8_t b = data[src++];

      const uint32_t rgba = ((uint32_t)0xff << 24) | ((uint32_t)b << 16) |
                            ((uint32_t)g << 8) | (uint32_t)r;
      for (uint32_t px = 0; px < scale_x && scan_x < chip.fb_width; px++) {
        chip.scanline[scan_x++] = rgba;
      }
    }

    while (scan_x < chip.fb_width) {
      chip.scanline[scan_x] = 0xff000000;
      scan_x++;
    }

    const uint32_t base_y = y * scale_y;
    for (uint32_t py = 0; py < scale_y && (base_y + py) < chip.fb_height; py++) {
      const uint32_t row_offset = ((base_y + py) * chip.fb_width) * 4;
      buffer_write(chip.fb, row_offset, chip.scanline, chip.fb_width * 4);
    }
  }

  for (uint32_t y = chip.logical_height * scale_y; y < chip.fb_height; y++) {
    if (!ensure_scanline_capacity(chip.fb_width)) {
      return;
    }
    for (uint32_t x = 0; x < chip.fb_width; x++) {
      chip.scanline[x] = 0xff000000;
    }
    buffer_write(chip.fb, (y * chip.fb_width) * 4, chip.scanline, chip.fb_width * 4);
  }
}

static void init_buffers(void) {
  if (chip.payload == NULL) {
    chip.payload = malloc(MAX_FRAME_BYTES);
    if (chip.payload != NULL) {
      chip.payload_capacity = MAX_FRAME_BYTES;
    }
  }

  if (chip.fb_width > 0 && chip.scanline == NULL) {
    chip.scanline = malloc(chip.fb_width * sizeof(uint32_t));
    if (chip.scanline != NULL) {
      chip.scanline_capacity = chip.fb_width;
      for (uint32_t x = 0; x < chip.fb_width; x++) {
        chip.scanline[x] = 0xff000000;
      }
    }
  }
}

static void handle_packet(uint8_t type, uint8_t *payload, uint32_t payload_len) {
  if (type == TYPE_CONFIG && payload_len >= 6) {
    const uint32_t width = payload[0] | ((uint16_t)payload[1] << 8);
    const uint32_t height = payload[2] | ((uint16_t)payload[3] << 8);
    if (!ensure_payload_capacity(width, height)) {
      return;
    }
    chip.logical_width = width;
    chip.logical_height = height;
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
    if (chip.payload_len > chip.payload_capacity) {
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

  const uint32_t debug_attr = attr_init("debug", false);
  chip.debug = attr_read(debug_attr);

  uint32_t fb_w = 0;
  uint32_t fb_h = 0;
  chip.fb = framebuffer_init(&fb_w, &fb_h);
  chip.fb_width = fb_w;
  chip.fb_height = fb_h;
  chip.logical_width = fb_w;
  chip.logical_height = fb_h;
  init_buffers();

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
