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
#define cntMsDelay 10000
void DeviceInit();
void DelayInit();
void DelayMs(int cms);

int main() {
	// Button state variables
	int BTN1 = 0;
	int BTN2 = 0;
	int BTN3 = 0;
	// Initialization
	DeviceInit();
	DelayInit();
	// State for machine and LEDs
	unsigned currentState = 0;
	unsigned LEDs = 0;
	// Clean all bits
	PORTClearBits(IOPORT_G, BIT_12|BIT_13|BIT_14|BIT_15);
	// Quick delay before reading bits
	DelayMs(50);
	// Main loop
	while(1) {
		// Read button states
		BTN1 = PORTReadBits(IOPORT_G, BIT_6);
		BTN2 = PORTReadBits(IOPORT_G, BIT_7);
		BTN3 = PORTReadBits(IOPORT_A, BIT_0);
		// Reset LED state
		LEDs = 0;
		// Set state based on buttons
		// If BTN1 pressed, nickle
		if(BTN1 & 0x0040) {
			currentState += 1;
		} 
		// If BTN2 pressed, dime
		if(BTN2 & 0x0080) {
			currentState += 2;
		}
		// If BTN3 pressed, reset
		if(BTN3) {
			currentState = 8;
		}
		// Set LEDs based on state
		// XX1, any 5 cents
		if(currentState & 0x01) {
			// LED 1
			LEDs |= BIT_12;
		}
		// X1X, any 10 cents
		if(currentState & 0x02) {
			// LED 2
			LEDs |= BIT_13;
		}
		// 1XX, any 20 cents
		if(currentState & 0x04) {
			// LED 3
			LEDs |= BIT_14;
		}
		// Turn current state LEDs on for 250 ms
		PORTWrite(IOPORT_G, LEDs);
		DelayMs(250);
		// 110, exactly 30 cents, dispense item
		if((currentState & 0x02) && (currentState & 0x04)) {
			// Turn LED 4 on
			PORTWrite(IOPORT_G, BIT_15);
			DelayMs(1000);
			// If there's change, run the reset loop
			if(currentState > 6) {
				currentState = 8;
			} else {
				currentState = 0;
			}
		}
		// 1XXX, reset
		if(currentState == 8) {
			// Reset the current state
			currentState = 0;
			// On 250 ms
			PORTWrite(IOPORT_G, BIT_12|BIT_13|BIT_14);
			PORTClearBits(IOPORT_G, BIT_12|BIT_13|BIT_14);
			DelayMs(250);
			// Off 250 ms
			PORTWrite(IOPORT_G, BIT_12|BIT_13|BIT_14);
			DelayMs(250);
			// On 250 ms
			PORTWrite(IOPORT_G, BIT_12|BIT_13|BIT_14);
			PORTClearBits(IOPORT_G, BIT_12|BIT_13|BIT_14);
			DelayMs(250);
			// Off 250 ms
			PORTWrite(IOPORT_G, BIT_12|BIT_13|BIT_14);
			DelayMs(250);
		}
		// Delays for next loop and resetting stuff
		DelayMs(10);
		PORTClearBits(IOPORT_G, BIT_12|BIT_13|BIT_14|BIT_15);
		DelayMs(10);
	}
}

// Initialization and delay functions
void DeviceInit() {
	DDPCONbits.JTAGEN = 0;
	PORTSetPinsDigitalOut(IOPORT_G, BIT_12|BIT_13|BIT_14|BIT_15);
	PORTSetPinsDigitalIn(IOPORT_G, BIT_6|BIT_7);
	PORTSetPinsDigitalIn(IOPORT_A, BIT_0);
}

void DelayInit() {
	unsigned int tcfg;
	tcfg = T1_ON|T1_IDLE_CON|T1_SOURCE_INT|T1_PS_1_1|T1_GATE_OFF|T1_SYNC_EXT_OFF;
	OpenTimer1(tcfg, 0xFFFF);
}

void DelayMs(int cms) {
	int ims;
	for(ims = 0; ims < cms; ims++) {
		WriteTimer1(0);
		while(ReadTimer1() < cntMsDelay);
	}
}