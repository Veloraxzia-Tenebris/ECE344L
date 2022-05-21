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
#define PRESCALE 256
#define TOGGLES_PER_SEC 1
#define PB_DIV 8
#define T2_TICK (SYS_FREQ / PB_DIV / PRESCALE / TOGGLES_PER_SEC)
#define cntMsDelay 10000
#define cntMsDelay2 312
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
void T1Init();
void Delay(short int counts);

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
// Servo global variables
int servoflag = 0;

// Timer 1 ISR
void __ISR(_TIMER_1_VECTOR, ipl2) Timer1Handler(void) {
	mT1ClearIntFlag();
	servoflag = 1;
}

// Timer 2 ISR
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
	mT2ClearIntFlag();
	// Flash LED each second
	PORTToggleBits(IOPORT_G, BIT_12);
}

int main() {
	// Initialization
	DeviceInit();
	DelayInit();
	T1Init();
	// LEDs counter
	unsigned LEDs = 0;
	// SPI variables
	unsigned char buffer[40];
	unsigned char str[40];
	unsigned char overheatedMessage[40] = "SCRAM!";
	unsigned char* bufferp;
	unsigned char* c_buffer = "";
	int rData;
	// I2C variables
	int Index = 0, DataSz = 0;
	// Servo variables
	unsigned int tcfg2 = 0;
	short int servoCount = 0;
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
		servoCount = 180 + (short int)(byte1 * 4);
		if(servoflag) {
			PORTSetBits(IOPORT_D, BIT_1);
			// Servo PWM delay
			Delay(servoCount);
			PORTClearBits(IOPORT_D, BIT_1);
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
				PORTSetBits(IOPORT_D, BIT_1);
				// Set servo to max position
				Delay(80);
				PORTClearBits(IOPORT_D, BIT_1);
				DelayMs(20);
				// Flash "OVERHEATED!"
				putsSPI1(strlen(overheatedMessage), overheatedMessage);
				DelayMs(20);
				// Clear screen and reset cursor
				c_buffer = "[j";
				putsSPI1(2, c_buffer);
				DelayMs(20);
				// Clear screen and reset cursor
				c_buffer = "[j";
				putsSPI1(2, c_buffer);
				DelayMs(20);
				putsSPI1(strlen(str), str);
				DelayMs(20);
				// Turn off LEDs
				LEDs = 0;
			} else {
				// Write temperature level to LCD
				sprintf(str, "Temp: %d", temperature);
				// Clear screen and reset cursor
				c_buffer = "[j";
				putsSPI1(2, c_buffer);
				DelayMs(10);
				putsSPI1(strlen(str), str);
				DelayMs(10);
			}
			// Escape character
			SPI1BUF = 0x1b;
			// Clear screen and reset cursor
			c_buffer = "[j";
			putsSPI1(2, c_buffer);
			DelayMs(10);
			putsSPI1(strlen(str), str);
			DelayMs(10);

			servoflag = 0;
			
			// Display LEDs
			PORTWrite(IOPORT_E, LEDs);
		}
	}
	return 0;
}

// Initialization for device I/O
void DeviceInit() {
	DDPCONbits.JTAGEN = 0;
	PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTSetPinsDigitalOut(IOPORT_F, BIT_9);
	PORTSetPinsDigitalOut(IOPORT_D, BIT_1);
	PORTClearBits(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTClearBits(IOPORT_F, BIT_9);
	PORTClearBits(IOPORT_D, BIT_1);
}

void DelayInit() {
	unsigned int tcfg;
	unsigned int tcfg4;
	tcfg = T2_ON | T2_SOURCE_INT | T2_PS_1_256;
	tcfg4 = T4_ON | T4_SOURCE_INT | T4_PS_1_32;
	OpenTimer2(tcfg, T2_TICK);
	OpenTimer4(tcfg4, 0XFFFF);
	ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
	mT2ClearIntFlag();
	mT4ClearIntFlag();
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

void T1Init() {
	unsigned int tcfg1;
	// 20 ms
	tcfg1 = T1_ON | T1_SOURCE_INT | T1_PS_1_8;
	OpenTimer1(tcfg1, 0x61A8);
	INTEnableSystemMultiVectoredInt();
	ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
	mT1ClearIntFlag();
}

void Delay(short int counts) {
	WriteTimer4(0);
	while(ReadTimer4() < counts);
}