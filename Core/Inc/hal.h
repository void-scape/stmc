/*
 * Nic Ball
 * Updated 02/26/26
 * ECE 433
 *
 * General HAL utilities built during the labs for this course.
 */

#ifndef HAL_H
#define HAL_H

#include "stm32l552xx.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define BIT_SET(INT, BIT) ((INT) |= (1 << (BIT)))
#define BIT_CLEAR(INT, BIT) ((INT) &= ~(1 << (BIT)))
#define BIT_TOGGLE(INT, BIT) ((INT) ^= (1 << (BIT)))
#define BIT_READ(INT, BIT) (((INT) >> (BIT)) & 1)

// IRQ

void irq_enable(IRQn_Type irqn, uint32_t priority) {
    __disable_irq();
    NVIC_SetPriority(irqn, priority);
    NVIC_EnableIRQ(irqn);
    __enable_irq();
}

void irq_disable(IRQn_Type irqn) {
    __disable_irq();
    NVIC_DisableIRQ(irqn);
    __enable_irq();
}

// CLOCKS

void clocks_enable() {
    // Enable Clock to SYSCFG & EXTI
    RCC->APB2ENR |= 1;
    // Power interface clock
    RCC->APB1ENR1 |= 1 << 28;
    // LPUART1 clock
    RCC->APB1ENR2 |= 0x1;
    // LPUART1 clock = HSI16
    RCC->CCIPR1 |= 0x800;
    RCC->CCIPR1 &= ~0x400;
    // SYSCLK = HSI16
    RCC->CFGR |= 0x1;
    // Enable MSI + HSI16
    RCC->CR |= 0x161;
    // Enable Clock to SYSCFG & EXTI
    RCC->APB2ENR |= 1;
}

// TIMERS

// SysTick delay with ms resolution.
void delay_ms(uint32_t ms) {
    SysTick->CTRL = 0;
    // Use processor clock.
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
    // 1ms interval, subtracting 1 as per the manual:
    // ```
    // To generate a multi-shot timer with a period of N processor clock cycles,
    // use a RELOAD value of N-1. For example, if the SysTick interrupt is required
    // every 100 clock pulses, set RELOAD to 99.
    // ```
    SysTick->LOAD = 16000 - 1;
    // Reset val which also resets `SysTick_CTRL_COUNTFLAG_Pos`.
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    // For every `ms`, wait for the timer to count down.
    for (int i = 0; i < ms; i++) {
        // Reading `CTRL` seems to reset this value because this code works, but
        // the manual isn't explicit about this.
        while (!BIT_READ(SysTick->CTRL, SysTick_CTRL_COUNTFLAG_Pos)) {}
    }
}

typedef enum {
    TIM_TIM1,
    TIM_TIM4,
} Tim;

typedef enum {
    TIM_INTERRUPT_NONE = 0,
    TIM4_INTERRUPT_DISABLE,
    TIM4_INTERRUPT_ENABLE,
} Tim_Interrupt;

// Configure `tim` to fire every `ms` and optionally enable the `TIM4_IRQn`.
void tim_enable_ms(Tim tim, Tim_Interrupt interrupt, uint32_t ms) {
    // Enable tim clock
    switch (tim) {
    case TIM_TIM1: {
        BIT_SET(RCC->APB2ENR, RCC_APB2ENR_TIM1EN_Pos);
        break;
    }
    case TIM_TIM4: {
        BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_TIM4EN_Pos);
        break;
    }
    }

    TIM_TypeDef* tp;
    switch (tim) {
    case TIM_TIM1: {
        tp = TIM1;
        break;
    }
    case TIM_TIM4: {
        tp = TIM4;
        break;
    }
    default:
        return;
    }

    tp->CR1 = 0;
    // 1ms tick
    tp->PSC = 16 * 1000 - 1;
    tp->ARR = ms - 1;
    // Clear counter
    tp->CNT = 0;
    switch (interrupt) {
    case TIM_INTERRUPT_NONE: {
        break;
    }
    case TIM4_INTERRUPT_ENABLE: {
        // Set update interrupt enable
        TIM4->DIER |= 1;
        irq_enable(TIM4_IRQn, 0);
        break;
    }
    case TIM4_INTERRUPT_DISABLE:
    default: {
        // NOTE: Don't actually disable, I haven't checked if there is a hander here
        // that does something important!
        // TODO: This will not behave as expected if configured later.
    }
    }
    // Enable tim
    BIT_SET(tp->CR1, TIM_CR1_CEN_Pos);
}

// GPIO

#define PIN_TOGGLE(GPIOX, PIN) BIT_TOGGLE((GPIOX)->ODR, (PIN))
#define PIN_HIGH(GPIOX, PIN) BIT_SET((GPIOX)->ODR, (PIN))
#define PIN_LOW(GPIOX, PIN) BIT_CLEAR((GPIOX)->ODR, (PIN))
#define PIN_READ(GPIOX, PIN) BIT_READ((GPIOX)->IDR, (PIN))

