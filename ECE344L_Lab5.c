#include <peripheral/system.h>
#include <plib.h>
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

// Function prototypes
void DeviceInit();

// Global machine state variable
unsigned currentState = 0;
// Global input character
unsigned inChar = 0;

// UART interrupt
void __ISR(_UART2_VECTOR, ipl3) Uart2Handler(void) {
	// Read from receive buffer
	inChar = ReadUART2();
	// Change only letters to lower case
	if((inChar < 91) && (inChar > 64)) {
		inChar += 32;
	}
	// Increment machine state
	currentState++;
	// Write to transmit buffer
	WriteUART2(inChar);
	// Clear interrupt flag
	mU2RXClearIntFlag();
}

int main() {
	// Initialization
	DeviceInit();
	// LEDs counter
	unsigned LEDs;
	// Open and configure UART
	unsigned long u2mode = UART_EN | UART_IDLE_CON | UART_RX_TX |
                		   UART_DIS_WAKE | UART_DIS_LOOPBACK |
                		   UART_DIS_ABAUD | UART_NO_PAR_8BIT | UART_EN_BCLK |
					   UART_1STOPBIT | UART_IRDA_DIS | UART_MODE_SIMPLEX |
               		   UART_NORMAL_RX | UART_BRGH_SIXTEEN;
	unsigned long u2status = UART_TX_PIN_LOW | UART_RX_ENABLE | UART_TX_ENABLE | UART_INT_RX_CHAR;
	// Set BRG for a 9600 baud rate for a 10 MHz PBCLK
	OpenUART2(u2mode, u2status, 64);
	ConfigIntUART2(UART_RX_INT_EN | UART_TX_INT_DIS | UART_INT_PR3);
	// Clear interrupt flag
	mU2RXClearIntFlag();
	// Write ">"
	WriteUART2(62);
	while(1) {
		// If statement cascade for bit masking
		LEDs = 0;
		// 1
		if(currentState & 0x01) {
			LEDs |= BIT_0;
		}
		// 2
		if(currentState & 0x02) {
			LEDs |= BIT_1;
		}
		// 4
		if(currentState & 0x04) {
			LEDs |= BIT_2;
		}
		// 8
		if(currentState & 0x08) {
			LEDs |= BIT_3;
		}
		// 16
		if(currentState & 0x10) {
			LEDs |= BIT_4;
		}
		// 32
		if(currentState & 0x20) {
			LEDs |= BIT_5;
		}
		// 64
		if(currentState & 0x40) {
			LEDs |= BIT_6;
		}
		// 128
		if(currentState & 0x80) {
			LEDs |= BIT_7;
		}
		PORTWrite(IOPORT_E, LEDs);
	}
	return 0;
}

// Initialization for device I/O
void DeviceInit() {
	PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTClearBits(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
}