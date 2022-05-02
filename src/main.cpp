#include "Arduino.h"

#define I2C_ADDR 0x3c
#define OUT_REG PORTB
#define PI2C_SDA PB0
#define PI2C_SCL PB2
#define IN_REG PINB

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// #define DEBUG_ENABLED
#ifdef DEBUG_ENABLED
#include <TinyDebug.h>
#endif

// #include "i2c_softduino.h" // manual pin manipulation using arduino pins
// #include "i2c_bitbang.h" // manual pin manipulation
#include "i2c_usi.h" // hardware I2C

#include "gfx.h"

void setup() {
#ifdef DEBUG_ENABLED
  Debug.begin();
#endif
  i2c_init();
  displayInit();
}

/**
 * connect 2 points from points array
 */
void connectPoints(uint8_t i, uint8_t j, int points[][2]) {
  drawLine(points[i][0], points[i][1], points[j][0], points[j][1]);
}

// #define GFX_DRAWLINE_TEST
#ifdef GFX_DRAWLINE_TEST
#include "gfx_drawline_test.h"
#endif

#ifdef DEBUG_ENABLED
uint32_t totalTimeSum = 0;
uint32_t mathTimeSum = 0;
uint32_t renderTimeSum = 0;
uint16_t count = 0;
#endif

#include "cube.h"

void loop() {

#ifdef DEBUG_ENABLED
  uint32_t loopStart = millis();
#endif

#ifdef GFX_DRAWLINE_TEST
  testLine();
  return;
#else
  time_frame++;

  cube_calculate();

#ifdef DEBUG_ENABLED
  mathTimeSum += millis() - loopStart;
#endif

  firstPage();
  do {
#ifdef DEBUG_ENABLED
  uint32_t renderStart = millis();
#endif

    cube_render();

#ifdef DEBUG_ENABLED
    renderTimeSum += millis() - renderStart;
#endif
  } while (nextPage());
#endif

#ifdef DEBUG_ENABLED
  totalTimeSum += millis() - loopStart;
  count++;

  if (totalTimeSum > 5000 || count > 100) { // print stats every ~5000ms
    uint32_t millisPerCycle = totalTimeSum / count;
    Debug.print(F("math millis: "));
    Debug.print(mathTimeSum / count);
    Debug.print(F(", render millis: "));
    Debug.print(renderTimeSum / count);
    Debug.print(F(", cycle millis: "));
    Debug.print(millisPerCycle);
    Debug.print(F(", fps: "));
    Debug.println(1 / (0.001 * totalTimeSum / count));
    count = 0;
    totalTimeSum = mathTimeSum = renderTimeSum = 0;
  }
#endif
}
