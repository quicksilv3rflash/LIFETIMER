/*
 * lifetimer.c
 *
 * Created: 2016-04-09 08:50:39
 *  Author: Ondine
 */ 

/*
POWER CONSUMPTION (OF SOMEONE ELSE'S BOARD) WHILE USING TCCR0 ASNCHRONONOUS INTERRUPT TO UPDATE A VALUE
Without sleeping : 10.6mA
Sleeping with IDLE mode : 6.4mA
Sleeping with ADC mode: 3.1mA
Sleeping with EXT STANDBY mode: 0.5mA
Sleeping with POWER SAVE mode: 0.4mA
Sleeping with STANDBY mode: 0.5mA (Asynchronous timer no longer works)
Sleeping with POWER DOWN mode: 0.3mA (Asynchronous timer no longer works)
*/

#define F_CPU 1000000UL


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>

//right now I have about 68.07151992 % of my life remaining. The last digit should be decremented 4 times a second,
//assuming I have 2,500,000,000 seconds to live total (~79.2219 years, a convenient estimate for coding)
//the percentage will be in the form: AB.CDEFGHIJ
volatile uint8_t AB = 68;
volatile uint8_t CD = 07;
volatile uint8_t EF = 15;
volatile uint8_t GH = 19;
volatile uint8_t IJ = 96;


volatile uint8_t number_of_frames_displayed = 0;
volatile uint8_t num_overflows = 0;
volatile uint8_t seconds = 00;
volatile uint8_t minutes = 33;
volatile uint8_t hours = 13;

//SPI init
void SPIMasterInit(void)
{
	//set MOSI, SCK and SS as output
	DDRB |= (1<<PB3)|(1<<PB5)|(1<<PB2);
	//set SS to high
	PORTB |= (1<<PB2);
	//enable master SPI at clock rate Fck/16
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

//master send function
void SPIMasterSend(uint8_t data)
{
	//select slave
	PORTB &= ~(1<<PB2);
	//send data
	SPDR=data;
	//wait for transmition complete
	while (!(SPSR &(1<<SPIF)));
	//SS to high
	PORTB |= (1<<PB2);
}

//send_command(uint8_t); send COMMAND to the screen
void send_command(uint8_t data){
PORTB &= 0xFE; // pull PORTB0 low to indicate command
SPIMasterSend(data);
PORTB |= 0x01; // pull PORTB0 high to indicate data
}


//send_data(uint8_t); send DATA to the screen
void send_data(uint8_t data){
PORTB |= 0x01; // pull PORTB0 high to indicate data
SPIMasterSend(data);
PORTB &= 0xFE; // pull PORTB0 low to indicate command	
}


//below are draw functions. all start with a space.

void draw_zero(void){
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
}

void draw_one(void){
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x3E);
}

void draw_two(void){
	send_data(0x00);
	send_data(0x3A);
	send_data(0x2A);
	send_data(0x2E);
}

void draw_three(void){
	send_data(0x00);
	send_data(0x2A);
	send_data(0x2A);
	send_data(0x3E);
}

void draw_four(void){
	send_data(0x00);
	send_data(0x0E);
	send_data(0x08);
	send_data(0x3E);
}

void draw_five(void){
	send_data(0x00);
	send_data(0x2E);
	send_data(0x2A);
	send_data(0x3A);
}

void draw_six(void){
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x3A);
}

void draw_seven(void){
	send_data(0x00);
	send_data(0x02);
	send_data(0x02);
	send_data(0x3E);
}

void draw_eight(void){
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x3E);
}

void draw_nine(void){
	send_data(0x00);
	send_data(0x0E);
	send_data(0x0A);
	send_data(0x3E);
}

void draw_period(void){
	send_data(0x00);
	send_data(0x20);
}

//draw_percent(void); draws a percent symbol on the screen
void draw_percent(void){
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x00);
	send_data(0x42);
	send_data(0x25);
	send_data(0x12);
	send_data(0x08);
	send_data(0x24);
	send_data(0x52);
	send_data(0x21);
	send_data(0x00);
	send_data(0x22);
	send_data(0x3E);
}

