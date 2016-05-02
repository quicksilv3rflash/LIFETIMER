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

//right now I have about 68.03293884 % of my life remaining. The last digit should be decremented 4 times a second,
//assuming I have 2,500,000,000 seconds to live total (~79.2219 years, a convenient estimate for coding)
//the percentage is calculated by (2,500,000,000 - #of seconds since january 4th 1991)/2,500,000,000
//the percentage will be in the form: AB.CDEFGHIJ, and the J digit will be decremented every 1/4 second.
volatile uint8_t AB = 68;
volatile uint8_t CD = 03;
volatile uint8_t EF = 29;
volatile uint8_t GH = 34;
volatile uint8_t IJ = 84;

volatile uint8_t displayvar = 0; 
//if 0, display nothing/new order message
//if 1, display current MEHC order
//if 2, display previous MEHC order

volatile uint8_t order_entry_mode_flag = 0;
volatile uint8_t rand_int_locked = 0;
volatile uint8_t rand_int_loaded = 0;

volatile uint8_t order_array_index = 0;
volatile uint8_t hours_until_next_MEHC_order = 0;
volatile uint8_t current_MEHC_order[] = {0,0,0,0,0,0,0,0,0,0};
volatile uint8_t previous_MEHC_order[] = {0,0,0,0,0,0,0,0,0,0};

volatile uint8_t number_of_frames_displayed = 0;
volatile uint8_t num_overflows = 0;
volatile uint8_t seconds = 00;
volatile uint8_t minutes = 30;
volatile uint8_t hours = 17;
volatile uint8_t days = 1;
volatile uint8_t months = 5;
volatile uint8_t years = 16; //rofl Y2K bug again

uint32_t nucleus_north[] = {23641,23414,22693,22490,21475,21442,21389,21372,21427,21431,21432,20414,20453,20451,20509,20470,20400,19569,19534,19558,19597,19462,19464,18256,18315,18213,17380,17303,17426,16384,16437,16374};
uint32_t nucleus_east[] = {67113,57203,65765,57895,65426,64234,63204,61635,60309,59019,59746,65479,64260,63178,61672,60365,59129,65421,64271,63176,61713,60428,59239,67567,66172,64987,67577,66127,65024,67582,66178,65067};

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

//fills one line with them message "NEW ORDERS FROM"
void draw_newordersfrom(void){
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	//new
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x10);
	send_data(0x08);
	send_data(0x10);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//orders
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x1C);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	//from
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
		
}

//fills one line with the message "MACHINE EMPIRE"
void draw_machineempire(void){
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//machine
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x00);
	send_data(0x3E);
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
	send_data(0x2A);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	
	//empire
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
}

//fills one line with the message "HIGH COMMAND"
void draw_highcommand(void){
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//high
	send_data(0x3E);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//command
	send_data(0x3E);
	send_data(0x22);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x1C);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);

		
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

