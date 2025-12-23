/**
* Name:    Samuel Siakpebru
* Date:    07/05/2025
* Youtube:  https://youtube.com/shorts/gSGAht8rtUo?feature=share




*/
#include "intrinsics.h"
#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#define BUTTON_PIN       BIT3  
#define MAX_POSITION     6       
#define CENTER_POSITION  3     
#define BUTTON_P1IES     P1IES
#define BUTTON_P1IFG     P1IFG
#define BUTTON_P1IE      P1IE

typedef enum {IDLE, CYCLE, WIN, LOSE} state_t;

volatile bool globalButtonFlag = false;

void init();
void delay_s(uint16_t seconds);
void delay_ms(uint16_t ms);
static inline void setLED(int position);
bool buttonPressed();

state_t runIdleState(uint16_t* input, uint16_t* seconds);
state_t runCycleState(uint16_t* input, uint16_t* seconds, uint16_t* direction);
state_t runWinState(uint16_t* input, uint16_t* seconds);
state_t runLoseState(uint16_t* input, uint16_t* seconds);

int main(void) {
    volatile uint16_t input = 0;
    volatile uint16_t seconds = 0;
    volatile uint16_t direction = 1;
    state_t currentState = IDLE;
    state_t nextState = currentState;

    init();

    while (1) {
        switch (currentState) {
            case IDLE:
                nextState = runIdleState(&input, &seconds);
                break;
            case CYCLE:
                nextState = runCycleState(&input, &seconds, &direction);
                break;
            case WIN:
                nextState = runWinState(&input, &seconds);
                break;
            case LOSE:
                nextState = runLoseState(&input, &seconds);
                break;
            default:
                nextState = IDLE;
                break;
        }

        currentState = nextState;
        delay_ms(150);
        seconds++;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port1(void){
    globalButtonFlag = true;
   P1IFG &= ~BUTTON_PIN;
}
state_t runIdleState(uint16_t* input, uint16_t* seconds) {
    state_t nextState = IDLE;

    setLED(-1); // turn all LEDs off

    if (buttonPressed()) {
        *input = 0;
        *seconds = 0;
        nextState = CYCLE;
    }
    return nextState;
}

state_t runCycleState(uint16_t* input, uint16_t* seconds, uint16_t* direction) {
    state_t nextState = CYCLE;
    setLED(*input);

    if (buttonPressed()) {
        *seconds = 0;
        nextState = (*input == CENTER_POSITION) ? WIN : LOSE;
    } else {
        if (*direction) {
            (*input)++;
            if (*input >= MAX_POSITION  ) {
                *input = MAX_POSITION;
                *direction = 0; // switch to backward
            }
        } else {
            if (*input > 0) {
                (*input)--;
            }
            if (*input == 0) {
                *direction = 1; // switch to forward
            }
        }
    }

    return nextState;
}

state_t runWinState(uint16_t* input, uint16_t* seconds) {
    state_t nextState = WIN;

    P1OUT |= BIT6;

    int i;
    for (i = 0; i < 3; i++) {
        P2OUT |= (BIT3 | BIT4 | BIT5);
        P2OUT |= (BIT2 | BIT1 | BIT0);
        delay_ms(200);

        P2OUT &= ~(BIT3 | BIT4 | BIT5);
        P2OUT &= ~(BIT2 | BIT1 | BIT0);
        delay_ms(200);
    }

    *input = 0;
    *seconds = 0;
    nextState = IDLE;
    globalButtonFlag = false;
    return nextState;
}

state_t runLoseState(uint16_t* input, uint16_t* seconds) {
    state_t nextState = LOSE;

    delay_s(1); 

    *input = 0;
    *seconds = 0;
    nextState = IDLE;
    globalButtonFlag = false;
    return nextState;
}

void init() {
    WDTCTL = 0x5A80; // stop watchdog timer

    P1DIR |= BIT6;
    P1OUT &= ~BIT6;

    P2DIR |= (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);
    P2OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);

    P1DIR &= ~BUTTON_PIN;  // Set P1.3 as input
    P1REN |= BUTTON_PIN;   // Enable pull-up/down resistor
    P1OUT |= BUTTON_PIN;   // Use pull-up resistor (active-low logic)

    P1DIR &= ~BUTTON_PIN;
    BUTTON_P1IES |= BUTTON_PIN;
    BUTTON_P1IFG &= ~BUTTON_PIN;
    BUTTON_P1IE |= BUTTON_PIN;

    __enable_interrupt();


}

static inline void setLED(int position) {
    P1OUT &= ~BIT6;
    P2OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);

    switch (position) {
        case 0: P2OUT |= BIT3; break;
        case 1: P2OUT |= BIT4; break;
        case 2: P2OUT |= BIT5; break;
        case 3: P1OUT |= BIT6; break;
        case 4: P2OUT |= BIT2; break;
        case 5: P2OUT |= BIT1; break;
        case 6: P2OUT |= BIT0; break;
    }
}

void delay_s(uint16_t seconds) {
    while (seconds--) {
        __delay_cycles(1000000);
    }
}

void delay_ms(uint16_t ms) {
    while (ms--) {
        __delay_cycles(1000);
    }
}

static inline bool buttonPressed() {
    if (globalButtonFlag) {
        globalButtonFlag = false; 
        return true;
    }
    return false;
}
