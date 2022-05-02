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

float rotated_3d_points[NPOINTS][3];  // eight 3D points - rotated around Y axis
uint16_t angle_deg_0 = 60;         // rotation around the Y axis
uint16_t angle_deg_1 = 60;         // rotation around the X axis
uint16_t angle_deg_2 = 60;         // rotation around the Z axis
#define z_offset -4.0              // offset on Z axis
float time_frame;               // ever increasing time value

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

void cube_calculate() {
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
}

void cube_render() {
    // connect the lines between the individual points
    for (uint8_t i = 0; i < 4; i++) {
      connectPoints(i, (i + 1) % 4, points);
      connectPoints(4 + i, 4 + (i + 1) % 4, points);
      connectPoints(i, 4 + i, points);
    }
}