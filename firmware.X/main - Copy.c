#include <pic12f1571.h>
#include <xc.h>

#pragma config MCLRE = OFF    // MCLR Pin disabled and acting as GPIO (Pin 4)
#pragma config WDTE = OFF     // WDT disabled
#pragma config LVP = OFF      // High voltage programming as MCLR disabled
#pragma config FOSC = INTOSC  // Using Internal Osc (Enables Pin 2 to be used as GPIO)

#define _XTAL_FREQ 32000000  // 32MHz Clock

// Value Display / State / Counting Variables
volatile unsigned char period = 1;
volatile unsigned char newPeriod = 1;
volatile unsigned char count = 0;
volatile unsigned char countUp = 1;
volatile unsigned char number = 1;


void interrupt isr(void) 
{ 
// If TMR0 (Overflow Interrupt Enable) bit and T0IF (TMR0 Overflow Interrupt Flag) bit, interrupt is result of TIMER0
if(T0IE && T0IF)     
  { 
    T0IF = 0;                         // Turn interrupt off
    
    ///////////////////////////////////////////////////////////
    //  Clock divider to give number incrementing frequency  //
    //  Frequency = 244/period Hz                            //
    ///////////////////////////////////////////////////////////
    
    // If the correct period has passed, reset count and increment number
    if (count >= period) 
    {
      count = 0;
      number++;
    }          

    if (!count) period = newPeriod;   // Changing the period only when count equals zero prevents delayed updating when the up button is pressed rapidly 
    
    count++;                          // Increment count every time ISR triggers
  }
}

// Function to set GPIO correctly to light individual LED's on board as they are charlieplexed
void displayLED(unsigned char pos, unsigned char colour, unsigned char enable)
{
  // Takes the position in grid | 1 2 3 | and colour (red 0, green 1), and sets that led.
  //							| 4 5 6 |
  //							| 7 8 9 | 
  // NOTE: Top LED's green, bottom red. 

  const unsigned char ledAnode[9]    = {1,4,2,5,2,5,4,5,4};
  const unsigned char ledCathode[9]  = {0,2,1,2,0,4,1,1,0};
  
  if (enable) 
  {      
    if (colour == 0) // If red
    {
      ANSELA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need analog
      TRISA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need to be tristated
      PORTA = 0 | (1<<ledAnode[pos-1]);                  // All GPIO are low except the one on anode side of diode  	
    }
    else if (colour == 1) // if green
    {
        ANSELA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need analog
        TRISA = 0xFF & ~(1<<ledAnode[pos-1]|1<<ledCathode[pos-1]);  // All GPIO except the two for each diode need to be tristated
        PORTA = 0 | (1<<ledCathode[pos-1]);                       // All GPIO are low except the one on anode side of diode
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
    
  // // Below is used to decrease the time LED's are high to decrease ghosting
  // TRISA = (1<<3)|(1<<4);                        // RA3, RA4 Input, others output
  // PORTA = 0;                                    // All GPIO are low
  // __delay_us(100);                             
  
}

void main() 
{

  // Init registers
  OSCCON = 0b11110000;    // Internal oscillator 32MHz and PLL enabled
  ANSELA = 0;             // All I/O pins are configured as digital
  ADCON0 = 0;             // Disable ADC module
  TRISA = (1<<3)|(1<<4);  // RA3, RA4 Input, others output
  PORTA = 0;              // PORTA init all low
  WPUA = 0;               // Disable pullup resistors

  // Setup TIMER0
  OPTION_REG = 0b10000110;    // Weak pull-ups disabled, Timer0 prescaler 1:128 
  INTCON = 0b10100000;        // Global Interrupt Enabled and TMR0 Overflow Interrupt Enabled 

  unsigned char pos = 5;
  unsigned char colour = 1;
  unsigned char enable = 1;
  
  

  while(1)
  {
      TRISA = 0xFF & ~(1<<2|1<<0);  // All GPIO except the two for each diode need to be tristated
      PORTA = 0 | (1<<0);                       // All GPIO are low except the one on anode side of diode
    
 
    // ///////////////////////////////////////////////////////////
    // //                   Display the number                  //
    // ///////////////////////////////////////////////////////////   
    // // Loop over the LED's and check if they need to be lit          
    // for(signed char i = 7; i >= 0; i--)
    // {
    //     unsigned char bitOn = (unsigned char) ((dispNumber >> i) & 1); // Returns 1 if bit is required to display number
    //     (bitOn) ? displayNumber(i) : displayNumber(10); // Display the bit if set, otherwise display nothing (10 is all off)         
    // }

    // (singleStep) ? displayNumber(8) : displayNumber(9); // If single step, turn SS led on (8), otherwise turn clock driven led on (9)
  }
}