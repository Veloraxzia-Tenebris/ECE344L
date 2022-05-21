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
#define BRG_UART 64
#define I2C_CLOCK_FREQ 100000

// Function prototypes
void DeviceInit();
void DelayInit();
void DelayMs(int cms);

// Global machine state variable
unsigned currentState = 0;
// I2C global variables
unsigned char i2c_data[4];
unsigned char SlaveAddress = 0x4F;
short byte1 = 0, byte2 = 0, rd_cnt = 0;
unsigned char* in_char_p = 0;
int rawTemperature = 0, temperature = 0;

int main() {
	// Initialization
	DeviceInit();
	DelayInit();
	// LEDs counter
	unsigned LEDs = 0;
	unsigned button1 = 0, button2 = 0, button3 = 0;
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
	// Open and configure UART
	unsigned long int u2mode;
	unsigned long int u2status;
	u2mode = UART_EN | UART_IDLE_CON | UART_RX_TX |
		    UART_DIS_WAKE | UART_DIS_LOOPBACK |
		    UART_DIS_ABAUD | UART_NO_PAR_8BIT | UART_EN_BCLK |
		    UART_1STOPBIT | UART_IRDA_DIS | UART_MODE_SIMPLEX |
		    UART_NORMAL_RX | UART_BRGH_SIXTEEN;
	u2status = UART_TX_PIN_LOW | UART_RX_ENABLE | UART_TX_ENABLE | UART_INT_RX_CHAR;
	// Set BRG for a 9600 baud rate for a 10 MHz PBCLK
	OpenUART2(u2mode, u2status, BRG_UART);
	ConfigIntUART2(UART_RX_INT_EN | UART_TX_INT_DIS | UART_INT_PR3);
	// Clear interrupt flag
	mU2RXClearIntFlag();
	// Write ">"
	WriteUART2(62);
	// Main while loop
	while(1) {
		// Read button states
		button1 = PORTReadBits(IOPORT_G, BIT_6);
		button2 = PORTReadBits(IOPORT_G, BIT_7);
		button3 = PORTReadBits(IOPORT_A, BIT_0);
		// Reset LED state
		LEDs = 0;
		// Reset current state
		currentState = 0;
		// Set state based on buttons
		// If button 1 pressed, increment by 1
		if(button1) {
			currentState += 1;
		} 
		// If button 2 pressed, increment by 2
		if(button2) {
			currentState += 2;
		}
		// If button 3 pressed, increment by 4
		if(button3) {
			currentState += 4;
		}
		DelayMs(50);
		// LED state loop
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

		// Write power level to LCD
		sprintf(str, "POWER LEVEL: %d", currentState);
		// Escape character
		SPI1BUF = 0x1b;
		// Clear screen and reset cursor
		c_buffer = "[j";
		putsSPI1(2, c_buffer);
		DelayMs(10);
		putsSPI1(strlen(str), str);
		DelayMs(100);
		
		// Write temperature to terminal
		printf("The current temperature is: %dF", temperature);
		WriteUART2(0xD);
		WriteUART2(0xA);

		// Overheating
		if(temperature > 80) {
			printf("Overheated!");
			DelayMs(10);
			putsSPI1(strlen(overheatedMessage), overheatedMessage);
			DelayMs(100);
			WriteUART2(0xD);
			WriteUART2(0xA);
			LEDs = 0;
		}

		// Display LEDs
		PORTWrite(IOPORT_E, LEDs);
	}
	return 0;
}

// Initialization for device I/O
void DeviceInit() {
	DDPCONbits.JTAGEN = 0;
	PORTSetPinsDigitalIn(IOPORT_G, BIT_6 | BIT_7);
	PORTSetPinsDigitalIn(IOPORT_A, BIT_0);
	PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTSetPinsDigitalOut(IOPORT_F, BIT_9);
	PORTClearBits(IOPORT_G, BIT_6 | BIT_7);
	PORTClearBits(IOPORT_A, BIT_0);
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