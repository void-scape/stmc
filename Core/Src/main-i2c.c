#include "hal.h"
#include "stm32l552xx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    // lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);

    gpio_configure_pin(GPIOB, 8, GPIO_MODE_ALTERNATE, GPIO_OUTPUT_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
    gpio_configure_pin(GPIOB, 9, GPIO_MODE_ALTERNATE, GPIO_OUTPUT_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);

    I2C1->TIMINGR = (1 << I2C_TIMINGR_PRESC_Pos) | (0x13 << I2C_TIMINGR_SCLL_Pos) |
                    (0xf << I2C_TIMINGR_SCLH_Pos) | (0x2 << I2C_TIMINGR_SDADEL_Pos) |
                    (0x4 << I2C_TIMINGR_SCLDEL_Pos);

    // set MCP23008 address
    I2C1->CR2 |= 1 << 6;
    // write to slave
    BIT_CLEAR(I2C1->CR2, I2C_CR2_RD_WRN_Pos);
    I2C1->CR2 |= 1 << I2C_CR2_NBYTES_Pos;
    // automatically send NACK when finished
    BIT_SET(I2C1->CR2, I2C_CR2_AUTOEND_Pos);
    // enable I2C1
    BIT_SET(I2C1->CR1, 0);

    // start sending the data
    BIT_SET(I2C1->CR2, I2C_CR2_START_Pos);

    while (!BIT_READ(I2C1->ISR, I2C_ISR_TXE_Pos)) {}

    // configure the pins to write in the device
    I2C1->TXDR = 0;

    while (!BIT_READ(I2C1->ISR, I2C_ISR_TC_Pos)) {}

    // read from slave
    BIT_SET(I2C1->CR2, I2C_CR2_RD_WRN_Pos);
    // start reading the data
    BIT_SET(I2C1->CR2, I2C_CR2_START_Pos);

    while (!BIT_READ(I2C1->ISR, I2C_ISR_RXNE_Pos)) {}

    uint32_t val = I2C1->RXDR;

    for (;;) {}

    // lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);

    // // ADC is accepting input from the output of the OPAMP1
    // adc_enable(ADC1, ADC_IN_OPAMP1, ADC_MODE_SINGLE);

    // // OPAMP1_VINP
    // gpio_configure_push_pull_low_speed(GPIOA, 0, GPIO_MODE_ANALOG);
    // // enable OPAMP clock
    // BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_OPAMPEN_Pos);
    // // internal PGA enable, gain programmed in PGA_GAIN
    // // PGA_GAIN is already 2
    // OPAMP1->CSR |= 0b10 << OPAMP1_CSR_OPAMODE_Pos;
    // // inverting input not externally connected
    // OPAMP1->CSR |= 0b11 << OPAMP1_CSR_VMSEL_Pos;
    // // enable OPAMP1
    // BIT_SET(OPAMP1->CSR, OPAMP1_CSR_OPAEN_Pos);
    //
    // for (;;) {
    //     char float_buf[32];
    //     uint32_t adc_data = adc_blocking_read(ADC1);
    //     format_float(float_buf, mcp9701_to_fahrenheit(adc_data));
    //     lpuart_blocking_printf("$%s;", float_buf);
    //     delay_ms(100);
    // }
}
