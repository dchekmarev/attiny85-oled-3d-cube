// #define I2C_FAST_MODE 1
#define I2C_NOP_DELAY 1
// #define I2C_NO_DELAY 1


// I2C timings as per specification
#if I2C_FAST_MODE == 1
	#define I2C_TLOW	1.3
	#define I2C_THIGH	0.6
#else
	#define I2C_TLOW	4.7
	#define I2C_THIGH	4.0
#endif

#define DDR_USI             DDRB
#define PORT_USI            PORTB
#define PIN_USI             PINB
#define PORT_USI_SDA        PB0
#define PORT_USI_SCL        PB2
#define PIN_USI_SDA         PINB0
#define PIN_USI_SCL         PINB2

#define USISR_TRANSFER_8_BIT 		0b11110000 | (0x00<<USICNT0)
#define USISR_TRANSFER_1_BIT 		0b11110000 | (0x0E<<USICNT0)

#define USICR_CLOCK_STROBE_MASK		0b00101011

#define USI_CLOCK_STROBE()			{ USICR = USICR_CLOCK_STROBE_MASK; }

#define USI_SET_SDA_OUTPUT()		{ DDR_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_INPUT() 		{ DDR_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SDA_HIGH()			{ PORT_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_LOW()			{ PORT_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SCL_OUTPUT()		{ DDR_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_INPUT() 		{ DDR_USI &= ~(1 << PORT_USI_SCL); }

#define USI_SET_SCL_HIGH()			{ PORT_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_LOW()			{ PORT_USI &= ~(1 << PORT_USI_SCL); }



#if I2C_NOP_DELAY == 1
inline void dly() { __asm__("nop\n\t"); };
#define USI_I2C_WAIT_HIGH()			{ dly(); }
#define USI_I2C_WAIT_LOW()			{ dly();  }
#elif I2C_NO_DELAY == 1
#define USI_I2C_WAIT_HIGH()			{ }
#define USI_I2C_WAIT_LOW()			{ }
#else
#include <util/delay.h>
#define USI_I2C_WAIT_HIGH()			{ _delay_us(I2C_THIGH); }
#define USI_I2C_WAIT_LOW()			{ _delay_us(I2C_TLOW);  }
#endif

/**
 * Transfers out data which pre-stored in USIDR
 */
char _USI_I2C_Master_Transfer(char USISR_temp) {
	USISR = USISR_temp;								//Set USISR as requested by calling function

	// Shift Data
	do {
		USI_I2C_WAIT_LOW();
		USI_CLOCK_STROBE();								//SCL Positive Edge
		while (!(PIN_USI&(1<<PIN_USI_SCL)));		//Wait for SCL to go high
		USI_I2C_WAIT_HIGH();
		USI_CLOCK_STROBE();								//SCL Negative Edge
	} while (!(USISR&(1<<USIOIF)));					//Do until transfer is complete
	
	USI_I2C_WAIT_LOW();

	return USIDR;
}

#define SDA_ON (OUT_REG |= (1 << PI2C_SDA))
#define SDA_OFF (OUT_REG &= ~(1 << PI2C_SDA))
#define SCL_ON (OUT_REG |= (1 << PI2C_SCL))
#define SCL_OFF (OUT_REG &= ~(1 << PI2C_SCL))

#define SDA_READ (IN_REG & (1 << PI2C_SDA))
#define SCL_READ (IN_REG & (1 << PI2C_SCL))

/**
 * Init necessary ports/registers, set USI into I2C mode.
 */
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf
void i2c_init() {
  DDRB |= (1<<PORT_USI_SDA)|(1<<PORT_USI_SCL);
  DDR_USI  |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
  PORT_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
  USIDR = 0xFF;
  // USICR – USI Control Register
  USICR = (0 << USISIE) | (0 << USIOIE) | (1 << USIWM1) | (0 << USIWM0) | (1 << USICS1) | (0 << USICS0) | (1 << USICLK) | (0 << USITC);
  //  USISR – USI Status Register
  USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF)  | (1 << USIDC)  | (0x00 << USICNT0);
}

/**
 * I2C start sequence.
 * SDA switches from high to low before the SCL switches from high to low.
 */
void i2c_start() {
	USI_SET_SCL_HIGH(); 						//Setting input makes line pull high

	while (!(PIN_USI & (1<<PIN_USI_SCL)));		//Wait for SCL to go high

	#if I2C_FAST_MODE == 1
		USI_I2C_WAIT_HIGH();
	#else
		USI_I2C_WAIT_LOW();
	#endif
	USI_SET_SDA_OUTPUT();
	USI_SET_SCL_OUTPUT();
	USI_SET_SDA_LOW();
	USI_I2C_WAIT_HIGH();
	USI_SET_SCL_LOW();
	USI_I2C_WAIT_LOW();
	USI_SET_SDA_HIGH();
}

/**
 * I2C stop sequence.
 * SDA switches from low to high after SCL switches from low to high.
 */
void i2c_stop() {
	USI_SET_SDA_LOW();           				// Pull SDA low.
	USI_I2C_WAIT_LOW();
	USI_SET_SCL_INPUT();            				// Release SCL.
	while( !(PIN_USI & (1<<PIN_USI_SCL)) );  	// Wait for SCL to go high.  
	USI_I2C_WAIT_HIGH();
	USI_SET_SDA_INPUT();            				// Release SDA.
	while( !(PIN_USI & (1<<PIN_USI_SDA)) );  	// Wait for SDA to go high. 
}

/* Transmit 8 bit data to slave */
bool i2c_tx(uint8_t dat) {
  USIDR = dat;
  _USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);
  USI_SET_SDA_INPUT();
  if (_USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT) & 0x01) {
    USI_SET_SCL_HIGH();
    USI_SET_SDA_HIGH();
    return 0;
  }
  USI_SET_SDA_OUTPUT();
  return 1;
}
