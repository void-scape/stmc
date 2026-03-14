/*
 * Nic Ball
 * 02/12/26
 * ECE 433
 */

// #include "gpio.h"
#include "stm32l552xx.h"

#include <stddef.h>
#include <stdio.h>

// gpio.h

/*
 * Nic Ball
 * 01/29/26
 * ECE 433
 *
 * Some HAL utility functions for enabling and configuring GPIO. I implemented
 * this by referencing the HAL file.
 */

#define BIT_SET(INT, BIT) ((INT) |= (1 << (BIT)))
#define BIT_CLEAR(INT, BIT) ((INT) &= ~(1 << (BIT)))
#define BIT_TOGGLE(INT, BIT) ((INT) ^= (1 << (BIT)))
#define BIT_READ(INT, BIT) (((INT) >> (BIT)) & 1)

#define PIN_TOGGLE(GPIOX, PIN) BIT_TOGGLE((GPIOX)->ODR, (PIN))
#define PIN_HIGH(GPIOX, PIN) BIT_SET((GPIOX)->ODR, (PIN))
#define PIN_LOW(GPIOX, PIN) BIT_CLEAR((GPIOX)->ODR, (PIN))
#define PIN_READ(GPIOX, PIN) BIT_READ((GPIOX)->IDR, (PIN))

// Enable the clocks for the A, B, and C GPIO ports.
void gpio_clocks_enable() {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
}

typedef enum {
    MODE_INPUT = 0,
    MODE_OUTPUT,
    MODE_ALTERNATE,
    MODE_ANALOG,
} GPIO_Mode;

typedef enum {
    PUSH_PULL = 0,
    OPEN_DRAIN = 1,
} GPIO_OutputType;

typedef enum {
    SPEED_LOW = 0,
    SPEED_MEDIUM,
    SPEED_HIGH,
    SPEED_VERY_HIGH,
} GPIO_Speed;

typedef enum {
    NO_PULLUP_DOWN = 0,
    PULL_UP,
    PULL_DOWN,
} GPIO_PullUpDown;

