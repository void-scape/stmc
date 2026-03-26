/*
 * Nic Ball
 * 03/13/26
 * ECE 433
 */

#include "hal.h"
#include "stm32l552xx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define BUF_LEN 127
#define PINS 4
#define CS 12
#define SDO 14
#define SDI 15
#define SCK 13

// The relevant commands from the 25LC010A datasheet:
// https://ww1.microchip.com/downloads/en/DeviceDoc/21832H.pdf
typedef enum {
    INSTR_READ = 0b011,
    INSTR_WRITE = 0b010,
    INSTR_WREN = 0b110,
    INSTR_RDSR = 0b101,
} Instr;

uint32_t spi(uint32_t byte) {
    // NOTE: The data transfer depends on the type that is written to DR, so I
    // am casting to bytes.
    while (!BIT_READ(SPI1->SR, SPI_SR_TXE_Pos)) {}
    *(uint8_t*)&SPI1->DR = byte;
    while (!BIT_READ(SPI1->SR, SPI_SR_RXNE_Pos)) {}
    return *(uint8_t*)&SPI1->DR;
}

// Write `byte` to `addr` in the 25LC010A.
void write_addr(uint32_t addr, uint32_t byte) {
    // enable write latch
    PIN_LOW(GPIOE, CS);
    spi(INSTR_WREN);
    while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
    PIN_HIGH(GPIOE, CS);

    // write `byte` to `addr`
    PIN_LOW(GPIOE, CS);
    spi(INSTR_WRITE);
    spi(addr);
    spi(byte);
    while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
    PIN_HIGH(GPIOE, CS);

    // wait for the data to be written
    for (;;) {
        PIN_LOW(GPIOE, CS);
        spi(INSTR_RDSR);
        uint32_t sr = spi(0);
        while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
        PIN_HIGH(GPIOE, CS);
        // bit 0 is the WIP flag, if false, the write is finished
        if (!BIT_READ(sr, 0)) {
            return;
        }
    }
}

// Read a byte from `addr` in the 25LC010A.
uint32_t read_addr(uint32_t addr) {
    // write `byte` from `addr`
    PIN_LOW(GPIOE, CS);
    spi(INSTR_READ);
    spi(addr);
    uint32_t byte = spi(0);
    while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
    PIN_HIGH(GPIOE, CS);
    return byte;
}

// Clear the 25LC010A memory to 0.
void clear(void) {
    for (int i = 0; i < BUF_LEN; i++) {
        write_addr(i, 0);
    }
}

static char buf[BUF_LEN];
static uint32_t start = 0;
static uint32_t len = 0;

void EXTI13_IRQHandler(void) {
    EXTI->RPR1 |= 1 << 13;
    // print all of the string data in the 25LC010A
    for (int i = 0; i < BUF_LEN; i++) {
        uint32_t data = read_addr(i);
        if (!data) {
            break;
        }
        lpuart_blocking_printf("%c", data);
    }
    // reset everything
    start = 0;
    len = 0;
    clear();
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    // NOTE: Writing to the 25LC010A is slow, so reducing the baud rate prevents
    // data loss.
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 9600);
    button_enable(BUTTON_INTERRUPT_ENABLE);

    // configure the pins to the correct alternate functions
    uint32_t pins[PINS] = {CS, SDO, SDI, SCK};
    for (int i = 1; i < PINS; i++) {
        uint32_t pin = pins[i];
        gpio_configure_pin(GPIOE, pin, GPIO_MODE_ALTERNATE, GPIO_OUTPUT_TYPE_PUSH_PULL,
                           GPIO_SPEED_VERY_HIGH, GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
        // AF5 = 0b0101
        uint32_t afr_idx = pin / 8;
        uint32_t bit_pos = (pin % 8) * 4;
        BIT_SET(GPIOE->AFR[afr_idx], bit_pos);
        BIT_CLEAR(GPIOE->AFR[afr_idx], bit_pos + 1);
        BIT_SET(GPIOE->AFR[afr_idx], bit_pos + 2);
        BIT_CLEAR(GPIOE->AFR[afr_idx], bit_pos + 3);
    }
    // I am manually updating CS, so it it a digital output
    gpio_configure_pin(GPIOE, CS, GPIO_MODE_OUTPUT, GPIO_OUTPUT_TYPE_PUSH_PULL,
                       GPIO_SPEED_VERY_HIGH, GPIO_PULLUPDOWN_NO_PULLUP_DOWN);

    // enable SPI1 clock
    BIT_SET(RCC->APB2ENR, RCC_APB2ENR_SPI1EN_Pos);
    // master mode
    BIT_SET(SPI1->CR1, SPI_CR1_MSTR_Pos);
    // software slave management
    BIT_SET(SPI1->CR1, SPI_CR1_SSM_Pos);
    // internal slave select high
    BIT_SET(SPI1->CR1, SPI_CR1_SSI_Pos);
    // 8-bit data size
    SPI1->CR2 |= 0b0111 << SPI_CR2_DS_Pos;
    // RXNE event is generated if the FIFO level is greater than or equal to 1/4 (8-bit)
    // NOTE: Since we only want to send bytes, the RXNE must trigger on 8-bit data.
    BIT_SET(SPI1->CR2, SPI_CR2_FRXTH_Pos);
    // SPI enable
    BIT_SET(SPI1->CR1, SPI_CR1_SPE_Pos);

    // clear any garbage data
    clear();
    for (;;) {
        uint32_t c = lpuart_blocking_read();
        if (c == '\n' || c == '\r') {
            // append the message into the 25LC010A
            lpuart_blocking_printf("%d", len);
            for (int j = 0; j < len; j++) {
                uint32_t i = (start + j) % BUF_LEN;
                write_addr(i, buf[i]);
            }
            start = (start + len) % BUF_LEN;
            len = 0;
        } else {
            uint32_t i = (start + len++) % BUF_LEN;
            buf[i] = c;
        }
    }
}
