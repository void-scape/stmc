// IMPORTANT: Baud rate is 921600!
/*
 * Nic Ball
 * 03/04/26
 * ECE 433
 */

#include "hal.h"
#include "stm32l552xx.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define SINE_SAMPLES 100
static uint32_t sine[SINE_SAMPLES];
static uint32_t sine_index = 0;
void TIM4_IRQHandler(void) {
    BIT_CLEAR(TIM4->SR, TIM_SR_UIF_Pos);
    dac_write(DAC1, sine[sine_index]);
    sine_index = (sine_index + 1) % SINE_SAMPLES;
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 921600);
    // enable the ADC and configure the input to the DAC CH1 output
    adc_enable(ADC1, ADC_IN_PA4, ADC_MODE_SINGLE_CONTINUOUS);
    // generate sine data
    for (int i = 0; i < SINE_SAMPLES; i++) {
        sine[i] = (4095 / 2) * (1.0f + sin(2.0f * 3.145f * (float)i / (float)SINE_SAMPLES));
    }
    dac_ch1_enable(DAC1_MODE_EXTERNAL);
    // every 10us, write sine data into the DAC data register
    tim_enable_interrupt(TIM4, TIM4_IRQn, 10, 1);
    // write the ADC output to the serial port continuously
    for (;;) {
        uint32_t val = adc_continuous_read(ADC1);
        lpuart_blocking_printf("$%lu;", val);
    }
}
