
#include <msp430.h>
#include <intrinsics.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
const unsigned char TimeOFF1=3, TimeOFF2=10, Bounce_delay=0x080;
const unsigned char Port_Lamp1=0x20, Port_Lamp2=0x10, Port_Fan=0x08;
const unsigned int Minute=0x7800, TimeAllOFF=0x600; //1sec = 0x200
unsigned char Lamp1 =0, TimeOutA0=1, TimeOutA1=1;
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

  P2OUT |= BIT1;         // Lamp1 ON
  P2OUT &= ~BIT2;        // Lamp2 OFF
  P3OUT &= ~BIT2;        // Fan OFF
  P2DIR |= BIT1+BIT2;
  P3DIR |= BIT2;

  
  CCTL0 = CCIE;                             // CCR0 interrupt enabled
  CCR0 = Minute;                            // 1min = 0x7800 1sec = 0x200
  TACTL = ID_3+TASSEL_1 + MC_1;             // ACLK, upmode

  
  P2IE |= BIT5+BIT4+BIT3;                   // interrupt enabled
  P2IES = 0x020;                            // Rising edge for Lamp2 & Fan (Falling for Fan)
  P2IFG &= ~BIT5;                           // PIFG cleared
  P2IFG &= ~BIT4; 
  P2IFG &= ~BIT3;
  
  _BIS_SR(LPM3_bits + GIE);                 // Enter LPM0 w/ interrupt
}  


// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
  if (Port_Lamp1 & P2IFG) 
    {P2OUT |= BIT1;                            //Lamp1 ON
    Lamp1 =1;
    
    TimeOutA0=TimeOFF1;
    TACTL = MC_0;
    TA0R = 0x0000;
    CCTL0 = CCIE;                             // CCR0 interrupt enabled
    CCR0 = Minute;
    TACTL = ID_3+TASSEL_1 + MC_1;             // ACLK, upmode
    
    P2IFG &= ~BIT5;                           // P2.5 motion sensor
    L2Delay1 = 0;
    }
  if (Port_Lamp2 & P2IFG) 
    {L2Delay2 = TA0R - L2Delay1;              //Contact bounce delay
    if (L2Delay2>Bounce_delay)                // 250 ms
       {if (Lamp1) P2OUT ^= BIT2;             //Toggle Lamp2 OFF
       }
    P2IFG &= ~BIT4;                           // P2.4 SW1 Lamp2
    
    if (TA0R<0x1400)
       {TACTL = MC_0;
       TA0R = 0x0000;
       CCTL0 = CCIE;                          // CCR0 interrupt enabled
       CCR0 = Minute;
       TACTL = ID_3+TASSEL_1 + MC_1;          // ACLK, upmode
       }
    L2Delay1 = TA0R;
    while (Port_Lamp2 & P2IN) 
       {unsigned int delta;
       delta = TA0R - L2Delay1;
       if (delta>TimeAllOFF) 
          {P2OUT &= ~BIT1;                    // Main Lamp1 OFF
           P2OUT &= ~BIT2;                    // Lamp2 OFF
           P3OUT &= ~BIT2;                    // Lamp2 OFF
           Lamp1 =0;
           TACTL = MC_0;
           TA1CTL = MC_0;
           TA0R = 0x0000;
           TA1R = 0x0000;}
       P2IFG = 0;
       if (delta>Minute) break;
       }
      
    }
  if (Port_Fan & P2IFG) 
    {TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
     TA1CCR0 = Bounce_delay;                   // Contact bounce delay
     TA1CTL = ID_3+TASSEL_1 + MC_1;            // ACLK, upmode
     unsigned char compare_delay;
     compare_delay=Bounce_delay-10;
     while (TA1R<compare_delay);
     TA1CTL = MC_0;

     TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
     TA1CCR0 = Minute;                         // 1min = 0x7800 1sec = 0x200
     TA1CTL = ID_3+TASSEL_1 + MC_1;            // ACLK, upmode     
     TimeOutA1=TimeOFF2;
     P3OUT ^= BIT2;
     P2IFG &= ~BIT3;
     }
 }


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{ TimeOutA0 = TimeOutA0-1;
  if (TimeOutA0<1)
  {P2OUT &= ~BIT1;                            // Main Lamp1 OFF
  P2OUT &= ~BIT2;                             // Lamp2 OFF
  Lamp1 =0;
  TACTL = MC_0;
  }
}

// TA0_A1 Interrupt vector
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR (void)
{ TimeOutA1 = TimeOutA1-1;
  if (TimeOutA1<1)
  {P3OUT &= ~BIT2;                           // Fan OFF
  TA1CTL = MC_0;}
}