//fills one line with the message "TIME IS RUNNING OUT"
void draw_timeisrunningout(void){
	//time
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x00);
	
	//is
	send_data(0x3E);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x00);
	
	//running
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3A);
		//out
	send_data(0x00);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
}

//draw digit selects the correct draw function for the given digit
void draw_digit(uint8_t digit){
	if(digit == 0){
		draw_zero();
		}else if(digit == 1){
		draw_one();
		}else if(digit == 2){
		draw_two();
		}else if(digit == 3){
		draw_three();
		}else if(digit == 4){
		draw_four();
		}else if(digit == 5){
		draw_five();
		}else if(digit == 6){
		draw_six();
		}else if(digit == 7){
		draw_seven();
		}else if(digit == 8){
		draw_eight();
		}else if(digit == 9){
		draw_nine();
	}
}

//void displaytimeunit(uint8_t timeunit); displays time unit
void displaytimeunit(uint8_t timeunit){
uint8_t timeunit_now = timeunit; //stores volatile into timeunit_now which will be manipulated for display
draw_digit((timeunit_now - (timeunit_now % 10))/10);
draw_digit(timeunit_now % 10);
}
	
	
//void refresh_screen(void); refreshes the screen
void refresh_screen(void){
	uint8_t j=0;
	for(j=0;j<6;j++){
		send_command(0xB0 | j); //set page address (0xb0 | address)
		
		uint8_t add = 0x00; //column address
		send_command((0x10|(add>>4))+0x02); //set column address part one
		send_command((0x0f&add));//set column address part two
		/*
		uint8_t i=0;
		for(i=0;i<64;i++){
			send_data(screenmemory[i+64*j]);
		}
		*/
		if(j == 0){
			displaytimeunit(AB);
			draw_period();
			displaytimeunit(CD);
			displaytimeunit(EF);
			displaytimeunit(GH);
			displaytimeunit(IJ);	
			
			for(uint8_t i=0;i<2;i++){
				send_data(0x00);
				send_data(0x00);
				send_data(0x00);
				send_data(0x00);
			}
			draw_percent();
		}
		if(j==1){
			draw_timeisrunningout();
		}
		if(j==2){
			displaytimeunit(hours);
			send_data(0x00);
			send_data(0x22);
			send_data(0x00);
			displaytimeunit(minutes);
			send_data(0x00);
			send_data(0x22);
			send_data(0x00);
			displaytimeunit(seconds);	
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
		}		
	}
}

//INTERRUPT SERVICE ROUTINE FOR PIN CHANGE (BUTTON PRESS)
ISR(PCINT2_vect) {
	
	number_of_frames_displayed = 0;
	//resetting this variable to zero causes the screen to be turned on for a bit while it's re-incremented
}

//Timer2 init according to datasheet
void RTCInit(void){
	//Disable timer2 interrupts
	TIMSK2  = 0;
	//Enable asynchronous mode
	ASSR  = (1<<AS2);
	//set initial counter value
	TCNT2=0;
	//set prescaler 32 (update every 1/4 second)
	TCCR2B |= (1<<CS21)|(1<<CS00);
	//wait for registers update
	while (ASSR & ((1<<TCN2UB)|(1<<TCR2BUB)));
	//clear interrupt flags
	TIFR2  = (1<<TOV2);
	//enable TOV2 interrupt
	TIMSK2  = (1<<TOIE2);
}

//Overflow ISR
ISR(TIMER2_OVF_vect){
	uint8_t frames_until_turnoff = 35;
	
	if(number_of_frames_displayed == 0){
		send_command(0xAF); //turn the display on
	}	
	if(number_of_frames_displayed < frames_until_turnoff){
		refresh_screen();
		number_of_frames_displayed++;
	}else if(number_of_frames_displayed >= frames_until_turnoff){
		send_command(0xAE); //display off command
	}
	
	IJ--;
	if(IJ == 0){
		IJ = 99;
		GH--;
		if(GH == 0){
			GH = 99;
			EF--;
			if(EF == 0){
				EF = 99;
				CD--;
				if(CD == 0){
					CD = 99;
					AB--;
				}
			}
		}
	}
	
	
	num_overflows++;
	if(num_overflows % 4 == 0){
		seconds++;
		if(seconds == 60){
			seconds = 0;
			minutes++;
			if(minutes == 60){
				minutes = 0;
				hours++;
				if(hours == 24){
					hours = 0;
				}
			}
		}
	}
}

