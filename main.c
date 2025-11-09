/**
* Name:    Samuel Siakpebru
* Date:    08/02/2025
* Youtube: https://youtube.com/shorts/Gly_8OtgdvE?si=lGy6Q11B6QJfSqcb
**/

#include "msp430g2553.h"
#include <msp430.h>
#include <stdbool.h>

void init_UART(void);
void send_byte(unsigned int byte);
bool buttonPressed();
void initGPIO(void);
void init_ADC(void);

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; 
     init_UART(); 
     initGPIO();
     init_ADC();

    while(1)
    {
     unsigned char adc_val = read_ADC();  // Read potentiometer

     if(adc_val != 255){
        send_byte(adc_val);
     }
     
     if(buttonPressed()){
        send_byte(255);
     }
      __delay_cycles(10000);

  }
}

void initGPIO(void){
P1DIR &= ~BIT3;
P1REN |= BIT3;
P1OUT |= BIT3;    
}

void init_UART(void){
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    
    P1SEL = BIT1 | BIT2;  // P1.1 = RXD, P1.2 = TXD
    P1SEL2 = BIT1 | BIT2;

    UCA0CTL1 |= UCSWRST;
    
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    UCA0BR0 = 104; // 1 MHz 9600
    UCA0BR1 = 0; // 1  MHz 9600
    UCA0MCTL = UCBRS0;
    
    UCA0CTL1 &= ~UCSWRST; // initialize UCSI state machine
    


}

void init_ADC(void){
    ADC10CTL1 = INCH_1;
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON;
    ADC10AE0 |= BIT1;  
}

unsigned char read_ADC(void){
   ADC10CTL0 |= ENC + ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);     // Wait for conversion to finish
    unsigned int raw = ADC10MEM;       // Read 10-bit value (0–1023)

    return (unsigned char)(raw >> 2);
}

void send_byte(unsigned int byte){
while(!(IFG2 & UCA0TXIFG));
UCA0TXBUF = byte;
}

bool buttonPressed(){  
    return !(P1IN & BIT3);
}