// Configure GPIO `pin` in `port`.
void gpio_configure_pin(GPIO_TypeDef* port, uint32_t pin, GPIO_Mode mode, GPIO_OutputType type,
                        GPIO_Speed speed, GPIO_PullUpDown pupd) {
    switch (mode & 0b11) {
    case MODE_INPUT: {
        port->MODER &= ~(1 << (pin * 2));
        port->MODER &= ~(1 << (pin * 2 + 1));
        break;
    }
    case MODE_OUTPUT: {
        port->MODER |= (1 << (pin * 2));
        port->MODER &= ~(1 << (pin * 2 + 1));
        break;
    }
    case MODE_ALTERNATE: {
        port->MODER &= ~(1 << (pin * 2));
        port->MODER |= (1 << (pin * 2 + 1));
        break;
    }
    case MODE_ANALOG: {
        port->MODER |= (1 << (pin * 2));
        port->MODER |= (1 << (pin * 2 + 1));
        break;
    }
    }

    if (type == PUSH_PULL) {
        port->OTYPER &= ~(1UL << pin);
    } else {
        port->OTYPER |= (1UL << pin);
    }

    switch (speed & 0b11) {
    case SPEED_LOW: {
        port->OSPEEDR &= ~(1 << (pin * 2));
        port->OSPEEDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case SPEED_MEDIUM: {
        port->OSPEEDR |= (1 << (pin * 2));
        port->OSPEEDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case SPEED_HIGH: {
        port->OSPEEDR &= ~(1 << (pin * 2));
        port->OSPEEDR |= (1 << (pin * 2 + 1));
        break;
    }
    case SPEED_VERY_HIGH: {
        port->OSPEEDR |= (1 << (pin * 2));
        port->OSPEEDR |= (1 << (pin * 2 + 1));
        break;
    }
    }

    switch (pupd & 0b11) {
    case NO_PULLUP_DOWN: {
        port->PUPDR &= ~(1 << (pin * 2));
        port->PUPDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case PULL_UP: {
        port->PUPDR |= (1 << (pin * 2));
        port->PUPDR &= ~(1 << (pin * 2 + 1));
        break;
    }
    case PULL_DOWN: {
        port->PUPDR &= ~(1 << (pin * 2));
        port->PUPDR |= (1 << (pin * 2 + 1));
        break;
    }
    default:
        break;
    }

    if ((mode & 0b11) == MODE_OUTPUT) {
        // I don't think this makes sense for anything other than output, but
        // I am pretty sure the HAL always sets this to low...
        PIN_LOW(port, pin);
    }
}

// provided

void clocks_enable() {
    RCC->APB1ENR1 |= 1 << 28; // Power interface clock
    RCC->APB1ENR2 |= 0x1;     // LPUART1 clock
    RCC->CCIPR1 |= 0x800;     // LPUART1 clock = HSI16
    RCC->CCIPR1 &= ~0x400;
    RCC->CFGR |= 0x1; // SYSCLK = HSI16
    RCC->CR |= 0x161; // Enable MSI + HSI16
    gpio_clocks_enable();
}

void lpuart_init() {
    PWR->CR2 |= 0x200; // Enable VDDIO2 (Port G)

    RCC->AHB2ENR |= (1 << 6); // Enable GPIOG clock

    // PG7 = LPUART1_TX (AF8)
    GPIOG->MODER &= ~(0x3 << (2 * 7));
    GPIOG->MODER |= (0x2 << (2 * 7));
    GPIOG->AFR[0] &= ~(0xF << (4 * 7));
    GPIOG->AFR[0] |= (0x8 << (4 * 7));

    // PG8 = LPUART1_RX (AF8)
    GPIOG->MODER &= ~(0x3 << (2 * 8));
    GPIOG->MODER |= (0x2 << (2 * 8));
    GPIOG->AFR[1] &= ~(0xF << (4 * 0));
    GPIOG->AFR[1] |= (0x8 << (4 * 0));

    // UART configuration
    LPUART1->PRESC = 0;
    LPUART1->BRR = 0x8AE3; // 115200 baud @ 16 MHz
    LPUART1->CR1 = 0;
    LPUART1->CR1 |= (1 << 3) | (1 << 2); // TE + RE
    LPUART1->CR2 = 0;
    LPUART1->CR3 = 0;
    LPUART1->CR1 |= 1; // Enable UART
}

void lpuart_print(char msg[]) {
    uint8_t i = 0;
    while (msg[i] != '\0') {
        while (!(LPUART1->ISR & 0x0080)) {}
        LPUART1->TDR = (msg[i++] & 0xFF);
    }
}

// todo

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

// TIM5 delay with µs resolution.
void delay_us(uint32_t us) {
    BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_TIM5EN_Pos);
    TIM5->CR1 = 0;
    TIM5->ARR = us - 1;
    // `- 1` such that the timer doesn't divide by zero, as per the manual:
    // ```
    // The counter clock frequency CK_CNT is equal to fCK_PSC / (PSC[15:0] + 1).
    // ```
    TIM5->PSC = 16 - 1;
    TIM5->CNT = 0;
    TIM5->CR1 |= TIM_CR1_CEN_Msk;
    // Poll the update interrupt flag until the counter finishes
    while (!BIT_READ(TIM5->SR, TIM_SR_UIF_Pos)) {}
    // Software needs to clear this flag
    BIT_CLEAR(TIM5->SR, TIM_SR_UIF_Pos);
}

// TIM3 frequency generator — PC7 toggles at 50% duty cycle
void freq_gen(uint16_t val) {
    gpio_configure_pin(GPIOC, 7, MODE_ALTERNATE, PUSH_PULL, SPEED_LOW, NO_PULLUP_DOWN);

    // Set the AF2 to connect PC7 to TIM3_CH2
    GPIOC->AFR[0] &= ~(0xF << 28);
    BIT_SET(GPIOC->AFR[0], 29);

    // enable TIM3 clock
    BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_TIM3EN_Pos);
    // Divided 16MHz source clk by 16000, for 1ms tick
    TIM3->PSC = 16000 - 1;
    TIM3->ARR = val - 1;
    // Set output to toggle on match
    TIM3->CCMR1 = 0x3000;
    // Output will toggle when CNT==CCR2
    TIM3->CCR2 = 1;
    // Enable CH2 compare mode
    TIM3->CCER |= 1 << 4;
    // Clear counter
    TIM3->CNT = 0;
    // Enable TIM3
    TIM3->CR1 = 1;

    for (;;) {}
}

// LPTIM2 edge counter — PD12 as external clock input
void edge_counter() {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    gpio_configure_pin(GPIOD, 12, MODE_ALTERNATE, PUSH_PULL, SPEED_LOW, NO_PULLUP_DOWN);

    // AF14 to connect PD12 to LPTIM2_IN1
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~(0xF << 16)) | (0xE << 16);

    // Enable LPTIM2 clock
    BIT_SET(RCC->APB1ENR2, RCC_APB1ENR2_LPTIM2EN_Pos);
    LPTIM2->CR = 0;
    // Configure LPTIM2 to use external clock mode
    LPTIM2->CFGR = 1 | (1 << 23);
    // Enable
    BIT_SET(LPTIM2->CR, 0);
    LPTIM2->CNT = 0;
    LPTIM2->CR |= 0b100;
    // reset at 512, gives a nice sawtooth wave in the plotter
    LPTIM2->ARR = 512;

    while (1) {
        // Connect a wire from TX pin on CN6 connector to PD12.
        char buff[100];
        // NOTE: I am on macos so I had to use an online tool which requires this format
        // instead of $%d;
        // - https://web-serial-plotter.atomic14.com/
        sprintf(buff, "%d\n", (int)LPTIM2->CNT);
        // sprintf(buff, "$%d;", (int)LPTIM2->CNT);
        lpuart_print(buff);
        for (int i = 0; i < 10000; i++) {}
    }
}

