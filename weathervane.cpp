/*
 * Weathervane Project
 *
 * February 1-2, 2015
 * (instead of watching the super bowl)
 *
 * Created: 1/30/2015 10:36:08 PM
 *  Author: John
 */ 


#define F_CPU 1000000UL // 1 MHz
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>


#define DP_MASK			0b00001000
#define NUMERAL_0		0b11110110
#define NUMERAL_1		0b01010000
#define NUMERAL_2		0b01100111
#define NUMERAL_3		0b01110011
#define NUMERAL_4		0b11010001
#define NUMERAL_5		0b10110011
#define NUMERAL_6		0b10110111
#define NUMERAL_7		0b01110000
#define NUMERAL_8		0b11110111
#define NUMERAL_9		0b11110011
#define NUMERAL_BLANK	0b00000000
#define NUMERAL_NEG		0b00000001

#define BLANK			10
#define NEG				11

#define DIGIT_1			0b00110100
#define DIGIT_2			0b00011100
#define DIGIT_3			0b00101100
#define DIGIT_4			0b00111000
#define DIGITS_OFF		0b00111100

#define __N				0b11101111
#define __NNE			0b00010000
#define __NE			0b11110111
#define __ENE			0b00001000
#define __E				0b11111011
#define __ESE			0b00000100
#define __SE			0b11111101
#define __SSE			0b00000010
#define __S				0b11111110
#define __SSW			0b00000001
#define __SW			0b01111111
#define __WSW			0b10000000
#define __W				0b10111111
#define __WNW			0b01000000
#define __NW			0b11011111
#define __NNW			0b00100000

#define CARDINAL		0b00000010
#define MINOR			0b00000000

#define DIGIT_MASK		0b00111100
#define COMPASS_MASK	0b00000010

// anemometer facts
#define PI						3.14159
#define CLICKS_PER_REV			3

#define ANEM_RADIUS_INCHES		3
#define ANEM_CIRC_INCHES		18.8496   //(ANEM_RADIUS_INCHES * 2 * PI)
#define ANEM_CIRC_MILES			0.000297  //(ANEM_CIRC_INCHES / (12 * 5280))
#define ANEM_CIRC_MILES_32		623

#define CPU_CLOCK				1000000
#define TIMEBASE				0.000256 // (256 / CPU_CLOCK)
#define TICKS_PER_SECOND		(1 / TIMEBASE) //3906.25  //(1 / TIMEBASE)
#define TICKS_PER_MINUTE		60 * TICKS_PER_SECOND //234375  //60 * TICKS_PER_SECOND

//#define ONE_RPS					1302 //(TICKS_PER_SECOND / CLICKS_PER_REV)
#define ONE_RPM					(TICKS_PER_MINUTE / CLICKS_PER_REV) //78125  //(TICKS_PER_MINUTE / CLICKS_PER_REV)
//#define ONE_RPH					ONE_RPM * 60

uint8_t numbers[12] = { NUMERAL_0, NUMERAL_1, NUMERAL_2, NUMERAL_3, NUMERAL_4, NUMERAL_5, NUMERAL_6, NUMERAL_7, NUMERAL_8, NUMERAL_9, NUMERAL_BLANK, NUMERAL_NEG };
uint8_t digits[4] = { DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4 };
uint8_t compass[16] = { __N, __NNE, __NE, __ENE, __E, __ESE, __SE, __SSE, __S, __SSW, __SW, __WSW, __W, __WNW, __NW, __NNW };
uint8_t compass_ctrl[16] = { CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR, CARDINAL, MINOR };
	
	
// holds the characters to be displayed
uint8_t display[4] = { 0, 0, 0, 0 };
	
uint8_t last_anem_value;
uint8_t wind_direction;

uint8_t anem_timeout = 0;
uint8_t dp_on = 1;


double rpm, rps, mph;

