/**
 * project's git https://github.com/dchekmarev/attiny85-oled-3d-cube
 */

#include "Arduino.h"

#define I2C_ADDR 0x3c
#define OUT_REG PORTB
#define PI2C_SDA PB0
#define PI2C_SCL PB2
#define IN_REG PINB

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define I2C_ALLOW_SHORTCUTS
#define I2C_FAST_DELAY

// #define DEBUG_ENABLED
#ifdef DEBUG_ENABLED
#include <TinyDebug.h>
#endif

#include "i2c.h"
#include "gfx.h"

void setup() {
#ifdef DEBUG_ENABLED
  Debug.begin();
#endif
  DDRB = (1 << DDB2) | (1 << DDB0);  // Set the PB0 and PB2 as output
}

#define NPOINTS 8

int points[NPOINTS][2];  // eight 2D points for the cube, values will be calculated in the code

int8_t const orig_points[NPOINTS][3] = {  // eight 3D points - set values for 3D cube
    {-1, -1, 1},
    {1, -1, 1},
    {1, 1, 1},
    {-1, 1, 1},
    {-1, -1, -1},
    {1, -1, -1},
    {1, 1, -1},
    {-1, 1, -1}};

float rotated_3d_points[NPOINTS][3];
uint16_t angle_deg_0 = 60;
uint16_t angle_deg_1 = 60;
uint16_t angle_deg_2 = 60;
#define z_offset -4.0

float time_frame;

/**
 * connect 2 points from points array
 */
void connectPoints(uint8_t i, uint8_t j) {
  drawLine(points[i][0], points[i][1], points[j][0], points[j][1]);
}

/**
 * rotate points around given axis by given degree
 */
void rotate(uint16_t angle_deg, uint8_t axis0) {
  uint8_t axis1 = axis0 == 1 ? 0 : 1;
  uint8_t axis2 = axis0 == 2 ? 0 : 2;
  float rads = radians(angle_deg);
  float cos_val = cos(rads);
  float sin_val = sin(rads);

  // rotate 3d points in given 2-axis projection
  for (uint8_t i = 0; i < NPOINTS; i++) {
    float axis0_coord = rotated_3d_points[i][axis0] * cos_val - rotated_3d_points[i][axis2] * sin_val;
    float axis1_coord = rotated_3d_points[i][axis1];
    float axis2_coord = rotated_3d_points[i][axis0] * sin_val + rotated_3d_points[i][axis2] * cos_val;
    rotated_3d_points[i][axis0] = axis0_coord;
    rotated_3d_points[i][axis1] = axis1_coord;
    rotated_3d_points[i][axis2] = axis2_coord;
  }
}

// #define GFX_DRAWLINE_TEST
#ifdef GFX_DRAWLINE_TEST
#include "gfx_drawline_test.h"
#endif

#ifdef DEBUG_ENABLED
uint32_t totalTimeSum = 0;
uint32_t mathTimeSum = 0;
uint16_t count = 0;
#endif

void loop() {

#ifdef DEBUG_ENABLED
  uint32_t loopStart = millis();
#endif

#ifdef GFX_DRAWLINE_TEST
  testLine();
  return;
#endif
  time_frame++;

  uint16_t cube_size = 48 + sin(time_frame * 0.2) * 15;
  angle_deg_0 = (angle_deg_0 + 3) % 360;
  angle_deg_1 = (angle_deg_1 + 5) % 360;
  angle_deg_2 = (angle_deg_2 + 7) % 360;

  // init points
  for (uint8_t i = 0; i < NPOINTS; i++) {
    rotated_3d_points[i][0] = orig_points[i][0];
    rotated_3d_points[i][1] = orig_points[i][1];
    rotated_3d_points[i][2] = orig_points[i][2];
  }

  // rotate to current position
  rotate(angle_deg_0, 0);
  rotate(angle_deg_1, 1);
  rotate(angle_deg_2, 2);

  // calculate the points
  for (uint8_t i = 0; i < NPOINTS; i++) {
    // project 3d points into 2d space with perspective divide -- 2D x = x/z,   2D y = y/z
    points[i][0] = round(64 + rotated_3d_points[i][0] / (rotated_3d_points[i][2] + z_offset) * cube_size);
    points[i][1] = round(32 + rotated_3d_points[i][1] / (rotated_3d_points[i][2] + z_offset) * cube_size);
  }

#ifdef DEBUG_ENABLED
  mathTimeSum += millis() - loopStart;
#endif

  firstPage();
  do {
    // connect the lines between the individual points
    for (uint8_t i = 0; i < 4; i++) {
      connectPoints(i, (i + 1) % 4);
      connectPoints(4 + i, 4 + (i + 1) % 4);
      connectPoints(i, 4 + i);
    }
  } while (nextPage());

#ifdef DEBUG_ENABLED
  totalTimeSum += millis() - loopStart;
  count++;

  if (totalTimeSum > 5000 || count > 100) { // print stats every ~5000ms
    uint32_t millisPerCycle = totalTimeSum / count;
    Debug.print(F("math millis: "));
    Debug.print(mathTimeSum / count);
    Debug.print(F(", cycle millis: "));
    Debug.print(millisPerCycle);
    Debug.print(F(", fps: "));
    Debug.println(1 / (0.001 * totalTimeSum / count));
    count = 0;
    totalTimeSum = mathTimeSum = 0;
  }
#endif
}
