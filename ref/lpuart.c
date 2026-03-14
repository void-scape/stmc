/*
 * Nic Ball
 * 02/05/26
 * ECE 433
 */

#include "gpio.h"
#include "stm32l562xx.h"

#include <stddef.h>

void configure_lpuart(void) {
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
    gpio_configure_pin(GPIOG, 7, MODE_ALTERNATE, PUSH_PULL, SPEED_VERY_HIGH, NO_PULLUP_DOWN);
    BIT_SET(GPIOG->AFR[0], 31);
    BIT_CLEAR(GPIOG->AFR[0], 30);
    BIT_CLEAR(GPIOG->AFR[0], 29);
    BIT_CLEAR(GPIOG->AFR[0], 28);
    // Set PG8 to alternate mode with AF8
    gpio_configure_pin(GPIOG, 8, MODE_ALTERNATE, PUSH_PULL, SPEED_VERY_HIGH, NO_PULLUP_DOWN);
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
}

int main(void) {
	configure_lpuart();

    // LEDs
    gpio_enable_clocks();
    gpio_configure_pin(GPIOA, 9, MODE_OUTPUT, PUSH_PULL, SPEED_HIGH, NO_PULLUP_DOWN);
    gpio_configure_pin(GPIOC, 7, MODE_OUTPUT, PUSH_PULL, SPEED_HIGH, NO_PULLUP_DOWN);
    gpio_configure_pin(GPIOB, 7, MODE_OUTPUT, PUSH_PULL, SPEED_HIGH, NO_PULLUP_DOWN);
    // Button
    gpio_configure_pin(GPIOC, 13, MODE_INPUT, PUSH_PULL, SPEED_LOW, NO_PULLUP_DOWN);

    int i = 0;
    int button_state = 0;
    char buf[] = "Nic Ball";

    while (1) {
		// If RXFIFO is not empty, we can read from the data register.
        if (BIT_READ(LPUART1->ISR, USART_ISR_RXNE_RXFNE_Pos) == 1) {
			// Here I am switching on the ascii codes for `r`, `g`, and `b`. The
			// uppercase variants are the lowercase ascii codes subtracted by 32.
            switch (LPUART1->RDR) {
			case (114 - 32):
            case 114: {
                PIN_TOGGLE(GPIOA, 9);
                break;
            }
			case (103 - 32):
            case 103: {
                PIN_TOGGLE(GPIOC, 7);
                break;
            }
			case (98 - 32):
            case 98: {
                PIN_TOGGLE(GPIOB, 7);
                break;
            }
            default:
                PIN_LOW(GPIOA, 9);
                PIN_LOW(GPIOC, 7);
                PIN_LOW(GPIOB, 7);
                break;
            }
        }

		// If TXFIFO is not full, we can write a byte to it.
        if (BIT_READ(LPUART1->ISR, USART_CR1_TXEIE_TXFNFIE_Pos) == 1) {
			// Only writes to the serial connection if the button has just been
			// pressed.

            if (PIN_READ(GPIOC, 13) && !button_state) {
                button_state = 1;
                LPUART1->TDR = buf[i++];
                if (buf[i] == '\0') {
                    i = 0;
                }
            } else if (!PIN_READ(GPIOC, 13) && button_state) {
                button_state = 0;
            }
        }
    }
}
