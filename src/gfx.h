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

// init sequence, from u8g2lib
static const uint8_t ssd1306_128x64_noname_init_seq[] PROGMEM = {
  0x0ae,		                /* display off */
  0x0d5, 0x080,		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  0x0a8, 0x03f,		/* multiplex ratio */
  0x0d3, 0x000,		/* display offset */
  0x040,		                /* set display start line to 0 */
  0x08d, 0x014,		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, SSD1306 only, should be removed for SH1106 */
  0x020, 0x000,		/* horizontal addressing mode */
  
  0x0a1,				/* segment remap a0/a1*/
  0x0c8,				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  0x0da, 0x012,		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  0x081, 0x0cf, 		/* [2] set contrast control */
  0x0d9, 0x0f1, 		/* [2] pre-charge period 0x022/f1*/
  0x0db, 0x040, 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  0x02e,				/* Deactivate scroll */ 
  0x0a4,				/* output ram to display */
  0x0a6,				/* none inverted normal display mode */
  0x0af,		                /* display on */
};

void displayInit() {
  i2c_start();
  i2c_tx((I2C_ADDR << 1));
  i2c_tx(0x00);
  for (int i = 0; i < sizeof(ssd1306_128x64_noname_init_seq); i++) {
    i2c_tx(pgm_read_byte(ssd1306_128x64_noname_init_seq + i));
  }
  i2c_stop();
}

/**
 * copy pixels via i2c to oled display
 */
void displayPage() {
  i2c_start();
  i2c_tx((I2C_ADDR << 1));
  i2c_tx(0x00);
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
