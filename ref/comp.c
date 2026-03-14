// IMPORTANT: The temperature sensor needs to be connected to PA2.
/*
 * Nic Ball
 * 03/04/26
 * ECE 433
 */

#include "hal.h"
#include "stm32l552xx.h"

#include <stddef.h>

int main(void) {
    clocks_enable();
    // red LED
    gpio_configure_push_pull_low_speed(GPIOA, 9, GPIO_MODE_OUTPUT);

    adc_enable(ADC1, ADC_IN_PA2, ADC_MODE_SINGLE);
    uint32_t threshold = adc_blocking_read(ADC1) + 120;

    dac_ch1_enable(DAC1_MODE_INTERNAL);
    dac_write(DAC1, threshold);

    // enable COMP1 clock
    BIT_SET(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN_Pos);
    // set COMP1 negative input to DAC CH1
    COMP1->CSR &= ~COMP_CSR_INMSEL_Msk;
    COMP1->CSR |= 0b100 << COMP_CSR_INMSEL_Pos;
    // set COMP1 positive input to PA2
    COMP1->CSR &= ~COMP_CSR_INPSEL_Msk;
    COMP1->CSR |= 0b10 << COMP_CSR_INPSEL_Pos;
    // enable COMP1
    BIT_SET(COMP1->CSR, COMP_CSR_EN_Pos);

    for (;;) {
        // NOTE: In the spec it said use an interrupt but in the slides it said
        // to use CSR. Since CSR is much simpler, I chose to poll it. This
        // makes the red LED move smoothly between fully on and fully off.
        PIN_SET(GPIOA, 9, BIT_READ(COMP1->CSR, COMP_CSR_VALUE_Pos));
    }
}
