#include "stm32f0xx.h"

static void gpio_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN;
    GPIOA->MODER &= ~(3u << (5*2));
    GPIOA->MODER |=  (1u << (5*2));      // PA5 output
    GPIOC->MODER &= ~(3u << (13*2));
    GPIOC->PUPDR &= ~(3u << (13*2));
    GPIOC->PUPDR |=  (1u << (13*2));     // PC13 pull-up
}

static void exti13_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[3] &= ~(0xFu << 4);
    SYSCFG->EXTICR[3] |=  (0x2u << 4);   // Port C
    EXTI->IMR  |= (1u << 13);
    EXTI->FTSR |= (1u << 13);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void EXTI4_15_IRQHandler(void) {
    if (EXTI->PR & (1u << 13)) {
        EXTI->PR = (1u << 13);
        GPIOA->ODR ^= (1u << 5);          // Toggle LED
    }
}

int main(void) {
    gpio_init();
    exti13_init();
    EXTI->PR = (1u << 13);
    __enable_irq();

    while (1) {
        __NOP();
    }
}
