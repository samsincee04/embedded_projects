// STM32F091RC - GPIOC buttons on PC6 & PC8; LEDs on PC4 & PC5
// Build with stm32f0xx.h available (STM32CubeIDE or equivalent toolchain)
/**
 * Youtube link: https://youtu.be/Lcb9Z_0MaTg?si=sfyK244dzoF2S-nW
 */

#include "stm32f0xx.h"
#include <stdint.h>
#include <stdbool.h>

//          GPIO CONFIG

// LED pin (PA5)
#define LED_GPIO_PORT          GPIOA
#define LED_PIN_NUM            5u
#define LED_PIN_MASK           (1u << LED_PIN_NUM)

// Button pin (PC13)
#define BTN_GPIO_PORT          GPIOC
#define BTN_PIN_NUM            13u
#define BTN_PIN_MASK           (1u << BTN_PIN_NUM)

// MODER/AFR bit shifts
#define GPIO_MODER_BITS_PER_PIN   2u
#define GPIO_AFR_BITS_PER_PIN     4u

// MODER mode values
#define GPIO_MODE_INPUT           0x0u
#define GPIO_MODE_OUTPUT          0x1u
#define GPIO_MODE_AF              0x2u
#define GPIO_MODE_ANALOG          0x3u

// PUPDR values
#define GPIO_PULL_NONE            0x0u
#define GPIO_PULL_UP              0x1u
#define GPIO_PULL_DOWN            0x2u

//               EXTI CONFIG

#define BTN_EXTI_LINE             BTN_PIN_NUM
#define BTN_EXTI_MASK             (1u << BTN_EXTI_LINE)

#define EXTI_PORTC_VALUE          0x2u     // Port C value for EXTICR
#define EXTICR_INDEX              (BTN_PIN_NUM / 4u)
#define EXTICR_SHIFT              ((BTN_PIN_NUM % 4u) * 4u)

//       APPLICATION CONSTANTS

#define NUM_STATES                3u

#define SYSTICK_CLK_FREQ_HZ     48000000ul // 48MHz for STM32F091
#define SYSTICK_TICK_MS         1ul        // 1ms tick rate
#define SYSTICK_RELOAD_VALUE    (SYSTICK_CLK_FREQ_HZ / (1000ul / SYSTICK_TICK_MS))

#define BLINK_INTERVAL_MS       500ul      // Toggle every 500ms (1Hz blink rate)
#define DEBOUNCE_DELAY_MS       30ul       // 30ms minimum time between button presses
#define INACTIVITY_TIME_OUT_MS  5000ul


typedef enum { STATE_0, STATE_1, STATE_2} state_t;

static void gpio_init(void);
static void systick_init(void);
static void enter_stop_mode(void);

void EXTI4_15_IRQHandler(void);
void SysTick_Handler(void);

static inline void setLed(bool enable);
static inline void toggleLed(void);

volatile state_t mode = STATE_0;
volatile uint32_t g_tick_count = 0;
volatile uint32_t g_last_press_time = 0;


int main(void){

uint32_t last_toggle_time = 0;
uint32_t last_activity_time = 0;

gpio_init();
systick_init();

while(1) {
	//Enter STOP mode after inactivity
	if((g_tick_count - last_activity_time) > INACTIVITY_TIME_OUT_MS){

		enter_stop_mode();
		systick_init();
		last_activity_time = g_tick_count;
	}
switch(mode){

case STATE_0:
	setLed(false);
	break;

case STATE_1:
	setLed(true);
	break;

case STATE_2:

    if ((g_tick_count - last_toggle_time) >= BLINK_INTERVAL_MS) {
	     toggleLed();
	  last_toggle_time = g_tick_count;
	                }
    break;

default:
	setLed(false);
	break;


}
    }
} /* USE_FULL_ASSERT */

static void gpio_init(void){
	// enable port clocks in RCC_AHBENR for Ports C(button) and A(LED)
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;

	// LED: PA5 Output
	LED_GPIO_PORT->MODER &= ~(0x3u << (LED_PIN_NUM * GPIO_MODER_BITS_PER_PIN));
	LED_GPIO_PORT->MODER |= (GPIO_MODE_OUTPUT << (LED_PIN_NUM * GPIO_MODER_BITS_PER_PIN));

	    // Configure PC13 as input and enable pull-up
	BTN_GPIO_PORT -> MODER &= ~(0x3u << (BTN_PIN_NUM *  GPIO_MODER_BITS_PER_PIN));// PC13 as input
	BTN_GPIO_PORT -> PUPDR &= ~(0x3u << (BTN_PIN_NUM *  GPIO_MODER_BITS_PER_PIN));
	BTN_GPIO_PORT -> PUPDR |= (GPIO_PULL_UP << (BTN_PIN_NUM *  GPIO_MODER_BITS_PER_PIN));

    //    EXTI Configuration
	RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;// enable SYSCFG clk

    SYSCFG -> EXTICR[EXTICR_INDEX] &= ~(0xFu << EXTICR_SHIFT);
    SYSCFG -> EXTICR[EXTICR_INDEX] |= (EXTI_PORTC_VALUE << EXTICR_SHIFT);


    EXTI -> FTSR |= BTN_EXTI_MASK;// falling edge is active
    EXTI -> RTSR &= ~BTN_EXTI_MASK;// ensure only falling edge is active
    EXTI -> IMR |= BTN_EXTI_MASK;
    EXTI -> PR |= BTN_EXTI_MASK;

    NVIC_EnableIRQ(EXTI4_15_IRQn);
}

//STOP MODE Entry (Deep Sleep)
static void enter_stop_mode (void){
	SysTick -> CTRL &= ~SysTick_CTRL_ENABLE_Msk; // disable SysTick

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // select STOP mode

	__WFI(); // Enter STOP mode

}


void EXTI4_15_IRQHandler(void) {
        // Check if EXTI line 13 triggered the interrupt
        if (EXTI->PR & BTN_EXTI_MASK) {

        	if ((g_tick_count - g_last_press_time) > DEBOUNCE_DELAY_MS) {
        	            // State Transition: 0 -> 1 -> 2 -> 0
        	            mode = (mode + 1) % NUM_STATES;

        	            // Update the last press time
        	            g_last_press_time = g_tick_count;
        	        }


        	EXTI->PR |= BTN_EXTI_MASK; // Clear interrupt flag

        }

    }



static inline void setLed(bool enable){
	if(enable){
	   LED_GPIO_PORT->BSRR = LED_PIN_MASK; //set PA5(turn LED on)
	}else{
	   LED_GPIO_PORT->BSRR = (LED_PIN_MASK << 16u); // turn LED off
		}
	}

static inline void toggleLed(void){

	    GPIOA->ODR ^= LED_PIN_MASK;
}


static void systick_init(void){
	// Set the reload value for the desired tick rate
	    SysTick->LOAD = SYSTICK_RELOAD_VALUE - 1;

	    // Set the clock source to core clock (AHB)
	    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

	    // Enable SysTick interrupt
	    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

	    // Enable SysTick
	    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

