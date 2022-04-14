#define SDA_ON (OUT_REG |= (1 << PI2C_SDA))
#define SDA_OFF (OUT_REG &= ~(1 << PI2C_SDA))
#define SCL_ON (OUT_REG |= (1 << PI2C_SCL))
#define SCL_OFF (OUT_REG &= ~(1 << PI2C_SCL))

#define SDA_READ (IN_REG & (1 << PI2C_SDA))
#define SCL_READ (IN_REG & (1 << PI2C_SCL))

#ifdef I2C_FAST_DELAY
inline void dly() { __asm__("nop\n\t"); };
#else
// for higher clock it might require bigger delays
#include <util/delay.h>
inline void dly() { _delay_us(4.5); }
#endif

/*  i2c start sequence */
void i2c_start() {
  SDA_ON;
  dly();
  SCL_ON;
  dly();
  SDA_OFF;
  dly();
  SCL_OFF;
  dly();
}

/*  i2c stop sequence */
void i2c_stop() {
  SDA_OFF;
  dly();
  SCL_ON;
  dly();
  SDA_ON;
  dly();
}

/* Transmit 8 bit data to slave */
bool i2c_tx(uint8_t dat) {
  for (uint8_t i = 8; i; --i) {
    (dat & 0x80) ? SDA_ON : SDA_OFF;  // Mask for the eigth bit
    dat <<= 1;                        // Move
#ifndef I2C_ALLOW_SHORTCUTS
    dly();
#endif
    SCL_ON;
    dly();
    SCL_OFF;
    dly();
  }
  SDA_ON;
  SCL_ON;
  dly();
#ifndef I2C_ALLOW_SHORTCUTS
  bool ack = !SDA_READ;  // Acknowledge bit
  SCL_OFF;
  return ack;
#else
  SCL_OFF;
  return true;
#endif
}
