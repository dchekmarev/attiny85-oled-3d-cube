void testLine() {
  for (int c = 0; c < 64; c++) {
    firstPage();
    do {
      drawLine(0, 32, 127, c);
    } while (nextPage());
  }

  for (int c = 0; c < 64; c++) {
    firstPage();
    do {
      drawLine(127, 32, 0, c);
    } while (nextPage());
  }

  for (int c = 0; c < 128; c++) {
    firstPage();
    do {
      drawLine(63, 0, c, 63);
    } while (nextPage());
  }

  for (int c = 0; c < 128; c++) {
    firstPage();
    do {
      drawLine(63, 63, c, 0);
    } while (nextPage());
  }
}
