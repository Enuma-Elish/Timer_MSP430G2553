
#include <msp430.h>
#include <intrinsics.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
const unsigned char TimeOFF1=3, TimeOFF2=10;
unsigned char Lamp1 =0, TimeOutA0=1;
unsigned int  L2Delay1=0, L2Delay2;

int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;                   // Stop watchdog timer
  
  BCSCTL2 = SELM_0 + DIVM_0 + DIVS_0;
    if (CALBC1_8MHZ != 0xFF) {
        /* Adjust this accordingly to your VCC rise time */
        __delay_cycles(100000);
        // Follow recommended flow. First, clear all DCOx and MODx bits. Then
        // apply new RSELx values. Finally, apply new DCOx and MODx bit values.
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_8MHZ;      /* Set DCO to 8MHz */
        DCOCTL = CALDCO_8MHZ;
    }
    /*  ACLK = 32768/8 = 4096 Hz
     * Basic Clock System Control 1
     * XT2OFF -- Disable XT2CLK
     * ~XTS -- Low Frequency
     * DIVA_3 -- Divide by 8
     * Note: ~XTS indicates that XTS has value zero
     */
    BCSCTL1 |= XT2OFF + DIVA_3;

    /* 
     * Basic Clock System Control 3
     * XT2S_0 -- 0.4 - 1 MHz
     * LFXT1S_0 -- If XTS = 0, XT1 = 32768kHz Crystal ; If XTS = 1, XT1 = 0.4 - 1-MHz crystal or resonator
     * XCAP_1 -- ~6 pF
     */
    BCSCTL3 = XT2S_0 + LFXT1S_0 + XCAP_1;

  P2OUT |= BIT1;
  P2OUT &= ~BIT2;
  P3OUT &= ~BIT2;
  P2REN = BIT4;
  P2DIR |= BIT1+BIT2;
  P3DIR |= BIT2;
 
  CCTL0 = CCIE;                             // CCR0 interrupt enabled
  CCR0 = 0x7800;                            // 1min = 0x7800 1sec = 0x200
  TACTL = ID_3+TASSEL_1 + MC_1;             // ACLK, upmode
  
  P2IE |= BIT5+BIT4+BIT3;                   // interrupt enabled
  P2IES |= BIT5+BIT3;                       // Hi/lo edge
  P2IES &= ~BIT4;                           // Hi/lo edge
  P2IFG &= ~BIT5;                           // PIFG cleared
  P2IFG &= ~BIT4; 
  P2IFG &= ~BIT3;
  
  _BIS_SR(LPM3_bits + GIE);                 // Enter LPM0 w/ interrupt
}  


// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
  if (0x20 & P2IFG) 
    {P2OUT |= BIT1;                            //Main Lamp1 ON
    Lamp1 =1;
    
    TimeOutA0=TimeOFF1;
    TACTL = MC_0;
    TA0R = 0x0000;
    CCTL0 = CCIE;                             // CCR0 interrupt enabled
    CCR0 = 0x7800;
    TACTL = ID_3+TASSEL_1 + MC_1;             // ACLK, upmode
    
    P2IFG &= ~BIT5;                           // P2.5 motion sensor
    L2Delay1 = 0;
    }
  if (0x10 & P2IFG) 
    {L2Delay2 = TA0R - L2Delay1;              //Contact bounce delay
    if (L2Delay2>0x0100)                      //500 ms
       {if (Lamp1) P2OUT ^= BIT2;             //Toggle Lamp2 OFF
       }
    P2IFG &= ~BIT4;                           // P2.4 SW1 Lamp2
    L2Delay1 = TA0R;
    while (0x10 & P2IN) ;
    }
  if (0x08 & P2IFG) P2IFG &= ~BIT3;
 }


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{ TimeOutA0 = TimeOutA0-1;
  if (TimeOutA0<1)
  {P2OUT &= ~BIT1;                            // Main Lamp1 OFF
  P2OUT &= ~BIT2;                            // Lamp2 OFF
  Lamp1 =0;
  TACTL = MC_0;
  }
  CCR0 = 0x7800;
}

