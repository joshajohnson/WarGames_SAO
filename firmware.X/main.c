#include <pic12f1571.h>
#include <xc.h>

#pragma config MCLRE = OFF    // MCLR Pin disabled and acting as GPIO (Pin 4)
#pragma config WDTE = OFF     // WDT disabled
#pragma config LVP = OFF      // High voltage programming as MCLR disabled
#pragma config FOSC = INTOSC  // Using Internal Osc (Enables Pin 2 to be used as GPIO)

#define _XTAL_FREQ 32000000  // 32MHz Clock
#define NUM_STATES 15

// ISR variables for timing
volatile unsigned char oneTwentyHertz = 0;
volatile unsigned char sixtyHertz = 0;
volatile unsigned char accumulator = 0;

// LED States (Global so XC8 stores it in rom not flash)
const unsigned char positions[NUM_STATES][9] = {
                      {5,9,3,7,8,2,6,4,1},
                      {5,9,1,7,8,2,4,6,3},
                      {5,7,1,9,8,2,6,4,3},
                      {5,9,1,7,8,2,6,4,3},
                      {5,3,9,1,2,8,7,4,6},
                      {5,9,1,7,8,2,3,4,6},
                      {5,9,3,7,8,2,1,6,4},
                      {5,3,7,1,2,8,9,6,4},
                      {5,3,9,1,2,8,6,4,7},
                      {5,7,3,9,8,2,6,4,1},
                      {5,7,3,9,8,2,1,6,4},
                      {5,1,7,3,2,8,9,4,6},
                      {5,3,7,1,2,8,9,6,4},
                      {5,9,3,7,8,2,6,4,1},
                      {5,1,9,3,2,8,6,4,7}
                      };

void interrupt isr(void) 
{ 
// If TMR0 (Overflow Interrupt Enable) bit and T0IF (TMR0 Overflow Interrupt Flag) bit, interrupt is result of TIMER0
if(T0IE && T0IF)     
  { 
    T0IF = 0;           // Turn interrupt off
   	oneTwentyHertz = 1;

   	// Clock divider to get 60Hz pulse
   	if (oneTwentyHertz)
  	{
  		oneTwentyHertz = 0;

  		if (accumulator >= 1)
  			{
  				accumulator = 0;
  				sixtyHertz = 1;
  			}
  		else 
  			accumulator++;
  	}	
  }
}


// Function to set GPIO correctly to light individual LED's on board as they are charlieplexed
void displayLED(unsigned char pos, unsigned char colour, unsigned char enable)
{
  // Takes the position in grid | 1 2 3 | and colour (0 or 1) and sets that LED
  //														| 4 5 6 |
  //														| 7 8 9 | 

  const unsigned char ledAnode[10]    = {1,4,2,5,2,5,4,5,4};
  const unsigned char ledCathode[10]  = {0,2,1,2,0,4,1,1,0};
  
  if (enable) 
  {      
    if (colour == 0)
    {
      TRISA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need to be tristated
      PORTA = 0 | (1<<ledAnode[pos-1]);                  					// All GPIO are low except the one on anode side of diode  	
    }
    else if (colour == 1)
    {
        TRISA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need to be tristated
        PORTA = 0 | (1<<ledCathode[pos-1]);                       	// All GPIO are low except the one on cathode side of diode        
    }
    else 
    {
        TRISA = 0;
        PORTA= 0;
    }
  }
  else 
  {
    TRISA = 0;
    PORTA= 0;
  }                     
  
}


void main() 
{

  // Init registers
  OSCCON = 0b11110000;              // Internal oscillator 8MHz and 4x PLL enabled
  ANSELA = 0;                       // All I/O pins are configured as digital
  ADCON0 = 0;                       // Disable ADC module
  TRISA = (1<<3)|(1<<4);            // RA3, RA4 Input, others output
  PORTA = 0;                        // PORTA init all low
  WPUA = 0;                         // Disable pullup resistors

  // Setup TIMER0
  OPTION_REG = 0b10000111;          // Weak pull-ups disabled, Timer0 prescaler 1:256 
  INTCON = 0b10100000;              // Global Interrupt Enabled and TMR0 Overflow Interrupt Enabled 

  unsigned char colour = 0;
  unsigned char enable = 1;
  signed char statesShown = -1;
  unsigned char accumulatorTwo = 0;
  unsigned char updateRate = 60;    // Frequency of position update = 60 / updateRate
  unsigned char gameDisplayed = 0;
  unsigned char countUp = 1;
  unsigned char nextGame = 0;

  
  // Test all of the LEDs are in the correct position
  for (unsigned char col = 0; col <= 1; col ++)
  {
    for (unsigned char j = 9; j > 0; j--)
    {
        displayLED(j,col,1);
        __delay_ms(100);
    }
  displayLED(10,0,1);
  }

  while(1)
  {
    // If all states have been displayed, goto next displayed sequence
    if (nextGame)
    {
      nextGame = 0;
      if (gameDisplayed == 0) countUp = 1;
      if (gameDisplayed == NUM_STATES-1) countUp = 0;

      countUp ? gameDisplayed++ : gameDisplayed--;
    }

  	// Increase number of positions shown on display at speed updateRate
		if (sixtyHertz)
		{
			sixtyHertz = 0;

			if (accumulatorTwo >= (updateRate - (5 * gameDisplayed - 13))) // Magic numbers control ramp speed 
	  			{
	  				accumulatorTwo = 0;

            if (statesShown >= 9) 
              {
                nextGame = 1;
                __delay_ms(300);
              }

	  		    (statesShown >= 9) ? statesShown = 0 : statesShown++;    // Increment num shown between 0 and 9	

	  			}
	  	else accumulatorTwo++;		 
		}


		for (signed char pos = 0; pos <= statesShown; pos++)
		{
			(pos % 2 == 0) ? colour = 0 : colour = 1; 	                    // One colour always goes first so current colour can be determined by modulo 
			(statesShown == 9) ? enable = 0 : enable = 1;						        // Turns enable off if in position 0 (nothing on screen)
      displayLED(positions[gameDisplayed][pos], colour, enable);
      __delay_us(500);
		}

  }
}