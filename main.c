#include "msp430f5438a.h"
#include <stdint.h>

// Defines from firmware
#define LIGHTDIR P4DIR
#define LIGHTOUT P4OUT
#define LIGHTSEL P4SEL
#define LIGHT    BIT2
#define LIGHTCONTROL TB0CCTL2
#define LIGHTLEVEL TB0CCR2

#define BUTTON_1 1 << 0
#define BUTTON_2 1 << 1
#define BUTTON_3 1 << 2
#define BUTTON_4 1 << 6

uint8_t l_state = 0;
uint8_t b_mask = (BUTTON_1|BUTTON_2|BUTTON_3|BUTTON_4);

// Adapted from firmware
void l_init() {
  LIGHTDIR |= LIGHT;
  LIGHTSEL |= LIGHT;
  TB0CTL |= TBSSEL_1 + MC_1;
  TB0CCR0 = 32; // control PWM freq = 32768/16 = 2048hz
  LIGHTLEVEL = 16;
  LIGHTCONTROL = OUTMOD_0;
}

// Adapted from firmware
void button_init()
{
  // INPUT
  P2DIR &= ~(b_mask);
  P2SEL &= ~(b_mask);

  // ENABLE INT
  P2IES |= b_mask;

  // pullup in dev board
  P2REN |= b_mask;
  P2OUT |= b_mask;
  P2IFG &= ~(b_mask);

  P2IE |= b_mask;
}

void ser_init() {
  // Serial I/O pins init from firmware
  P1DIR |= BIT1;                            // Set P1.1 to output direction
  P1IES |= BIT2;                            // P1.2 Hi/lo edge
  P1DIR &= ~BIT2;                            // Set P1.2 to input
  P1SEL &= ~BIT2;                            // Select Port1
  P1IFG &= ~BIT2;                           // P1.2 IFG cleared
  P1IE |=  BIT2;                            // P1.2 interrupt enabled
}

// Set up interrupts, then put background to sleep,
// and let everything be interrupt driven
void main(void)
{
  __disable_interrupt();
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  SFRIFG1 &= ~WDTIFG;
  SFRIE1 |= WDTIE;

  ser_init();
  button_init();
  l_init();
  __enable_interrupt();

  // Flash backlight to signal boot
  LIGHTCONTROL = OUTMOD_7;
  __delay_cycles(1000000);
  LIGHTCONTROL = OUTMOD_0;
  _BIS_SR(LPM4_bits + GIE); // Enter LPM4 w/interrupt
}

// Port 1 interrupt service routine
// Mirror incoming data from P1.1 to P1.2
// Works up to 19200 bps or so with 1MHz clock
// Backlight is visible up to about 1200 bps
// Remember that DTR will reset the watch
__attribute__((interrupt(PORT1_VECTOR))) void Port_1(void)
{
  P1IFG &= ~BIT2; // P1.2 IFG cleared
  if(P1IN & BIT2) {
    P1OUT |= BIT1; // Set P1.1
    P1IES |= BIT2; // P1.2 Hi/lo edge
    LIGHTCONTROL = OUTMOD_0; // Turn off backlight
  } else {
    P1OUT &= ~BIT1; // Clear P1.1
    P1IES &= ~BIT2; // P1.2 Hi/lo edge
    LIGHTCONTROL = OUTMOD_7; // Turn on backlight
  }
  l_state = 0;
}


// Port 2 interrupt service routine
// Invert backlight when pressing any of the four buttons
__attribute__((interrupt(PORT2_VECTOR))) void Port_2(void)
{
  P2IFG &= ~(b_mask); // Lazily clear all button interrupts
  l_state = 1 - l_state;
  if(l_state) {
    LIGHTCONTROL = OUTMOD_7;
  } else {
    LIGHTCONTROL = OUTMOD_0;
  }
}