int main(void){
	DDRB = 0x2F; //Set D/C, RST#, CS#, MOSI, SCK as Output
	DDRD = 0x00; //PIND3=BUTTON_A, PIND2=BUTTON_B, PIND3=BUTTIONC
	
	RTCInit();
	// Turn interrupts on.
	sei();
	
	//set sleep mode to minimum power while allowing interrupt
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	PCICR |= _BV(PCIE2); //PIN CHANGE INTERRUPT CONTROL REGISTER (PIN BLOC PCINT16-23)
	PCMSK2 |= _BV(PCINT18); //PIN CHANGE interrupt MASK activate pin for BUTTON_B
	
	//initialize master SPI
	SPIMasterInit();

	// Display reset routine (before initialization)
	PORTB |= 0x02;	// Initially set RST HIGH
	_delay_ms(5);	// VDD (3.3V) goes high at start, lets just chill for 5 ms
	PORTB &= 0xFC;	// Bring RST low, reset the display
	_delay_ms(10);	// wait 10ms
	PORTB |= 0x02;	// Set RST HIGH, bring out of reset
	
	
	// Display initialization routine (after reset)
	send_command(0x8D); //set charge pump
	send_command(0x14); //to some setting that's default in the sparkfun arduino library
	
	send_command(0xAF); //turn the display on
	
	send_command(0xB0); // sets to memory page 0 (0xB0 | desired page number)
	
	uint8_t j=0;
	for(j=0;j<6;j++){
	send_command(0xB0 | j);
	uint8_t i=0;
	for(i=0;i<128;i++){
	send_data(i%16);
	}
	}
	
	while(1){




		sleep_mode();

		
		
	
		/*
		//sending these commands inhibits screen display temporarily
		send_command(0xD3);		// SET DISPLAY OFFSET
		send_command(0x00);					// no offset
		
		send_command(0x40 | 0x00); // SETSTARTLINE to line #0
		//end inhibiting commands
		//*/
		/*
		if((PIND & 0x08) == 0x08){
			send_command(0xAF); //turn the display on
				uint8_t j=0;
				for(j=0;j<6;j++){
					send_command(0xB0 | j);
					uint8_t i=0;
					for(i=0;i<128;i++){
						send_data(i%16);
					}
				}
		}
		*/
		
		/*
		if((PIND & 0x04) == 0x04){
			send_command(0xAF); //turn the display on
				uint8_t j=0;
				for(j=0;j<6;j++){
					send_command(0xB0 | j);
					uint8_t i=0;
					for(i=0;i<128;i++){
						send_data(0xAA);
					}
				}
				
		}*/
		
		/*
		if((PIND & 0x02) == 0x02){
			send_command(0xAF); //turn the display on
				uint8_t j=0;
				for(j=0;j<6;j++){
					send_command(0xB0 | j);
					uint8_t i=0;
					for(i=0;i<128;i++){
						send_data(2*i + 1);
					}
				}
		}
		*/
	}
}



/*
options from which to choose for orders:

TEXT "ORDERS FROM MACHINE EMPIRE HIGH COMMAND"

 Go to a place (randomly generated GPS coordinates)
 Stop and meditate
 Get [ randomly generated word ]
 Take care of yourself
 Climb Mars Hill
 Benefit nearest person
 Change bearing to (N, S, E, W, NE, NW, SE, SW) until mission is clear
 Fix something
*/

/*This code is the MINIMUM to get the display to turn on and display "snow" (random GDDRAM contents)
	// Display reset routine (before initialization)
	PORTB |= 0x02;	// Initially set RST HIGH
	_delay_ms(5);	// VDD (3.3V) goes high at start, lets just chill for 5 ms
	PORTB &= 0xFC;	// Bring RST low, reset the display
	_delay_ms(10);	// wait 10ms
	PORTB |= 0x02;	// Set RST HIGH, bring out of reset
	
	
	// Display initialization routine (after reset)
	send_command(0x8D); //set charge pump
	send_command(0x14); //to some setting that's default in the sparkfun arduino library
	
	send_command(0xAF); //turn the display on
	*/