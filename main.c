/*
 * Seven Segment Display Timer
 *
 * Created: 3/16/2021 6:02:18 PM
 * Author : Christian Drewes
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <time.h>

#define ASEG 0
#define BSEG 1
#define CSEG 2
#define DSEG 3
#define ESEG 4
#define FSEG 5
#define GSEG 6
#define NUM0 0
#define NUM1 1
#define NUM2 2
#define NUM3 3
#define STARTBUTTON 5 //PORTD
#define MINUTEBUTTON 4 //PORTB
#define SECONDBUTTON 7 //PORTB
#define BUZZER 0 //PORTC

#define DEBOUNCE_TIME 25 //Button debounce
#define NUMBER_DELAY 6 //Time delay to make illusion of all numbers on screen
#define LOOP_NUMBER_DELAY 150 //Animation time delay
#define LOCK_INPUT_TIME 200

/*Functions to light each digit*/
void zero();
void one();
void two();
void three();
void four();
void five();
void six();
void seven();
void eight();
void nine();

unsigned char second_button_state();
unsigned char minute_button_state();
unsigned char start_button_state();

/*custom array with each number function*/
typedef void (*f)();
f ones[] = {&zero, &one, &two, &three, &four, &five, &six, &seven, &eight, &nine};
f sec_tens[] = {&zero, &one, &two, &three, &four, &five};
f mins[] = {&zero, &one, &two, &three, &four, &five, &six, &seven, &eight, &nine};
f mins_tens[] = {&zero, &one, &two, &three, &four, &five, &six, &seven, &eight, &nine};

void init_io()
{
	DDRD = 0xFFu; // ALL PORT D is output
	DDRB = 0xFFu; // ALL PORT B is output
	DDRC = 0xFFu;
	
	DDRD &= ~(1<<SECONDBUTTON); // SECONDBUTTON is input
	DDRB &= ~(1<<MINUTEBUTTON); // MINUTEBUTTON is input
	DDRB &= ~(1<<STARTBUTTON); // STARTBUTTON is input
	
	PORTD = 0x00u; // Set low
	PORTB = 0x00u; // Set low
	PORTC = 0x00u; // Set low
	
	PORTD &= ~(1<<STARTBUTTON); // TURN OFF STARTBUTTON
	PORTB &= ~(1<<NUM0); // TURN ON FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
}

int main(void)
{
	// initialize the ports and direction ports
	init_io();
	unsigned char sec = 0;
	unsigned char left_sec = 1;
	unsigned char min = 0;
	unsigned char left_min = 0;
	// wait for user to press start button
	while(!start_button_state()){
		loopanimation();
	}
	while(1){
		delay(&sec, &left_sec, &min, &left_min); // Check for button press and show digits
		
		if(sec == 0 && left_sec > 0){ //ex, xx:20 -> xx:19
			--left_sec;
			sec = 9;
		}
		else if(sec == 0 && left_sec == 0 && min > 0){ //ex, x2:00 -> x1:59
			--min;
			sec = 9;
			left_sec = 5;
		}
		else if(sec == 0 && left_sec == 0 && min == 0 && left_min > 0){ //ex, 20:00 -> 19:59
			--left_min;
			sec = 9;
			left_sec = 5;
			min = 9;
		}
		else if(sec == 0 && left_sec == 0 && min == 0 && left_min == 0){
			/*buzzer*/
			while(!start_button_state()){
				/* Flash screen and beep buzzer */
				PORTC |= (1<<BUZZER);
				number_state(&sec, &left_sec, &min, &left_min);
				_delay_ms(1000); 
				PORTC &= ~(1<<BUZZER);
				_delay_ms(500);
			}
			/*Return to start state*/
			while(!start_button_state()){
				loopanimation();
			}
		}
		else --sec; //ex, xx:x9 -> xx:x8
	}
}

