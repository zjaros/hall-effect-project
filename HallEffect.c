               /* Hall Effect Odometer and Speedometer  */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "pinDefines.h"
#include "USART.h"

#define CIRCUMFERENCE  	88
#define INCHES_PER_FOOT	12
#define FEET_PER_MILE		5280
#define EIGHTH			660

#define INCHES_PER_MILE 		63360.0
#define MILLIS_PER_HOUR		3600000.0


//Flag for ISR
volatile uint8_t flag; //determines when to calculate


//Odometer Variables//
uint8_t inches;
uint16_t feet;
uint16_t miles;
char buffer[5]; //used for UART
uint16_t lFeet;	//used for lights


//Speedometer Variables//
volitile uint32_t millis;
float mph;



ISR(INT0_vect){ 
	//Magnet is sensed//
	if(bit_is_clear(BUTTON_PIN, BUTTON)){
		//Set the millis to the current time//
		millis = (TCNT1 >> 4);
		
		flag = 1;
	}
}

void calculate(void){
	//Converting inches to feet, then feet to miles//
	incrementFoot();
	incrementMile();

	//Display traveled distance to LEDs//
	lightUp();

	//Print data to UART//
	printDistance();
	printSpeed();

	//Reset the clock//
	TCNT1 = 0;
}

void incrementFoot(void){
	//Loop until all possible feet are removed
	while(inches >= INCHES_PER_FOOT){
		feet++;	
		lFeet++;	

		//Decrement inches
		inches = (inches - INCHES_PER_FOOT); 
	}
}


void incrementMile(void){
	if(feet >= FEET_PER_MILE){
		miles++;
		
		//Keep left-over feet
		feet = (feet % FEET_PER_MILE);
	} 
}


//Lights up 8 LEDs according to distance travelled
void lightUp(void){
	if(lFeet == 0){
		PORTB = 0b00000000;	//initially at 'off' state
	}
	else if(lFeet > 0 && lFeet <= EIGHTH){
		PORTB = 0b10000000;	//between 0 and first 1/8th	
	}
	else if(lFeet > EIGHTH && lFeet <= (EIGHTH * 2)){
		PORTB = 0b11000000;	//first 1/8th and second 1/8th
	}
	else if(lFeet > (EIGHTH * 2) && lFeet <= (EIGHTH * 3)){
		PORTB = 0b11100000;	//etc...
	}
	else if(lFeet > (EIGHTH * 3) && lFeet <= (EIGHTH * 4)){
		PORTB = 0b11110000;
	}
	else if(lFeet > (EIGHTH * 4) && lFeet <= (EIGHTH * 5)){
		PORTB = 0b11111000;
	}
	else if(lFeet > (EIGHTH * 5) && lFeet <= (EIGHTH * 6)){
		PORTB = 0b11111100;
	}
	else if(lFeet > (EIGHTH * 6) && lFeet <= (EIGHTH * 7)){
		PORTB = 0b01111110;
	}
	else if(lFeet > (EIGHTH * 7) && lFeet <= (EIGHTH * 8)){
		PORTB = 0b11111111;	//mile almost complete
	}
	else{
		//mile completed

		PORTB = 0x00; //Set back to 'off' state
		
		lFeet = 0; //Reset counter
	}
}


//Prints the distance//
void printDistance(void){
	itoa(feet,buffer,10); //Load number of feet in buffer
	
	//Lots of pretty printing (costly)
	printString("\r\nDistance Traveled: \r\n");
	printString("	Inches: ");
	printByte(inches);
	printString("\r\n	Feet: ");
	printString(buffer);
	printString("\r\n	Miles: ");
	printByte(miles);
	printString("\r\n"); 
}


//Converts the speed and prints it as MPH//
void printSpeed(void){
	
	//Convert the data to MPH
	convert(millis);

	//Print it	
	printString("\r\nCurrent Speed: ");
	printByte(mph);
	printString("  mph \r\n");
}



//incehs per millis  -->  miles per hour//
void convert(uint32_t time){
	//Convert to float
	float t = time;

	//Get the miles per hour from inches per millisecond	
	mph = (CIRCUMFERENCE/t)*(MILLIS_PER_HOUR / INCHES_PER_MILE);
}


void initInterrupt0(void){
	EIMSK |= (1 << INT0);		//enable INT0
	EICRA |= (1 << ISC00);	//Trigger when button changes
}


//From 'reactionTimer.c'
static inline void initTimer1(void) {
  //Prescale of 64//
  TCCR1B |= (1 << CS11) | (1 << CS10);
}


int main(void) { 
    
  flag = 0;
  millis = 0;
  
  BUTTON_PORT |= (1 << BUTTON);		//pullup  
  
  initInterrupt0();
  initTimer1();
  initUSART();
  
  sei();		//Set global interrupt enable bit

  DDRB |= 0xff;	//LED Output set

  printString("ODOMETER - - - -\r\n");

  // ------ Event loop ------ //
  while (1) {
	
	
  	if(flag){
		//Increment the distance
		inches = (inches + CIRCUMFERENCE);
		
		//Make Calculations
		calculate();

		//Reset Flag
		flag = 0; //reset flag 
	}
	
	
  }                            /* End event loop */

  return (0);                  /* This line is never reached */
}