//void draw_letter(index); draws a letter corresponding to the index. 0=A, 1=B, etc. etc. 25=Z
//all letters are six pixels wide with the last pixel being a space
void draw_letter(uint8_t index){
	if(index == 0){//a
		send_data(0x3E);
		send_data(0x0A);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 1){//b
		send_data(0x3E);
		send_data(0x2A);
		send_data(0x14);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 2){//c
		send_data(0x3E);
		send_data(0x22);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);		
	}else if(index == 3){//d
		send_data(0x3E);
		send_data(0x22);
		send_data(0x1C);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 4){//e
		send_data(0x3E);
		send_data(0x2A);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 5){//f
		send_data(0x3E);
		send_data(0x0A);
		send_data(0x0A);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);		
	}else if(index == 6){//g
		send_data(0x3E);
		send_data(0x22);
		send_data(0x3A);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 7){//h
		send_data(0x3E);
		send_data(0x08);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 8){//i
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 9){//j
		send_data(0x30);
		send_data(0x20);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 10){//k
		send_data(0x3E);
		send_data(0x1C);
		send_data(0x36);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 11){//L
		send_data(0x3E);
		send_data(0x20);
		send_data(0x20);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 12){//m
		send_data(0x3E);
		send_data(0x04);
		send_data(0x08);
		send_data(0x04);
		send_data(0x3E);
		send_data(0x00);		
	}else if(index == 13){//n
		send_data(0x3E);
		send_data(0x04);
		send_data(0x08);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 14){//o
		send_data(0x3E);
		send_data(0x22);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 15){//p
		send_data(0x3E);
		send_data(0x0A);
		send_data(0x0E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 16){//Q
		send_data(0x3E);
		send_data(0x72);
		send_data(0x26);
		send_data(0x3C);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 17){//r
		send_data(0x3E);
		send_data(0x1A);
		send_data(0x2E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 18){//s
		send_data(0x2C);
		send_data(0x2A);
		send_data(0x1A);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 19){//T
		send_data(0x02);
		send_data(0x3E);
		send_data(0x02);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 20){//u
		send_data(0x3E);
		send_data(0x20);
		send_data(0x3E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 21){//v
		send_data(0x1E);
		send_data(0x20);
		send_data(0x1E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 22){//w
		send_data(0x3E);
		send_data(0x10);
		send_data(0x08);
		send_data(0x10);
		send_data(0x3E);
		send_data(0x00);
	}else if(index == 23){//x
		send_data(0x22);
		send_data(0x14);
		send_data(0x08);
		send_data(0x14);
		send_data(0x22);
		send_data(0x00);
	}else if(index == 24){//y
		send_data(0x0E);
		send_data(0x38);
		send_data(0x0E);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}else if(index == 25){//z
		send_data(0x32);
		send_data(0x2A);
		send_data(0x26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
	}			
}
	
//void draw_blankline(void); draws a blank line
void draw_blankline(void){
	uint8_t k = 0;
	for(k=0;k<64;k++){
		send_data(0x00);
	}
}

//void draw_stopandmeditate(void); fills one line with the message "STOP AND MEDITATE"	
void draw_stopandmeditate(void){
	//stop
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//and
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x1C);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	//meditate
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x1C);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
}

void draw_tendself(void){
	for(uint8_t s=0; s<13; s++){
		send_data(0x00);
	}
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x1C);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x20);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0A);
	for(uint8_t w=0;w<16;w++){
		send_data(0x00);
	}
}

void draw_climbmarshill(void){
	uint8_t i=0;	
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x20);
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
	send_data(0x14);
	for(i=0;i<7;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x04);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	for(i=0;i<7;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x20);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x20);
	send_data(0x20);
	
	send_data(0x00);
}