// Enable the clocks for the A, B, C, and E GPIO ports.
void gpio_clocks_enable() {
    RCC->AHB2ENR |=
        RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOEEN;
}

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_ALTERNATE,
    GPIO_MODE_ANALOG,
} GPIO_Mode;

typedef enum {
    GPIO_OUTPUT_TYPE_PUSH_PULL = 0,
    GPIO_OUTPUT_TYPE_OPEN_DRAIN = 1,
} GPIO_OutputType;

typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM,
    GPIO_SPEED_HIGH,
    GPIO_SPEED_VERY_HIGH,
} GPIO_Speed;

typedef enum {
    GPIO_PULLUPDOWN_NO_PULLUP_DOWN = 0,
    GPIO_PULLUPDOWN_PULL_UP,
    GPIO_PULLUPDOWN_PULL_DOWN,
} GPIO_PullUpDown;

// Configure GPIO `pin` in `port`.
void gpio_configure_pin(GPIO_TypeDef* port, uint32_t pin, GPIO_Mode mode, GPIO_OutputType type,
                        GPIO_Speed speed, GPIO_PullUpDown pupd) {
    switch (mode & 0b11) {
    case GPIO_MODE_INPUT: {
        port->MODER &= ~(1 << (pin * 2));
        port->MODER &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_MODE_OUTPUT: {
        port->MODER |= (1 << (pin * 2));
        port->MODER &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_MODE_ALTERNATE: {
        port->MODER &= ~(1 << (pin * 2));
        port->MODER |= (1 << (pin * 2 + 1));
        break;
    }
    case GPIO_MODE_ANALOG: {
        port->MODER |= (1 << (pin * 2));
        port->MODER |= (1 << (pin * 2 + 1));
        break;
    }
    }

    switch (type) {
    case GPIO_OUTPUT_TYPE_PUSH_PULL: {
        port->OTYPER &= ~(1UL << pin);
        break;
    }
    case GPIO_OUTPUT_TYPE_OPEN_DRAIN: {
        port->OTYPER |= (1UL << pin);
        break;
    }
    }

    switch (speed & 0b11) {
    case GPIO_SPEED_LOW: {
        port->OSPEEDR &= ~(1 << (pin * 2));
        port->OSPEEDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_SPEED_MEDIUM: {
        port->OSPEEDR |= (1 << (pin * 2));
        port->OSPEEDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_SPEED_HIGH: {
        port->OSPEEDR &= ~(1 << (pin * 2));
        port->OSPEEDR |= (1 << (pin * 2 + 1));
        break;
    }
    case GPIO_SPEED_VERY_HIGH: {
        port->OSPEEDR |= (1 << (pin * 2));
        port->OSPEEDR |= (1 << (pin * 2 + 1));
        break;
    }
    }

    switch (pupd & 0b11) {
    case GPIO_PULLUPDOWN_NO_PULLUP_DOWN: {
        port->PUPDR &= ~(1 << (pin * 2));
        port->PUPDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_PULLUPDOWN_PULL_UP: {
        port->PUPDR |= (1 << (pin * 2));
        port->PUPDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case GPIO_PULLUPDOWN_PULL_DOWN: {
        port->PUPDR &= ~(1 << (pin * 2));
        port->PUPDR |= (1 << (pin * 2 + 1));
        break;
    }
    }

    if ((mode & 0b11) == GPIO_MODE_OUTPUT) {
        // I don't think this makes sense for anything other than output, but
        // I am pretty sure the HAL always sets this to low...
        PIN_LOW(port, pin);
    }
}

// Configure GPIO `pin` in `port` with push pull output type in low speed.
void gpio_configure_push_pull_low_speed(GPIO_TypeDef* port, uint32_t pin, GPIO_Mode mode) {
    gpio_configure_pin(port, pin, mode, GPIO_OUTPUT_TYPE_PUSH_PULL, GPIO_SPEED_LOW,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
}

// LPUART

typedef enum {
    LPUART_INTERRUPT_DISABLE,
    LPUART_INTERRUPT_ENABLE,
} LPUart_Interrupt;

// Enable the LPUART with pins PG7 and PG8 at 115200 baud rate, optionally enabling
// the `LPUART1_IRQn`.
void lpuart_enable(LPUart_Interrupt interrupt) {
    // Enable Clock to PWR Interface
    BIT_SET(RCC->APB1ENR1, 28);
    // Enable Clock to GPIOG
    BIT_SET(RCC->AHB2ENR, 6);
    // Enable Clock to LPUART
    BIT_SET(RCC->APB1ENR2, 0);
    // Select the high speed internal (HSI) oscillator as the clock to LPUART1 (16MHz)
    BIT_SET(RCC->CCIPR1, 11);
    BIT_CLEAR(RCC->CCIPR1, 10);
    // HSI16 clock enable
    BIT_SET(RCC->CR, 8);

    // Enable GPIOG power
    BIT_SET(PWR->CR2, 9);

    // Set PG7 to alternate mode with AF8
    gpio_configure_pin(GPIOG, 7, GPIO_MODE_ALTERNATE, GPIO_OUTPUT_TYPE_PUSH_PULL,
                       GPIO_SPEED_VERY_HIGH, GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
    BIT_SET(GPIOG->AFR[0], 31);
    BIT_CLEAR(GPIOG->AFR[0], 30);
    BIT_CLEAR(GPIOG->AFR[0], 29);
    BIT_CLEAR(GPIOG->AFR[0], 28);
    // Set PG8 to alternate mode with AF8
    gpio_configure_pin(GPIOG, 8, GPIO_MODE_ALTERNATE, GPIO_OUTPUT_TYPE_PUSH_PULL,
                       GPIO_SPEED_VERY_HIGH, GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
    BIT_SET(GPIOG->AFR[1], 3);
    BIT_CLEAR(GPIOG->AFR[1], 2);
    BIT_CLEAR(GPIOG->AFR[1], 1);
    BIT_CLEAR(GPIOG->AFR[1], 0);

    // Initialize LPUART
    // NOTE: The spec says 57600 baud rate, but that is not a whole number here,
    // so I will keep the 115200 configuration.
    // 256*16000000/115200 = 35555
    LPUART1->BRR = 35555;
    // The order here is very sensitive, and this section gave me a lot of trouble.
    //
    // In the original code, the CR1 is configured with 0xD, which enables RE, TE,
    // and UE, but it does not set the FIFOEN. Without setting the FIFOEN, transmitting
    // data worked but receiving data didn't. Then, it turns, out, you have to set
    // the FIFOEN _before_ enabling the LPUART with UE.
    LPUART1->CR1 |= USART_CR1_FIFOEN_Msk;
    LPUART1->CR1 |= USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk;

    switch (interrupt) {
    case LPUART_INTERRUPT_ENABLE: {
        irq_enable(LPUART1_IRQn, 0);
        break;
    }
    case LPUART_INTERRUPT_DISABLE:
    default: {
        // NOTE: Don't actually disable, I haven't checked if there is a hander here
        // that does something important!
        // TODO: This will not behave as expected if configured later.
    }
    }
}

// NOTE: I am aware there is a library to link with for this but I don't want to
// deal with that. Also, no reason to bloat the binary for this simple behavior!
void format_float(char* buffer, float val) {
    int whole = (int)val;
    int fraction = (int)((val - (float)whole) * 100.0f);
    sprintf(buffer, "%d.%02d", whole, abs(fraction));
}

// BOARD

typedef enum {
    BUTTON_INTERRUPT_DISABLE,
    BUTTON_INTERRUPT_ENABLE,
} Button_Interrupt;

// Enable the NUCLEO-L552ZE-Q user push button, configured to PC13. Optionally
// enable `EXTI13_IRQn`.
void button_enable(Button_Interrupt interrupt) {
    gpio_configure_push_pull_low_speed(GPIOC, 13, GPIO_MODE_INPUT);
    switch (interrupt) {
    case BUTTON_INTERRUPT_ENABLE: {
        // Configure EXTI input to PC13
        EXTI->EXTICR[3] = (0x2) << 8;
        // Trigger on rising edge of PC13
        EXTI->RTSR1 |= 1 << 13;
        // Interrupt mask disable for PC13
        EXTI->IMR1 |= 1 << 13;
        irq_enable(EXTI13_IRQn, 0);
        break;
    }
    case BUTTON_INTERRUPT_DISABLE:
    default: {
        // NOTE: Don't actually disable, I haven't checked if there is a hander here
        // that does something important!
        // TODO: This will not behave as expected if configured later.
    }
    }
}

// ADC

typedef enum {
    ADC_IN_PC0 = 1,
    ADC_IN_PC1 = 2,
} ADC_In;

void configure_adc_pin(ADC_In channel) {
    uint8_t pin = (channel == ADC_IN_PC0) ? 0 : 1;
    gpio_configure_push_pull_low_speed(GPIOC, pin, GPIO_MODE_ANALOG);
}

typedef enum {
    ADC_INJECT_NONE = 0,
    ADC_INJECT_PC0 = 1,
    ADC_INJECT_PC1 = 2,
} ADC_Inject;

void adc_enable_ext_trig(ADC_TypeDef* adc, ADC_In channel, uint32_t ms, ADC_Inject inject) {
    tim_enable_ms(TIM_TIM1, TIM4_INTERRUPT_ENABLE, ms);
    // Set output to toggle on match
    TIM1->CCMR1 = 0x30;
    // Output will toggle when CNT==CCR1
    TIM1->CCR1 = 1;
    // Main output enable
    BIT_SET(TIM1->BDTR, TIM_BDTR_MOE_Pos);
    // Enable CH1 compare mode
    TIM1->CCER |= 1;
    // Clear counter
    TIM1->CNT = 0;

    configure_adc_pin(channel);
    // Enable ADC Clock
    BIT_SET(RCC->AHB2ENR, RCC_AHB2ENR_ADCEN_Pos);
    // Route SYSCLK (HCLK) to ADC
    RCC->CCIPR1 |= 0x3 << 28;
    // Turn on ADC Voltage Regulator
    BIT_CLEAR(adc->CR, ADC_CR_DEEPPWD_Pos);
    BIT_SET(adc->CR, ADC_CR_ADVREGEN_Pos);
    // External Trigger Enable and Polarity; EXTEN= 0b01 for rising edge
    BIT_SET(adc->CFGR, ADC_CFGR_EXTEN_Pos);
    BIT_CLEAR(adc->CFGR, ADC_CFGR_EXTEN_Pos + 1);
    // External trigger (EXT0) is connected to TIM1_CH1; EXTSEL=0000
    BIT_CLEAR(adc->CFGR, 6);
    BIT_CLEAR(adc->CFGR, 7);
    BIT_CLEAR(adc->CFGR, 8);
    BIT_CLEAR(adc->CFGR, 9);
    // Wait for the voltage regulator to stabilize
    delay_ms(10);

    // Set up ADC
    adc->SQR1 = (channel << ADC_SQR1_SQ1_Pos);
    // Enable Interrupt so that the Timer trigger the ADC to do conversion
    // Once the conversion is done, the EOC flag is raised and the IRQ handler for ADC is called.
    irq_enable(ADC1_2_IRQn, 0);
    adc->IER |= 1 << 2;
    adc->CR |= 1;

    switch (inject) {
    case ADC_INJECT_NONE: {
        break;
    }
    case ADC_INJECT_PC0:
    case ADC_INJECT_PC1: {
        ADC1->JSQR = inject << ADC_JSQR_JSQ1_Pos;
        break;
    }
    }

    // Disable overrun mode
    BIT_SET(adc->CFGR, ADC_CFGR_OVRMOD_Pos);
    // Wait until ADC is ready
    while (BIT_READ(adc->ISR, ADC_ISR_ADRDY_Pos) == 0) {}
    // Start Conversion
    BIT_SET(adc->CR, 2);
    // Enable TIM1
    TIM1->CR1 = 1;
}

void adc_enable(ADC_TypeDef* adc, ADC_In channel) {
    configure_adc_pin(channel);
    // Enable ADC Clock
    BIT_SET(RCC->AHB2ENR, RCC_AHB2ENR_ADCEN_Pos);
    // Route SYSCLK (HCLK) to ADC
    RCC->CCIPR1 |= 0x3 << 28;
    // Turn on ADC Voltage Regulator
    BIT_CLEAR(adc->CR, ADC_CR_DEEPPWD_Pos);
    BIT_SET(adc->CR, ADC_CR_ADVREGEN_Pos);
    // Wait for the voltage regulator to stabilize
    delay_ms(10);
    // Set up ADC
    adc->SQR1 = (channel << ADC_SQR1_SQ1_Pos);
    // Enable ADC
    BIT_SET(adc->CR, ADC_CR_ADEN_Pos);
    // Wait until ADC is ready
    while (BIT_READ(adc->ISR, ADC_ISR_ADRDY_Pos) == 0) {}
}

// Read from the `adc` injected data register.
uint32_t adc_inject_blocking_read(ADC_TypeDef* adc) {
    // Start conversion
    BIT_SET(adc->CR, ADC_CR_JADSTART_Pos);
    // Wait for conversion complete
    while (!BIT_READ(adc->ISR, ADC_ISR_JEOC_Pos)) {}
    // Clear injection
    ADC1->ISR |= ADC_ISR_JEOC;
    return ADC1->JDR1;
}

// Read from the `adc` regular data register.
uint32_t adc_blocking_read(ADC_TypeDef* adc) {
    // Start conversion
    BIT_SET(adc->CR, ADC_CR_ADSTART_Pos);
    // Wait for conversion complete
    while (BIT_READ(adc->ISR, ADC_ISR_EOC_Pos) == 0) {}
    // Read result and clear EOC
    // Result is 12 bits, so the value is masked
    return (adc->DR) & 0xfff;
}

float adc_norm(uint32_t adc_value) {
    return (float)adc_value * (1.0f / 4095.0f);
}

#endif // HAL_H
