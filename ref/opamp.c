// IMPORTANT: The temperature sensor needs to be connected to PA0.
/*
 * Nic Ball
 * 03/04/26
 * ECE 433
 */

#include "hal.h"
#include "stm32l552xx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);
    // ADC is accepting input from the output of the OPAMP1
    adc_enable(ADC1, ADC_IN_OPAMP1, ADC_MODE_SINGLE);

    // OPAMP1_VINP
    gpio_configure_push_pull_low_speed(GPIOA, 0, GPIO_MODE_ANALOG);
    // enable OPAMP clock
    BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_OPAMPEN_Pos);
    // internal PGA enable, gain programmed in PGA_GAIN
    // PGA_GAIN is already 2
    OPAMP1->CSR |= 0b10 << OPAMP1_CSR_OPAMODE_Pos;
    // inverting input not externally connected
    OPAMP1->CSR |= 0b11 << OPAMP1_CSR_VMSEL_Pos;
    // enable OPAMP1
    BIT_SET(OPAMP1->CSR, OPAMP1_CSR_OPAEN_Pos);

    for (;;) {
        char float_buf[32];
        uint32_t adc_data = adc_blocking_read(ADC1);
        format_float(float_buf, mcp9701_to_fahrenheit(adc_data));
        lpuart_blocking_printf("$%s;", float_buf);
        delay_ms(100);
    }
}
