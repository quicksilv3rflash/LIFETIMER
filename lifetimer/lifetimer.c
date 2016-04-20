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

volatile uint8_t num_overflows = 0;

static uint8_t screenmemory [] = {
	/* LCD Memory organised in 64 horizontal pixel and 6 rows of byte
	 B  B .............B  -----
	 y  y .............y        \
	 t  t .............t         \
	 e  e .............e          \
	 0  1 .............63          \
	                                \
	 D0 D0.............D0            \
	 D1 D1.............D1            / ROW 0
	 D2 D2.............D2           /
	 D3 D3.............D3          /
	 D4 D4.............D4         /
	 D5 D5.............D5        /
	 D6 D6.............D6       /
	 D7 D7.............D7  ----
	*/
	//SparkFun Electronics LOGO

	// ROW0, BYTE0 to BYTE63
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x07, 0x07, 0x06, 0x06, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// ROW1, BYTE64 to BYTE127
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x07, 0x0F, 0x3F, 0x3F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFC, 0xFC, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFC, 0xF8, 0xE0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// ROW2, BYTE128 to BYTE191
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC,
	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF1, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xF0, 0xFD, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// ROW3, BYTE192 to BYTE255
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x1F, 0x07, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// ROW4, BYTE256 to BYTE319
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x1F, 0x1F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, 0x07, 0x07, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// ROW5, BYTE320 to BYTE383
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
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

//INTERRUPT SERVICE ROUTINE
ISR(PCINT2_vect) {
	send_command(0xAF); //turn the display on
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
		if(j==1){
			draw_timeisrunningout();
		}else{
				draw_zero();
				draw_one();
				draw_period();
				draw_two();
				draw_three();
				draw_four();
				draw_five();
				draw_six();
				draw_seven();
				
				draw_eight();
				draw_nine();
				for(uint8_t i=0;i<2;i++){
				send_data(0x00);
				send_data(0x00);
				send_data(0x00);
				send_data(0x00);
				}
				draw_percent();
		}
		
	}
	for(j=0;j<10;j++){
	_delay_ms(200); // Giant delay
	}
}

//Timer2 init according to datasheet
void RTCInit(void){
	//Disable timer2 interrupts
	TIMSK2  = 0;
	//Enable asynchronous mode
	ASSR  = (1<<AS2);
	//set initial counter value
	TCNT2=0;
	//set prescaler 128
	TCCR2B |= (1<<CS22)|(1<<CS00);
	//wait for registers update
	while (ASSR & ((1<<TCN2UB)|(1<<TCR2BUB)));
	//clear interrupt flags
	TIFR2  = (1<<TOV2);
	//enable TOV2 interrupt
	TIMSK2  = (1<<TOIE2);
}

//Overflow ISR
ISR(TIMER2_OVF_vect){
	num_overflows++;
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
		send_command(0xAE); //display off command
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