/**
* Name:    Samuel Siakpebru
* Date:    07/05/2025
* Assignment: Lab 4
* Youtube:  https://youtube.com/shorts/JZd8EGpqajo?feature=share
**/

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define BUTTON_PIN BIT3
#define ECHO_PIN BIT5
#define BUZZER_PIN BIT6
#define TRIGGER_PIN BIT4

typedef enum { IDLE, ARM, ALARM } state_t;

volatile bool globalButtonFlag = false;
bool buzzerInitialized = false;

state_t runIdleState(float* ref);
state_t runArmState(float* current, float* tolerance, float* ref);
state_t runAlarmState(float* ref);

void init();
void initPWM_Buzzer();
void stopPWM_Buzzer();
float getDistance();
bool buttonPressed();
void delay_ms(unsigned int ms);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    float current = 0.0f;
    float ref = 0.0f;
    float tolerance = 0.05f;

    state_t currentState = IDLE;

    init();

    while (1)
    {
        switch (currentState)
        {
        case IDLE:
            currentState = runIdleState(&ref);
            break;
        case ARM:
            currentState = runArmState(&current, &tolerance, &ref);
            break;
        case ALARM:
            currentState = runAlarmState(&ref);
            break;
        default:
            currentState = IDLE;
        }
    }
}

state_t runIdleState(float* ref)
{
    stopPWM_Buzzer();
    buzzerInitialized = false;
    *ref = 0.0f;

    if (buttonPressed())
        return ARM;

    return IDLE;
}

state_t runArmState(float* current, float* tolerance, float* ref)
{
    if (buttonPressed())
        return IDLE;

    if (*ref == 0.0f)
    {
        *ref = getDistance();
        delay_ms(200); // Let sensor settle
    }

    *current = getDistance();

    if (*current > 0 && fabsf(*current - *ref) > *tolerance)
        return ALARM;

    delay_ms(200);
    return ARM;
}

state_t runAlarmState(float* ref)
{
    if (!buzzerInitialized)
    {
        initPWM_Buzzer();
        buzzerInitialized = true;
    }

    if (buttonPressed())
    {
        stopPWM_Buzzer();
        buzzerInitialized = false;
        *ref = 0.0f;
        return IDLE;
    }

    return ALARM;
}

void init()
{
    P1DIR &= ~BUTTON_PIN;
    P1REN |= BUTTON_PIN;
    P1OUT |= BUTTON_PIN;

    P1IES |= BUTTON_PIN;     
    P1IFG &= ~BUTTON_PIN;    
    P1IE  |= BUTTON_PIN;     

    
    P1DIR |= BUZZER_PIN;
    P1OUT &= ~BUZZER_PIN;

    __enable_interrupt();   
}

void initPWM_Buzzer()
{
    P1SEL |= BUZZER_PIN;             // TA0.1 on Buzzer Pin

    CCR0 = 500 - 1;
    CCR1 = 100;
    CCTL1 = OUTMOD_7;          // Reset/set

    TACTL = TASSEL_2 | MC_1 | ID_0;  // SMCLK, up mode
}

void stopPWM_Buzzer()
{
    TACTL = MC_0;
    CCTL1 = 0;
    P1SEL &= ~BUZZER_PIN;
    P1OUT &= ~BUZZER_PIN;
}

float getDistance()
{
    P1DIR |= TRIGGER_PIN;
    P1OUT |= TRIGGER_PIN;
    __delay_cycles(10);
    P1OUT &= ~TRIGGER_PIN;

    P1DIR &= ~ECHO_PIN;  

    while ((P1IN & ECHO_PIN) == 0); // Wait for high

    TA1R = 0;
    TA1CTL = TASSEL_2 | MC_2;

    while ((P1IN & ECHO_PIN)); // Wait for low

    unsigned int counts = TA1R;
    TA1CTL = MC_0;

    float time_sec = counts * 1.0e-6;
    float distance_m = (time_sec * 343.0) / 2.0;

    return distance_m;
}

bool buttonPressed()
{
    if (globalButtonFlag) {
        globalButtonFlag = false; 
        return true;
    }
    return false;
}

#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
    if (P1IFG & BUTTON_PIN)
    {
        globalButtonFlag = true;
        P1IFG &= ~BUTTON_PIN; 
    }
}

void delay_ms(unsigned int ms)
{
    while (ms--)
        __delay_cycles(1000);
}
