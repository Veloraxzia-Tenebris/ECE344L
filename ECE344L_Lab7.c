#include <peripheral/system.h>
#include <plib.h>
#include <math.h>
// Configuration and setup stuff
#pragma config ICESEL = ICS_PGx1
#pragma config FNOSC = PRIPLL
#pragma config POSCMOD = EC
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1
#pragma config FPBDIV = DIV_8
#pragma config FSOSCEN = OFF

#define SYS_FREQ (80000000L)
#define PB_DIV 8
#define cntMsDelay 10000
#define BRG_SPI 63
#define BRG_I2C 0x009
#define I2C_CLOCK_FREQ 100000
#define POT 4
#define AINPUTS 0xFFEF
#define ADC_COUNT 1023
#define VMAX 3.3
#define VMIN 0.0

// Function prototypes
void DeviceInit();
void DelayInit();
void DelayMs(int cms);
void initADC(int amask);
int readADC(int c);

// Global machine state variable
unsigned currentState = 0;
// I2C global variables
unsigned char i2c_data[4];
unsigned char SlaveAddress = 0x4F;
short byte1 = 0, byte2 = 0, rd_cnt = 0;
unsigned char* in_char_p = 0;
int rawTemperature = 0, temperature = 0;
// ADC global variables
short int changedVar, byte1 = 0, byte2 = 0, rd_cnt;

int main() {
	// Initialization
	DeviceInit();
	DelayInit();
	// LEDs counter
	unsigned LEDs = 0;
	// SPI variables
	unsigned char buffer[40];
	unsigned char str[40];
	unsigned char overheatedMessage[40] = "OVERHEATED!";
	unsigned char* bufferp;
	unsigned char* c_buffer = "";
	int rData;
	// I2C variables
	int Index = 0, DataSz = 0;
	// Open and configure SPI
	// Configure SPI SFRs
	bufferp = &buffer[0];
	in_char_p = &i2c_data[0];
	// Configure I2C registers
	SPI1CON = 0;
	rData = SPI1BUF;
	SPI1BRG = BRG_SPI;
	SPI1STATCLR = 0X40;
	SPI1CON = 0x8320;
	SPI1CON = SPI1CON | 0x10000000;
	// Reset display
	SPI1BUF = 0x1b;
	c_buffer = "[*";
	putsSPI1(2, c_buffer);
	DelayMs(500);
	// Display initialization message
	bufferp = "SPI Display On";
	putsSPI1(strlen(bufferp), bufferp);
	DelayMs(500);
	// Open and configure I2C
	OpenI2C1(I2C_EN, BRG_I2C);
	SlaveAddress = 0x4f;
	i2c_data[0] = (SlaveAddress << 1) | 0x0;
	i2c_data[1] = 0x00;
	DataSz = 2;
	// Sent start command
	StartI2C1();
	IdleI2C1();
	// Set input pointer to start of input
	Index = 0;
	// Write to tmp3 module
	while(DataSz) {
		MasterWriteI2C1(i2c_data[Index++]);
		IdleI2C1();
		DataSz--;
		if(I2C1STATbits.ACKSTAT) {
			break;
		}
	}
	// Send command for finished using I2C bus
	StopI2C1();
	IdleI2C1();
	// ADC configuration
	int inputValue;
	char adcStr[16];
	float bitValue = (VMAX - VMIN) / ADC_COUNT;
	float analogValue;
	changedVar = 0;
	// Initialize the ADC
	initADC(AINPUTS);
	// Main while loop
	while(1) {
		// Reset LED state
		LEDs = 0;
		// Reset current state
		currentState = 0;

		// Read value of potentiometer
		byte1 = readADC(POT);
		analogValue = (float) byte1 * bitValue;
		// Set values of LEDs
		currentState = (int) (analogValue * 10) % 8;
		int i = 0;
		for(i = 0; i < currentState; i++) {
			LEDs |= (1 << i);
		}

		// Send command to clear out I2C bus
		RestartI2C1();
		IdleI2C1();
		// Send read command
		MasterWriteI2C1((SlaveAddress << 1) | 0x01);
		IdleI2C1();
		// Read 2 bytes
		rd_cnt = MastergetsI2C1(2, in_char_p, 152);
		// Send command for finished using I2C bus
		StopI2C1();
		IdleI2C1();
		// 2 bytes to 2 1 byte variables
		byte1 = (short int) i2c_data[0];
		byte2 = (short int) i2c_data[1];
		// Combine bits for temperature data
		rawTemperature = byte2 | (byte1 << 8);
		// Shift out temperature bits
		temperature = rawTemperature / 256;
		// Convert C to F
		temperature = ((temperature * 9)/ 5) + 32;

		// Overheating
		if(temperature > 80) {
			DelayMs(10);
			// Flash "OVERHEATED!"
			putsSPI1(strlen(overheatedMessage), overheatedMessage);
			DelayMs(100);
			// Clear screen and reset cursor
			c_buffer = "[j";
			putsSPI1(2, c_buffer);
			DelayMs(100);
			// Clear screen and reset cursor
			c_buffer = "[j";
			putsSPI1(2, c_buffer);
			DelayMs(10);
			putsSPI1(strlen(str), str);
			DelayMs(100);
			// Turn off LEDs
			LEDs = 0;
		} else {
			// Write power level to LCD
			sprintf(str, "Temp: %d", temperature);
			// Clear screen and reset cursor
			c_buffer = "[j";
			putsSPI1(2, c_buffer);
			DelayMs(10);
			putsSPI1(strlen(str), str);
			DelayMs(100);
		}
		// Escape character
		SPI1BUF = 0x1b;
		// Clear screen and reset cursor
		c_buffer = "[j";
		putsSPI1(2, c_buffer);
		DelayMs(10);
		putsSPI1(strlen(str), str);
		DelayMs(100);

		// Display LEDs
		PORTWrite(IOPORT_E, LEDs);
	}
	return 0;
}

// Initialization for device I/O
void DeviceInit() {
	DDPCONbits.JTAGEN = 0;
	PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTSetPinsDigitalOut(IOPORT_F, BIT_9);
	PORTClearBits(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
}

void DelayInit() {
	unsigned int tcfg;
	tcfg = T2_ON | T2_SOURCE_INT | T2_PS_1_1;
	OpenTimer2(tcfg, 0XFFFF);
	mT2ClearIntFlag();
}

void DelayMs(int cms) {
	int ims;
	for(ims = 0; ims < cms; ims++) {
		WriteTimer2(0);
		while(ReadTimer2() < cntMsDelay);
	}
}

void initADC(int amask) {
	AD1PCFG = amask;
	AD1CON1 = 0x00E0;
	AD1CSSL = 0;
	AD1CON2 = 0;
	AD1CON3 = 0X1F01;
	AD1CON1bits.ADON = 1;
}

int readADC(int ch) {
	AD1CHSbits.CH0SA = ch;
	AD1CON1bits.SAMP = 1;
	while(!AD1CON1bits.DONE);
	return ADC1BUF0;
}