// TIM4 PWM — PB7 output, duty cycle = val%
void pwm(uint32_t val) {
    // Connect PB7 to TIM4_CH2 with AF2
    //
    // GPIOB clock is already enabled
    gpio_configure_pin(GPIOB, 7, MODE_ALTERNATE, PUSH_PULL, SPEED_LOW, NO_PULLUP_DOWN);
    GPIOB->AFR[0] &= ~(0xF << 28);
    GPIOB->AFR[0] |= (0x2) << 28;

    // configure TIM4
    BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_TIM4EN_Pos);
    TIM4->CR1 = 0;

    // No prescalar so that it's not flashy and looks good
    TIM4->PSC = 0;
    TIM4->ARR = 100 - 1;  // divided by 100 so the counter counts 0 to 99
    TIM4->CCMR1 = 0x6000; // Set output mode to pwm 1 where output is 1 if CNT < CCR,
    TIM4->CCR2 = val;
    TIM4->CCER |= 1 << 4; // Enable CH2 compare mode
    TIM4->CR1 = 1;        // Enable TIM4
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    lpuart_init();
    // red LED
    gpio_configure_pin(GPIOA, 9, MODE_OUTPUT, PUSH_PULL, SPEED_LOW, NO_PULLUP_DOWN);

    // // TEST CASE 1: SysTick delay
    // while (1) {
    //     PIN_TOGGLE(GPIOA, 9);
    //     delay_ms(1000);
    // }

    // // TEST CASE 2: TIM5 delay
    // while (1) {
    //     PIN_TOGGLE(GPIOA, 9);
    //     delay_us(1000000);
    // }

    // // TEST CASE 3: Frequency generator
    // // made it 1s instead of 5s
    // freq_gen(1000);

    // // TEST CASE 4: Edge counter
    // edge_counter();

    // // TEST CASE 5: PWM dimming
    // int i = 0;
    // while (1) {
    //     pwm(i);
    //     // I made this much faster so it looks good
    //     delay_ms(50);
    //     i = (i + 1) % 101; // 0–100
    // }

    return 0;
}
