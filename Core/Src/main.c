// IMPORTANT: You have to reset the board before values are sent over the LPUART!
/*
 * Nic Ball
 * 02/26/26
 * ECE 433
 */

#include "hal.h"
#include "stm32l552xx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Stores the temperature recorded by the temperature sensor.
static char temp_string[100];
// Current index into the `temp_string`.
static int temp_string_index = sizeof(temp_string);
// Flag that tells the main loop whether or not it should start printing
// the `temp_string`.
//
// This is set is the `TIM4_IRQHandler` and handled in the main loop such that
// the timer interrupt doesn't stall the whole processor for the float formatting.
static bool write_temp = false;
static uint32_t adc1_data = 0;

void ADC1_2_IRQHandler() {
    adc1_data = ADC1->DR;
    write_temp = true;
}

void LPUART1_IRQHandler(void) {
    // Make sure that TDR is ready to write to so this doesn't finish preemptively.
    if (BIT_READ(LPUART1->ISR, USART_ISR_TXE_Pos)) {
        // Send `temp_string` data if it is fully exhuasted.
        if (temp_string[temp_string_index]) {
            LPUART1->TDR = temp_string[temp_string_index++];
        } else {
            // Finished sending the name, disable further interrupts from TXE.
            BIT_CLEAR(LPUART1->CR1, USART_CR1_TXEIE_Pos);
        }
    }
}

// Reads the potentiometer's value and configures the temp sampling frequency
// between 1ms..2s.
void set_sampling_freq(void) {
    float inv_norm = adc_norm(adc_inject_blocking_read(ADC1));
    int freq = (int)((1.0f - inv_norm) * 2000.0f);
    if (freq < 1) {
        freq = 1;
    }
    // ADC1 is using TIM1 in ext trigger mode.
    // TODO: Make that configurable!
    tim_enable_ms(TIM_TIM1, TIM_INTERRUPT_NONE, freq);
}

void EXTI13_IRQHandler(void) {
    EXTI->RPR1 |= 1 << 13;
    set_sampling_freq();
}

float mcp9701_to_fahrenheit(uint32_t adc_value) {
    float vout = adc_norm(adc_value) * 3.3;
    float tc = 0.0195f;
    float v0 = 0.4f;
    float ta_celsius = (vout - v0) / tc;
    return (ta_celsius * 1.8) + 32.0f;
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_ENABLE);
    button_enable(BUTTON_INTERRUPT_ENABLE);
    adc_enable_ext_trig(ADC1, ADC_IN_PC0, 1, ADC_INJECT_PC1);
    // Initialize the sampling freq so the user doesn't have to press the
    // button first.
    set_sampling_freq();
	// NOTE: It didn't require that the main loop be empty in the assignment, but
	// if I wanted to make it empty, I would just do this in the `ADC1_2_IRQHandler`.
	// This is a lot of work to do in an interrupt though...
    for (;;) {
        if (write_temp) {
            char float_buf[100];
            format_float(float_buf, mcp9701_to_fahrenheit(adc1_data));
            sprintf(temp_string, "$%s;", float_buf);
            // Reset the index so that the interrupt handler knows it needs to send data.
            temp_string_index = 0;
            write_temp = false;
            // Enable the TXE interrupt to run the lpuart handler until the `temp_string` is
            // fully sent.
            BIT_SET(LPUART1->CR1, USART_CR1_TXEIE_Pos);
        }
    }
}
