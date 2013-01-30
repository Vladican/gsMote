#include "Synch.h"
#include "E-000001-000009_firmware_rev_1_0.h"

uint16_t* moteID;
//const char ResetCommand[6] = {'r','e','s','e','t','\n'};
uint8_t messageSize = 8;
char buff[8];

void synch(int SynchPer){
	*ResetCommand = "reset";
	RadioMonitorMode = SYNCHED;		//initialize the RadioMonitorMode to synched 
	EVSYS.CH1MUX = EVSYS_CHMUX_TCC1_OVF_gc;	//set overflow of lower 16 bits of the counter as event on channel 1
	TCD1.CTRLA = TC_CLKSEL_EVCH1_gc; //select event channel 1 as input clock to the upper 16 bits of the counter
	TCD1.INTCTRLA = 0x03;	//enable timer overflow interrupt as high priority interrupt
	TCD1.PER = SynchPer*500; //upper 16 bits of the 32-bit joint timer
		
	TCC1.PER = 64000;	//64000 cycles of cpu at 32MHz equals one 500 Hz interval
	TCC1.INTCTRLA = 0x02;  //enable timer overflow interrupt as medium priority interrupt
	TCC1.CTRLA = 0x01;  //start timer with clock precision of cpu clock (32MHz)
	PMIC.CTRL = ENABLE_ALL_INTERRUPT_LEVELS;
	sei(); //  Enable global interrupts
}

ISR(TCD1_OVF_vect) {
	TCC1.CTRLA = 0x00; //turn off the counter while synching
	TCD1.CTRLA = 0x00;
	TCC1.CTRLFSET = 0x0C;	//reset the value of the counter to 0
	TCD1.CTRLFSET = 0x0C;
	chb_init();
	chb_set_short_addr(*moteID);
	char message[messageSize];
	strcpy(message,ResetCommand);
	itoa((int)(*moteID),buff,10);
	strcat(message,buff);
	//insert code here to send and wait for synch message
	//perhaps just disable ADC or ADC interrupts while waiting for reply instead of having the whole mote hang
	RadioMonitorMode = TIME_SYNCH;
	ADC_Pause_Sampling();	//pause the ADC while synching
	chb_write(0x0000,message,strlen(message));
}	

