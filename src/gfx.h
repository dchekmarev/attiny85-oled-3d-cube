// simple gfx for ssd1306-based display

// in-memory buffer where we draw 128 columns of 8 bits (pixels) region
// for smaller mem footprints it is possible to reduce it, but this will require modifications in drawing (window size) + displayPage
uint8_t pixels[SCREEN_WIDTH];

// current page/region
uint8_t pageNo = 0;

void clearPage() {
  memset(&pixels, 0, SCREEN_WIDTH);
}

/**
 * start drawing from first page/region
 */
void firstPage() {
  pageNo = 0;
  clearPage();
}

/**
 * copy pixels via i2c to oled display
 */
void displayPage() {
  i2c_start();
  i2c_tx((I2C_ADDR << 1));   
  i2c_tx(0x00);              // Sending command
  i2c_tx(0x21);              // Set Column
  i2c_tx(0x00);              // Start at column 0
  i2c_tx(SCREEN_WIDTH - 1);  // End at 127
  i2c_tx(0x22);              // Set Page
  i2c_tx(pageNo);            // Start at page N
  i2c_tx(pageNo);            // End at page N
  i2c_stop();

  i2c_start();
  i2c_tx((I2C_ADDR << 1));
  i2c_tx(0x40);              // Sendind data
  for (uint8_t i = 0; i < SCREEN_WIDTH; ++i) {
    i2c_tx(pixels[i]);
  }
  i2c_stop();
}

/**
 * displays current page/region and moves to next if any
 */
bool nextPage() {
  if (pageNo < SCREEN_HEIGHT / 8) {
    displayPage();
    if (pageNo < SCREEN_HEIGHT / 8 - 1) {
      clearPage();
    }
  }
  return (++pageNo < (SCREEN_HEIGHT / 8));
}

/**
 * internal drawLine with substituted current drawing region coords
 * min_y & max_y - borders of currently rendered display page
 */
void _drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t min_y, uint8_t max_y) {
  int8_t dx = x2 - x1;
  if (dx == 0) {
    // vertical
    uint8_t pixel = pixels[x1];
    for (uint8_t y = max(min(y1, y2), min_y); y <= min(max(y1, y2), max_y); ++y) {
      pixel |= 1 << (y - min_y);
    }
    pixels[x1] = pixel;
  } else {
    // non-vertical
    if (max(y1, y2) < min_y || min(y1, y2) > max_y) {
      return;
    }
    int8_t dy = y2 - y1;
    float a = 1.0 * dy / dx;
    int16_t b = y1 - a * x1; // coef might be higher than coords
    if (abs(a) <= 1) {
      // it's more horizontal, iterate by X
      uint8_t x = a > 0 ? max(x1, (-1 + min_y - b) / a)
                    : a == 0 ? x1
                             : max(x1, (1 + max_y - b) / a);
      uint8_t max_x = a > 0 ? min(x2, (1 + max_y - b) / a + 1)
                            : a == 0 ? x2
                                     : min(x2, (-1 + min_y - b) / a + 1);
      for (; x < max_x; x++) {
        int y = a * x + b;
        if (y >= min_y && y <= max_y) {
          pixels[x] |= 1 << (y - min_y);
        }
      }
    } else {
      // it's more vertical, iterate by Y
      int y = max(min(y1, y2), min_y);
      max_y = min(max(y1, y2), max_y);
      for (; y <= max_y; y++) {
        int x = (y - b) / a;
        pixels[x] |= 1 << (y - min_y);
      }
    }
  }
}

/**
 * draw a line between 2 points visible within current page/region
 */
void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  if (x1 < x2) {
    _drawLine(x1, y1, x2, y2, pageNo * 8, (pageNo + 1) * 8 - 1);
  } else {
    _drawLine(x2, y2, x1, y1, pageNo * 8, (pageNo + 1) * 8 - 1);
  }
}
