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
#define cntMsDelay 10000
void DeviceInit();
void DelayInit();
void DelayMs(int cms);
// Timer period for 0.5 seconds
#define T2_TICK (SYS_FREQ / 8 / 256 / 2)

// Global state for machine
unsigned currentState = 0;
// Global counters
int second = 0;
int LEDs = 0;
int LED4Counter = 0;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
	mT2ClearIntFlag();
	// Change 255 counter every second
	if((second % 2) == 0) {
		LEDs = 0;
		if(currentState == 255) {
			currentState = 0;
		}
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
		currentState++;
		// Reset loop at 255
		if(currentState == 255) {
			currentState = 0;
		}
	}
	// Flash LED 1 every 0.5 seconds
	PORTToggleBits(IOPORT_G, BIT_12);
	// Flash LED 4 every 4 seconds
	if(second == 8) {
		second = 0;
		PORTToggleBits(IOPORT_G, BIT_15);
	}
	// Increment every 0.5 seconds
	second++;
}

int main() {
	// Initialization
	DeviceInit();
	DelayInit();
	while(1) {
		Nop();
	}
}

// Initialization and delay functions
void DeviceInit() {
	DDPCONbits.JTAGEN = 0;
	PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTSetPinsDigitalOut(IOPORT_G, BIT_12 | BIT_15);
	PORTClearBits(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7);
	PORTClearBits(IOPORT_G, BIT_12 | BIT_15);
}

// Initialization for timer 2
void DelayInit() {
	unsigned int tcfg;
	tcfg = T2_ON|T2_SOURCE_INT|T2_PS_1_256;
	OpenTimer2(tcfg, T2_TICK);
	INTEnableSystemMultiVectoredInt();
	ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
	mT2ClearIntFlag();
}