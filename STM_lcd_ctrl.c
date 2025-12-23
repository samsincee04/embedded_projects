#include "stm32f0xx.h"
#include <stdint.h>
#include <stdbool.h>

#define LCD_WIDTH 240
#define LCD_HEIGHT 320


static void spi1_init(void);
static void lcd_gpio_init(void);
static void lcd_send_command(uint8_t cmd);
static void spi_send8(uint8_t data);
void lcd_init(void);





int main(void) {
    lcd_init();

    // Write red hex code to all pixels
    lcd_send_command(0x2C);
    // TODO: Write CS pin low
	GPIOA->BSRR = (0x1u << (9 + 16 ));
    // TODO: Write DC pin high
	GPIOB->BSRR = (0x1u << 10);
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        spi_send8(0xF8);
        spi_send8(0x00);
    }
    // TODO: Drive CS pin high
    GPIOA->BSRR = (0x1u << 9);

    while (1) {

    }
}


void spi1_init(void) {
    // Enable SPI1 and GPIOA clocks
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;

    // TODO 1: Configure PA5 (SCK) and PA7 (MOSI) as GPIO alternate function
    GPIOA->MODER &= ~(0x3u << (5 * 2));
    GPIOA->MODER |= (0x2u << (5 * 2));// Alternate function mode
    GPIOA->AFR[0] &= ~(0xFu << (5 * 4));

    GPIOA->MODER &= ~(0x3u << (7 * 2));
    GPIOA->MODER |= (0x2u << (7 * 2));
    GPIOA->AFR[0] &= ~(0xFu << (7 * 4));



    // TODO 2: Configure SPI1 control registers
    // Set device as SPI Master (STM32 generates SCK and controls communication)
    SPI1->CR1 |= SPI_CR1_MSTR;// MSTR, Bit 2

    // Mode 3: CPOL=1 (CK=1 when idle), CPHA=1 (data captured on 2nd edge, which is rising edge).
    SPI1->CR1 |= SPI_CR1_CPOL;
    SPI1->CR1 |= SPI_CR1_CPHA;
    // Enable software slave management
    SPI1->CR1 |= SPI_CR1_SSM;
    // Set internal NSS high (required when using software slave management)
    SPI1->CR1 |= SPI_CR1_SSI;
    // Set baud rate prescaler: f_PCLK / 4
    SPI1->CR1 &= ~(0x7u << 3); //BR[2:0] so 3 bits
    SPI1->CR1 |= (0x1u << 3 );
    // Configure data frame size for 8 bits
    SPI1->CR2 &= ~(0xFu << 8);// DS[3:0], and Bits 11:8 so 8
    SPI1->CR2 |= (0x7u << 8);

    // TODO 3: Enable SPI peripheral
    SPI1->CR1 |= SPI_CR1_SPE;
}

void lcd_gpio_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

    // TODO: Initialize the mode of the other control pins
    GPIOA->MODER &= ~(0x3u << (9 * 2));
    GPIOA ->MODER |= (0x1u << (9 * 2));// PA9 as output

    GPIOA->MODER &= ~(0x3u << (1 * 2));
    GPIOA ->MODER |= (0x1u << (1 * 2));

    GPIOB->MODER &= ~(0x3u << (10 * 2));
    GPIOB->MODER |= (0x1u << (10 * 2));

    GPIOA->MODER &= ~(0x3u << (8 * 2));
    GPIOA ->MODER |= (0x1u << (8 * 2));



}

static void lcd_send_command(uint8_t cmd) {
	// TODO: Write DC and CS low
	GPIOB->BSRR = (0x1u << (10 + 16));

	GPIOA->BSRR = (0x1u << (9 + 16 ));


	    spi_send8(cmd);
	    // TODO: Set CS back high
	    GPIOA->BSRR = (0x1u << 9);
	}

	void lcd_send_data(uint8_t data) {
	    // TODO: Write DC high and CS low
		GPIOB->BSRR = (0x1u << 10);

		GPIOA->BSRR = (0x1u << (9 + 16));

	    spi_send8(data);
	    // TODO: Set CS back high
	    GPIOA->BSRR = (0x1u << 9);
	}

static void spi_send8(uint8_t data) {
    // TODO: wait until the tx buffer is empty
	while(!(SPI1->SR & SPI_SR_TXE)) {} // wait until TXE=1 (empty)

	*((volatile uint8_t*)&SPI1->DR) = data;

    // TODO: wait for transmission to complete
	while(SPI1->SR & SPI_SR_BSY) {} // wait until not busy
}

	void lcd_init(void) {
	    lcd_gpio_init();
	    spi1_init();

	    // Hardware reset
	    // TODO: Drive reset line low
	    GPIOA->BSRR = (0x1u << (1 + 16));
	    for (volatile int i=0; i<10000; ++i);  // short delay
	    // TODO: Drive reset line high
	    GPIOA->BSRR = (0x1u << 1);

	    for (volatile int i=0; i<10000; ++i); //short delay

	    // LCD Initialization Sequence
	    // TODO: Send sleep out command
	    lcd_send_command(0x11);

	    for (volatile int i=0; i<10000; ++i); // short delay
	    lcd_send_command(0x3A);

	    // TODO: Configure 16-bits per pixel
	    lcd_send_data(0x55);


	    // TODO: Send MADCTL command

	    lcd_send_command(0x36);
	    lcd_send_data(0x48);

	    // TODO: Send display ON command
	    lcd_send_command(0x29);

	    // TODO: Turn backlight on
	    GPIOA->BSRR = (0x1u << 8);
	}