void draw_benefitnearest(void){
	for(uint8_t j=0;j<4;j++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x14);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	for(uint8_t r=0;r<6;r++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	for(uint8_t b=0; b<4;b++){
		send_data(0x00);
	}
	
}

void draw_person(void){
	uint8_t i=0;
	for(i=0;i<19;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
	send_data(0x00);
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x04);
	send_data(0x08);
	send_data(0x3E);
	for(i=0;i<22;i++){
		send_data(0x00);
	}
}

	
void draw_fixsomething(void){
	uint8_t i=0;
	for(i=0;i<6;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x0A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x22);
	send_data(0x14);
	send_data(0x08);
	send_data(0x14);
	send_data(0x22);
	for(i=0;i<10;i++){
	send_data(0x00);
	}
	send_data(0x2C);
	send_data(0x2A);
	send_data(0x1A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
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
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
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

	for(i=0;i<5;i++){
		send_data(0x00);
	}
}
	
void draw_goto(void){
	uint8_t i=0;
	for(i=0;i<19;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	for(i=0;i<6;i++){
		send_data(0x00);
	}
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	for(i=0;i<24;i++){
		send_data(0x00);
	}
}

void draw_deg_n(uint8_t current_or_past){//1=current, 2=past, uses MEHC_order[2] to set nucleus, MEHC_order[3] to vary N/S
	uint8_t nucleus_index = 0;
	uint8_t variance = 0;
	// draws "+"
	send_data(0x00);
	send_data(0x08);
	send_data(0x1C);
	send_data(0x08);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	//draws "35."
	displaytimeunit(35);
	draw_period();

	if(current_or_past == 1){//if current
		nucleus_index = (current_MEHC_order[2] % 32);
		variance = current_MEHC_order[3];		
	}
	if(current_or_past == 2){//if past
		nucleus_index = (previous_MEHC_order[2] % 32);
		variance = previous_MEHC_order[3];	
	}
	
		//modifiable display data goes here
	if(variance > 126){
		displaytimeunit((uint8_t) (((nucleus_north[nucleus_index] / 10) + (variance % 64)) / 100));
		displaytimeunit((uint8_t) (((nucleus_north[nucleus_index] / 10) + (variance % 64)) % 100));
	}else{
		displaytimeunit((uint8_t) (((nucleus_north[nucleus_index] / 10) - (variance % 64)) / 100));
		displaytimeunit((uint8_t) (((nucleus_north[nucleus_index] / 10) - (variance % 64)) % 100));
	}
	
	
	
	draw_digit((uint8_t) (nucleus_north[nucleus_index] % 10));
	
	//draws "[ {deg}N ]"
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	send_data(0x3E);
	send_data(0x22);
	
	send_data(0x00);
	send_data(0x00);
	
	send_data(0x02);
	send_data(0x05);
	send_data(0x02);
	send_data(0x00);
	
	draw_letter(13);
	
	send_data(0x00);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
}

void draw_deg_e(uint8_t current_or_past){//1=current, 2=past, uses MEHC_order[2] to set nucleus, MEHC_order[4] to vary E/W
	uint8_t nucleus_index = 0;
	uint8_t variance = 0;
	// draws "-1"
	send_data(0x00);
	send_data(0x08);
	send_data(0x08);
	send_data(0x08);
	send_data(0x00);
	send_data(0x00);
	send_data(0x3E);
	
	//draws "11."
	displaytimeunit(11);
	draw_period();
	

	
	if(current_or_past == 1){//if current
		nucleus_index = (current_MEHC_order[2] % 32);
		variance = current_MEHC_order[4];		
	}
	if(current_or_past == 2){//if past
		nucleus_index = (previous_MEHC_order[2] % 32);
		variance = previous_MEHC_order[4];		
	}
	
		//modifiable display data goes here
	if(variance > 126){
		displaytimeunit((uint8_t) (((nucleus_east[nucleus_index] / 10) + (variance % 64)) / 100));
		displaytimeunit((uint8_t) (((nucleus_east[nucleus_index] / 10) + (variance % 64)) % 100));
		}else{
		displaytimeunit((uint8_t) (((nucleus_east[nucleus_index] / 10) - (variance % 64)) / 100));
		displaytimeunit((uint8_t) (((nucleus_east[nucleus_index] / 10) - (variance % 64)) % 100));
	}
	
	
	draw_digit((uint8_t) (nucleus_east[nucleus_index] % 10));
	
	//draws "[ {deg}E ]"
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	
	send_data(0x3E);
	send_data(0x22);
	
	send_data(0x00);
	send_data(0x00);
	
	send_data(0x02);
	send_data(0x05);
	send_data(0x02);
	send_data(0x00);
	
	draw_letter(4);
	
	send_data(0x00);
	send_data(0x22);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
}
	
void draw_getxxxx(uint8_t current_or_past){ //1=current, 2=past
	uint8_t i=0;
	for(i=0;i<5;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	for(i=0;i<16;i++){
		send_data(0x00);
	}
	
	if(current_or_past == 1){//if current
		draw_letter(current_MEHC_order[2] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(current_MEHC_order[3] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(current_MEHC_order[4] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(current_MEHC_order[5] % 26);
	}
	if(current_or_past == 2){//if past
		draw_letter(previous_MEHC_order[2] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(previous_MEHC_order[3] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(previous_MEHC_order[4] % 26);
		send_data(0x00);
		send_data(0x00);
		send_data(0x00);
		draw_letter(previous_MEHC_order[5] % 26);
	}
}
	
	
void draw_changebearing(void){
	uint8_t i=0;
	for(i=0;i<4;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x22);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x08);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
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
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	for(i=0;i<8;i++){
		send_data(0x00);
	}
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x14);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x2A);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x0A);
	send_data(0x3E);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x1A);
	send_data(0x2E);
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
	for(i=0;i<5;i++){
		send_data(0x00);
	}	
}
	
void draw_toxx(uint8_t current_or_past){//1=current, 2=past
	uint8_t i=0;
	uint8_t k = 0;
	for(i=0;i<12;i++){
		send_data(0x00);
	}
	send_data(0x02);
	send_data(0x3E);
	send_data(0x02);
	send_data(0x00);
	send_data(0x3E);
	send_data(0x22);
	send_data(0x3E);
	for(i=0;i<17;i++){
		send_data(0x00);
	}
	
	if(current_or_past == 1){ // current
		//N S E W NE NW SE SW go here
		if((current_MEHC_order[2] % 8) == 0){
			//n
			draw_letter(13);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((current_MEHC_order[2] % 8) == 1){
			//s
			draw_letter(18);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((current_MEHC_order[2] % 8) == 2){
			//e
			draw_letter(4);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((current_MEHC_order[2] % 8) == 3){
			//w
			draw_letter(22);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((current_MEHC_order[2] % 8) == 4){
			//ne
			draw_letter(13);
			draw_letter(4);
		}
		if((current_MEHC_order[2] % 8) == 5){
			//nw
			draw_letter(13);
			draw_letter(22);
		}
		if((current_MEHC_order[2] % 8) == 6){
			//se
			draw_letter(18);
			draw_letter(3);
		}
		if((current_MEHC_order[2] % 8) == 7){
			//sw
			draw_letter(18);
			draw_letter(22);
		}
	}
	
	if(current_or_past == 2){ //past
		//N S E W NE NW SE SW go here
		if((previous_MEHC_order[2] % 8) == 0){
			//n
			draw_letter(13);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((previous_MEHC_order[2] % 8) == 1){
			//s
			draw_letter(18);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((previous_MEHC_order[2] % 8) == 2){
			//e
			draw_letter(4);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((previous_MEHC_order[2] % 8) == 3){
			//w
			draw_letter(22);
			for(k=0;k<6;k++){
				send_data(0x00);
			}
		}
		if((previous_MEHC_order[2] % 8) == 4){
			//ne
			draw_letter(13);
			draw_letter(4);
		}
		if((previous_MEHC_order[2] % 8) == 5){
			//nw
			draw_letter(13);
			draw_letter(22);
		}
		if((previous_MEHC_order[2] % 8) == 6){
			//se
			draw_letter(18);
			draw_letter(3);
		}
		if((previous_MEHC_order[2] % 8) == 7){
			//sw
			draw_letter(18);
			draw_letter(22);
		}
	}
	
	for(i=0;i<16;i++){
		send_data(0x00);
	}
}
		
/*
options from which to choose for orders:

TEXT "ORDERS FROM MACHINE EMPIRE HIGH COMMAND"

ORDER #
0) Stop and meditate
1) Tend self
2) Climb Mars Hill
3) Benefit nearest person
4) Fix something
5) Go to a place (randomly generated GPS coordinates) 
6) Get [ randomly generated 4-letter word ] (GET is chosen because it can mean "become", "obtain", or "comprehend") 
7) Change bearing to (N, S, E, W, NE, NW, SE, SW) until mission is clear 
*/

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
			displaytimeunit(20);
			displaytimeunit(years);
			send_data(0x00);
			send_data(0x08);
			send_data(0x08);
			displaytimeunit(months);
			send_data(0x00);
			send_data(0x08);
			send_data(0x08);
			displaytimeunit(days);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			send_data(0x00);
			displaytimeunit(hours);
			send_data(0x00);
			send_data(0x38);
			send_data(0x10);
			send_data(0x38);
			displaytimeunit(minutes);
		}		
		if(j==3){
			if((displayvar == 0) && (hours_until_next_MEHC_order > 0)){
				draw_blankline();
			}
			if((displayvar == 0) && (hours_until_next_MEHC_order == 0)){
				draw_newordersfrom();
			}
			if((displayvar == 1) && (current_MEHC_order[0] != 5)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] != 5)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 5)){
				draw_goto();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 5)){
				draw_goto();
			}			
		}
		
		if(j==4){
			if((displayvar == 0) && (hours_until_next_MEHC_order > 0)){
				draw_blankline();
			}
			if((displayvar == 0) && (hours_until_next_MEHC_order == 0)){
				draw_machineempire();
			}
			//current order
			if((displayvar == 1) && (current_MEHC_order[0] == 0)){
				draw_stopandmeditate();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 1)){
				draw_tendself();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 2)){
				draw_climbmarshill();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 3)){
				draw_benefitnearest();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 4)){
				draw_fixsomething();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 5)){
				draw_deg_n(1);
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 6)){
				draw_getxxxx(1); //1=current, 2=past | oh boy I get to learn about feeding pointers to arrays into functions!!! ^.^ (BUT NOT TODAY JUST MORE KLUDGES)
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 7)){
				draw_changebearing();
			}
			//previous order
			if((displayvar == 2) && (previous_MEHC_order[0] == 0)){
				draw_stopandmeditate();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 1)){
				draw_tendself();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 2)){
				draw_climbmarshill();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 3)){
				draw_benefitnearest();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 4)){
				draw_fixsomething();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 5)){
				draw_deg_n(2);
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 6)){
				draw_getxxxx(2); //1=current, 2=past
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 7)){
				draw_changebearing();
			}
		}
		
		if(j==5){
			
			if((displayvar == 0) && (hours_until_next_MEHC_order > 0)){
				draw_blankline();
			}
			if((displayvar == 0) && (hours_until_next_MEHC_order == 0)){
				draw_highcommand();
			}
			//current order
			if((displayvar == 1) && (current_MEHC_order[0] == 0)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 1)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 2)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 3)){
				draw_person();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 4)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 5)){
				draw_deg_e(1);
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 6)){
				draw_blankline();
			}
			if((displayvar == 1) && (current_MEHC_order[0] == 7)){
				draw_toxx(1);
			}
			//previous order
			if((displayvar == 2) && (previous_MEHC_order[0] == 0)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 1)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 2)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 3)){
				draw_person();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 4)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 5)){
				draw_deg_e(2);
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 6)){
				draw_blankline();
			}
			if((displayvar == 2) && (previous_MEHC_order[0] == 7)){
				draw_toxx(2);
			}
			
		}
		//---
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
	
	if(((PIND & 0x02) == 0x02) && (hours_until_next_MEHC_order == 0)){ //whatever trigger condition for order entry mode goes here
		order_array_index = 0;
		order_entry_mode_flag = 1;
		
//		sleep_disable();
	}
	
	if(((PIND & 0x08) == 0x08) && (order_entry_mode_flag == 0)){
		displayvar++;
		if(displayvar >= 3){
			displayvar = 0;
		}
	}
	
	
	if((rand_int_loaded == 1) && (order_entry_mode_flag == 1)){
		if(order_array_index == 0){
			uint8_t r = 0;
			for(r=0;r<10;r++){
				previous_MEHC_order[r] = current_MEHC_order[r];
			}			
		current_MEHC_order[order_array_index] = (rand_int_locked % 8);
		order_array_index++;
		}
		if(order_array_index > 0){
			current_MEHC_order[order_array_index] = rand_int_locked;
			order_array_index++;
		}
		
		///*uncommenting this will set the timer to actually delay the orders
		if(order_array_index >= 7){
			hours_until_next_MEHC_order = current_MEHC_order[6];			
		}
		//uncommenting the above snippet sets timer to actually delay orders*/
		
		if(order_array_index >= 8){ 
			//this was a 5 before, but it caused me to write outside of the array and mess up the variable hours_until_next_MEHC_order...
			//so I've kludgily extended the array size to work around the effect
			order_entry_mode_flag = 0;
		}
		rand_int_loaded = 0;
	}
	
	if(number_of_frames_displayed == 0){
		send_command(0xAF); //turn the display on
	}	
	if(number_of_frames_displayed < frames_until_turnoff){
		refresh_screen();
		number_of_frames_displayed++;
	}else if((number_of_frames_displayed >= frames_until_turnoff) && (order_entry_mode_flag == 0)){
		send_command(0xAE); //display off command
	}
	
	IJ--;
	if(IJ == 255){
		IJ = 99;
		GH--;
		if(GH == 255){
			GH = 99;
			EF--;
			if(EF == 255){
				EF = 99;
				CD--;
				if(CD == 255){
					CD = 99;
					AB--; //I should be dead by the time this function breaks
				}
			}
		}
	}
	
	
	num_overflows++; //overflows happen every quarter second
	if(num_overflows % 4 == 0){
		seconds++;
		if(seconds == 60){
			seconds = 0;
			minutes++;
			if(minutes == 60){
				minutes = 0;
				hours++;
				if(hours_until_next_MEHC_order != 0){
					hours_until_next_MEHC_order--;
				}				
				if(hours == 24){
					hours = 0;
					days++;
					if((days == 32) || 
					((days == 31) && ((months == 4) || (months == 6) || (months == 9) || (months == 11))) ||
					((months == 2) && (days == 29) && ((years % 4) != 0)) ||
					((months == 2) && (days == 30) && (years % 4 == 0))){ // note that this code only checks if year % 4 == 0. I should be dead before it breaks.
						days = 1;
						months ++;
						if(months == 13){
							months = 1;
							years++;
						}
					}					
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
			uint8_t rand_int = 0;
			
			while(order_entry_mode_flag == 1){
				rand_int++;
				if(((PIND & 0x08) == 0x08) && (rand_int_loaded == 0)){
					rand_int_locked = rand_int;
					rand_int = 0;
					rand_int_loaded = 1;
				}
			}


			if(order_entry_mode_flag == 0){
				sleep_mode();
			}


		
		
	
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