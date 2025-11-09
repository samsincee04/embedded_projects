// STM32F091RC - GPIOC buttons on PC6 & PC8; LEDs on PC4 & PC5
// Build with stm32f0xx.h available (STM32CubeIDE or equivalent toolchain)
/**
 * Youtube link: https://youtube.com/shorts/VzZSHi-zsfE?si=_T_5k3jEHORVmxJb
 */

#include "stm32f0xx.h"
#include <stdint.h>
#include <stdbool.h>

#define SYSTICK_CLK_FREQ_HZ     48000000ul // 48MHz for STM32F091
#define SYSTICK_TICK_MS         1ul        // 1ms tick rate
#define SYSTICK_RELOAD_VALUE    (SYSTICK_CLK_FREQ_HZ / (1000ul / SYSTICK_TICK_MS))

#define BLINK_INTERVAL_MS       500ul      // Toggle every 500ms (1Hz blink rate)
#define DEBOUNCE_DELAY_MS       30ul       // 30ms minimum time between button presses


typedef enum { STATE_0, STATE_1, STATE_2} state_t;

static void gpio_init(void);
static void systick_init(void);
void EXTI4_15_IRQHandler(void);
void SysTick_Handler(void);
static inline void setLed(bool enable);
static inline void toggleLed(void);

volatile state_t mode = STATE_0;
volatile uint32_t g_tick_count = 0;
volatile uint32_t g_last_press_time = 0;


int main(void){


uint32_t last_toggle_time = 0;

gpio_init();
systick_init();

while(1) {
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

	GPIOA->MODER &= ~(0x3u << (5 * 2));
	GPIOA->MODER |= (0x1u << (5 * 2));

	    // Configure PC13 as input and enable pull-up
	GPIOC -> MODER &= ~(0x3u << (13 * 2));// PC13 as input
    GPIOC -> PUPDR &= ~(0x3u << (13 * 2));// changed before
    GPIOC -> PUPDR |= (0x1u << (13 * 2));

    RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;// enable SYSCFG clk

    SYSCFG -> EXTICR[3] &= ~(0xFu << (1 * 4));
    SYSCFG -> EXTICR[3] |= (0x2u << (1 * 4));


    EXTI -> FTSR |= (0x1u << (1 * 13));// falling edge is active
    EXTI -> RTSR &= ~(0x1u << (1 * 13));// ensure only falling edge is active
    EXTI -> IMR |= (0x1u << (1 * 13));
    EXTI -> PR |= (0x1u << (1 * 13));

    NVIC_EnableIRQ(EXTI4_15_IRQn);
}


void EXTI4_15_IRQHandler(void) {
        // Check if EXTI line 13 triggered the interrupt
        if (EXTI->PR & (0x1ul << 13)) {

        	if ((g_tick_count - g_last_press_time) > DEBOUNCE_DELAY_MS) {
        	            // State Transition: 0 -> 1 -> 2 -> 0
        	            mode = (mode + 1) % 3;

        	            // Update the last press time
        	            g_last_press_time = g_tick_count;
        	        }


        	EXTI->PR |= (0x1ul << 13);

        }

    }



static inline void setLed(bool enable){
	if(enable){
		GPIOA->BSRR = (1u << 5); //set PA5(turn LED on)
	}else{
			GPIOA->BSRR = (1u << (5 + 16)); // turn LED off
		}
	}

static inline void toggleLed(void){

	    GPIOA->ODR ^= (1u << 5);
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