double temp_double;
uint32_t temp_32;

	
void init_ports(void)
{
	DDRB = 0;
	DDRC = 0b00111100;
	DDRD = 0xFF;
	
	//PORTC = 0b00110000;
	//PORTC = 0b00111100;
	//PORTD = 0b11111111;
	
	//PORTC = DIGIT_4;
	//PORTD = NUMERAL_9;
	
	ADMUX =		0b01100000;
	ADCSRA =	0b11100010;
	ADCSRB =	0b00000000;
	
	last_anem_value = PINB;
	last_anem_value &= 0x01;	
}

void init_timers(void)
{
	TCCR1A = 0;
	TCCR1B = 0b00000100;
	TCCR1C = 0;
	
	TCNT1 = 0;
	
}

void init_interrupts(void)
{
	// set for pin-change interrupt on PCINT0 (PB0)
	PCICR = 0b00000001;
	PCIFR = 0b00000001;
	PCMSK0 = 0b00000001;
	
	TIMSK1 = 1;
	
	//TIFR1 = 1;
	
}

ISR(PCINT0_vect)
{
	uint8_t temp;
	uint16_t temp2;
	temp = PINB;
	temp &= 0x01;

	DDRD = 0;
	if ((temp == 0) && (last_anem_value == 1))
	{
		anem_timeout = 0;
		temp2 = TCNT1;
		TCNT1 = 0;
	
		rpm = (ONE_RPM / temp2);
		
		mph = 100 * (rpm * 60) * ANEM_CIRC_MILES;
	
	}
	last_anem_value = temp;
	DDRD = 0xFF;
}

ISR(TIMER1_OVF_vect)
{
	anem_timeout = 1;
}

void int2display(uint16_t val)
{
	if (val >= 9999)
	{
		display[3] = BLANK;
		display[2] = NEG;
		display[1] = NEG;
		display[0] = BLANK;
	}
	else {
		display[3] = (val % 10);
		val /= 10;
		display[2] = (val % 10);
		val /= 10;
		display[1] = (val % 10);
		val /= 10;
		display[0] = (val % 10);
		val /= 10;
	
		if (display[0] == 0) { display[0] = 10; }
		//if (display[1] == 0 && display[0] == 10) { display[1] = 10; }				
	}
}

void render_digits(uint8_t digit, uint8_t dp)
{
	DDRC = DIGIT_MASK;
	PORTD = NUMERAL_BLANK;
	PORTC = digits[digit];
	if ((digit == dp) & dp_on) { PORTD = (numbers[display[digit]] | DP_MASK); }
	else { PORTD = numbers[display[digit]]; }
}

void render_compass(uint8_t needle)
{
	DDRC = COMPASS_MASK;
	
	PORTD = compass[needle];
	PORTC = compass_ctrl[needle];
}

uint8_t get_direction(void)
{
	uint8_t temp;
	
	temp = ADCH ^ 0xFF;
	
	if ((temp < 7) | (temp > 248)) { return 0; }
	else {
		temp = temp + 8;
		temp = temp >> 4;
		return temp;
	}
	
	temp = temp >> 4;
	return temp;
}

int main(void)
{
	uint8_t cp = 0;
	
	init_ports();
	init_timers();
	init_interrupts();
	
	int2display(123);
	
	
	sei();
	
    while(1)
    {

		for (cp = 0; cp < 4; cp++)
		{
			render_digits(cp, 1);
			
			// 240Hz refresh rate -- to allow filming at 24Hz.
			_delay_us(382);
		}
		
		
		
		for (cp = 0; cp < 4; cp++)
		{
			wind_direction = get_direction();
			render_compass(wind_direction);
			
			// 240Hz refresh rate -- to allow filming at 24Hz.
			_delay_us(382);
		}		
		
		if (anem_timeout == 1) {
			dp_on = 0;
			int2display(9999);
			_delay_us(148);
			}
		else {
			dp_on = 1;
			int2display((uint16_t)mph);
			}
		//int2display(anem_timeout);			
    }

	
}