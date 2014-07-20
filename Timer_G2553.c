
#include <msp430.h>
#include <intrinsics.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;                   // Stop watchdog timer
  BCSCTL2 |= DIVS_3;                          // divider for SMCLK
      
  if (CALBC1_1MHZ==0xFF)                     // If calibration constants erased
  {                                                                                     
    while(1);                               // do not load, trap CPU!!  
  }
  DCOCTL = 0;          
  // Select lowest DCOx and MODx settings
  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
  DCOCTL = CALDCO_1MHZ;
  FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
  
  P2OUT &= ~BIT1;
  P2OUT &= ~BIT2;
  P3OUT &= ~BIT2;
  //P1REN = BIT1;
  P2DIR |= BIT1+BIT2;
  P3DIR |= BIT2;
 
  CCTL0 = CCIE;                               // CCR0 interrupt enabled
  CCR0 = 0x7FEC;
  TACTL = TASSEL_1 + MC_1;                    // ACLK, upmode
  
  P2IE |= BIT5;                             // P1.7 interrupt enabled
  P2IES |= BIT5;                            // P1.7 Hi/lo edge
  P2IFG &= ~BIT5;                           // P1.7 IFG cleared
  _BIS_SR(LPM3_bits + GIE);                   // Enter LPM0 w/ interrupt

  
}


// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
  P2OUT |= BIT1; 
  TACTL = MC_0;
  P2IFG &= ~BIT5;                           // P1.7 IFG cleared
  TA0R = 0x0000;
  CCTL0 = CCIE;                               // CCR0 interrupt enabled
  CCR0 = 0x8000;
  TACTL = ID_3+TASSEL_1 + MC_1;                    // ACLK, upmode
  
 }





// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
  P2OUT &= ~BIT1;                            // Toggle P1.0
  CCR0 = 0x8000;
  TACTL = MC_0;

}