/*Shows each number in its current state*/
void number_state(unsigned char* sec, unsigned char* left_sec, unsigned char* min, unsigned char* left_min){
	_delay_ms(NUMBER_DELAY);
	PORTB &= ~(1<<NUM0); // TURN ON FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	ones[*sec]();
	_delay_ms(NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB &= ~(1<<NUM1); // TURN ON SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	sec_tens[*left_sec]();
	_delay_ms(NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB &= ~(1<<NUM2); // TURN ON THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	mins[*min]();
	_delay_ms(NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB &= ~(1<<NUM3); // TURN ON FOURTH NUMBER
	mins_tens[*left_min]();
}

/*loop for the second delay*/
void delay(unsigned char* sec, unsigned char* left_sec, unsigned char* min, unsigned char* left_min){
	unsigned int delay = 0;
	do{
		number_state(&(*sec), &(*left_sec), &(*min), &(*left_min)); //Show digits
		delay += 4 * NUMBER_DELAY; //Calculate current time
		check_buttons(&(*sec), &(*left_sec), &(*min), &(*left_min));
	}while (delay < 1000); //If time is greater than 1s exit
}

/*Checks to see if each button has been pressed*/
void check_buttons(unsigned char* sec, unsigned char* left_sec, unsigned char* min, unsigned char* left_min){
	if(second_button_state()){
		if((int)*sec == 9){
			*sec = 0;
			*left_sec+=1;
			
			if((int)*left_sec == 6 && (int)*min != 5){
				*left_sec = 0;
				*min+=1;
			}
			else if((int)*left_sec == 6 && (int)*min == 5){
				*left_sec = 0;
				*min = 0;
				*left_min+=1;
			}
		}
		else *sec+=1;
		_delay_ms(LOCK_INPUT_TIME);
	}
	else if(minute_button_state()){
		if((int)*min == 9){
			*min = 0;
			if((int)*left_min != 9) *left_min+=1;
		}
		else *min+=1;
		_delay_ms(LOCK_INPUT_TIME);
	}
	else if(start_button_state()){
		while(!start_button_state()){
			number_state(&(*sec), &(*left_sec), &(*min), &(*left_min));
		}
	}
}

unsigned char second_button_state()
{
	if ((PIND & (1<<SECONDBUTTON))){
		_delay_ms(DEBOUNCE_TIME);
		if ((PIND & (1<<SECONDBUTTON))) return 1;
	}
	return 0;
}

unsigned char minute_button_state()
{
	if ((PINB & (1<<MINUTEBUTTON))){
		_delay_ms(DEBOUNCE_TIME);
		if ((PINB & (1<<MINUTEBUTTON))) return 1;
	}
	return 0;
}

unsigned char start_button_state()
{
	if ((PINB & (1<<STARTBUTTON))){
		_delay_ms(DEBOUNCE_TIME);
		if ((PINB & (1<<STARTBUTTON))) return 1;
	}
	return 0;
}

void zero(){
 	PORTD = 0x00u;
 	PORTD |= (1<<ASEG);
 	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
 	PORTD |= (1<<DSEG);
 	PORTD |= (1<<ESEG);
 	PORTD |= (1<<FSEG);
}
void one(){
	PORTD = 0x00u;
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
}
void two(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<BSEG);
	PORTD |= (1<<DSEG);
	PORTD |= (1<<ESEG);
	PORTD |= (1<<GSEG);	
}
void three(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<DSEG);
	PORTD |= (1<<GSEG);
}
void four(){
	PORTD = 0x00u;
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<FSEG);
	PORTD |= (1<<GSEG);
}
void five(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<DSEG);
	PORTD |= (1<<FSEG);
	PORTD |= (1<<GSEG);
}
void six(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<DSEG);
	PORTD |= (1<<ESEG);
	PORTD |= (1<<FSEG);
	PORTD |= (1<<GSEG);
}
void seven(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
}
void eight(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<DSEG);
	PORTD |= (1<<ESEG);
	PORTD |= (1<<FSEG);
	PORTD |= (1<<GSEG);
}
void nine(){
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	PORTD |= (1<<BSEG);
	PORTD |= (1<<CSEG);
	PORTD |= (1<<FSEG);
	PORTD |= (1<<GSEG);
}

/*simple animation to play while is start state*/
void loopanimation(){
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB &= ~(1<<NUM3); // TURN ON FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB &= ~(1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN ON FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB &= ~(1<<NUM1); // TURN ON SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB &= ~(1<<NUM0); // TURN ON FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<ASEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTD = 0x00u;
	PORTD |= (1<<BSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTD = 0x00u;
	PORTD |= (1<<CSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB &= ~(1<<NUM0); // TURN ON FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<DSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB &= ~(1<<NUM1); // TURN ON SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN OFF FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<DSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB &= ~(1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB |= (1<<NUM3); // TURN ON FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<DSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTB |= (1<<NUM0); // TURN OFF FIRST NUMBER
	PORTB |= (1<<NUM1); // TURN OFF SECOND NUMBER
	PORTB |= (1<<NUM2); // TURN OFF THIRD NUMBER
	PORTB &= ~(1<<NUM3); // TURN ON FOURTH NUMBER
	PORTD = 0x00u;
	PORTD |= (1<<DSEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTD = 0x00u;
	PORTD |= (1<<ESEG);
	
	_delay_ms(LOOP_NUMBER_DELAY);
	PORTD = 0x00u;
	PORTD |= (1<<FSEG);
}



