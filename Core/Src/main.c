#include "hal.h"
#include "stm32l552xx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

#define CS 9
#define CS_P GPIOF

#define DC 0
#define DC_P GPIOG

#define RST 1
#define RST_P GPIOG

#define BL 7
#define BL_P GPIOF

#define SDI 15
#define SCK 13

void spi_byte(uint8_t byte) {
    while (!BIT_READ(SPI1->SR, SPI_SR_TXE_Pos)) {}
    *(volatile uint8_t*)&SPI1->DR = byte;
    while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
    (void)SPI1->DR;
}

void spi1_enable(void) {
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
}

void lcd_write_cmd(uint32_t cmd) {
    PIN_LOW(CS_P, CS);
    PIN_LOW(DC_P, DC);
    spi_byte(cmd);
}

void lcd_write_data(uint32_t data) {
    PIN_LOW(CS_P, CS);
    PIN_HIGH(DC_P, DC);
    spi_byte(data);
    PIN_HIGH(CS_P, CS);
}

void lcd_reset(void) {
    PIN_HIGH(CS_P, CS);
    delay_ms(100);
    PIN_LOW(RST_P, RST);
    delay_ms(100);
    PIN_HIGH(RST_P, RST);
    delay_ms(100);
}

void lcd_init(void) {
    lcd_reset();

    lcd_write_cmd(0x36);
    lcd_write_data(0x00);

    lcd_write_cmd(0x3A);
    lcd_write_data(0x05);

    lcd_write_cmd(0x21);

    lcd_write_cmd(0x2A);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0x3F);

    lcd_write_cmd(0x2B);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0xEF);

    lcd_write_cmd(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_cmd(0xB7);
    lcd_write_data(0x35);

    lcd_write_cmd(0xBB);
    lcd_write_data(0x1F);

    lcd_write_cmd(0xC0);
    lcd_write_data(0x2C);

    lcd_write_cmd(0xC2);
    lcd_write_data(0x01);

    lcd_write_cmd(0xC3);
    lcd_write_data(0x12);

    lcd_write_cmd(0xC4);
    lcd_write_data(0x20);

    lcd_write_cmd(0xC6);
    lcd_write_data(0x0F);

    lcd_write_cmd(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x11);
    lcd_write_data(0x08);
    lcd_write_data(0x0C);
    lcd_write_data(0x15);
    lcd_write_data(0x39);
    lcd_write_data(0x33);
    lcd_write_data(0x50);
    lcd_write_data(0x36);
    lcd_write_data(0x13);
    lcd_write_data(0x14);
    lcd_write_data(0x29);
    lcd_write_data(0x2D);

    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x10);
    lcd_write_data(0x08);
    lcd_write_data(0x06);
    lcd_write_data(0x06);
    lcd_write_data(0x39);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x0B);
    lcd_write_data(0x16);
    lcd_write_data(0x14);
    lcd_write_data(0x2F);
    lcd_write_data(0x31);
    lcd_write_cmd(0x21);

    lcd_write_cmd(0x11);

    lcd_write_cmd(0x29);
}

void lcd_set_cursor(uint32_t xstart, uint32_t ystart, uint32_t xend, uint32_t yend) {
    lcd_write_cmd(0x2a);
    lcd_write_data(xstart >> 8);
    lcd_write_data(xstart & 0xff);
    lcd_write_data((xend - 1) >> 8);
    lcd_write_data((xend - 1) & 0xff);

    lcd_write_cmd(0x2b);
    lcd_write_data(ystart >> 8);
    lcd_write_data(ystart & 0xff);
    lcd_write_data((yend - 1) >> 8);
    lcd_write_data((yend - 1) & 0xff);

    lcd_write_cmd(0x2C);
}

void lcd_clear(void) {
    lcd_set_cursor(0, 0, LCD_WIDTH, LCD_HEIGHT);
    PIN_HIGH(DC_P, DC);
    uint32_t color = 0xF800;
    for (int i = 0; i < LCD_HEIGHT; i++) {
        for (int j = 0; j < LCD_WIDTH; j++) {
            spi_byte(color >> 8);
            spi_byte((color & 0xff) << 8);
        }
    }
}

static uint16_t pixels[LCD_WIDTH * LCD_HEIGHT];

void lcd_render(void) {
    lcd_set_cursor(0, 0, LCD_WIDTH, LCD_HEIGHT);
    PIN_HIGH(DC_P, DC);
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            uint16_t color = pixels[y * LCD_WIDTH + x];
            spi_byte(color >> 8);
            spi_byte(color & 0xff);
        }
    }
}

bool should_skip(float x, float y) {
    float q = (x - 0.25) * (x - 0.25) + y * y;
    return (q * (q + (x - 0.25)) < y * y / 4) || ((x + 1.0) * (x + 1.0) + y * y < 0.0625);
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    // lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);

    gpio_configure_push_pull_low_speed(DC_P, DC, GPIO_MODE_OUTPUT);
    gpio_configure_push_pull_low_speed(RST_P, RST, GPIO_MODE_OUTPUT);
    gpio_configure_push_pull_low_speed(BL_P, BL, GPIO_MODE_OUTPUT);
    gpio_configure_pin(CS_P, CS, GPIO_MODE_OUTPUT, GPIO_OUTPUT_TYPE_PUSH_PULL, GPIO_SPEED_VERY_HIGH,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);

    // configure the pins to the correct alternate functions
    uint32_t pins[] = {SDI, SCK};
    for (int i = 0; i < 2; i++) {
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

    spi1_enable();
    lcd_init();
    lcd_clear();

    for (int py = 0; py < LCD_HEIGHT; py++) {
        for (int px = 0; px < LCD_WIDTH; px++) {
            float x0 = (float)px / LCD_WIDTH * 2 - 1;
            x0 *= (float)LCD_WIDTH / LCD_HEIGHT;
            float y0 = (float)py / LCD_HEIGHT * 2 - 1;

            float rx = y0;
            float ry = -x0;
            x0 = rx;
            y0 = ry;

            float cx = -0.5;
            float cy = 0.0;

            float zoom = 1.5;
            x0 = x0 * zoom + cx;
            y0 = y0 * zoom + cy;

            int iteration = 0;
            int iterations = 100;

            if (should_skip(x0, y0)) {
                iteration = 0;
            } else {
                float x = 0.0;
                float y = 0.0;
                while (iteration < iterations) {
                    float x2 = x * x;
                    float y2 = y * y;
                    if (x2 + y2 > 4.0) {
                        break;
                    }
                    y = 2.0 * x * y + y0;
                    x = x2 - y2 + x0;
                    iteration++;
                }
                if (iteration == iterations) {
                    iteration = 0;
                }
            }

            uint16_t l = (uint16_t)((float)iteration / iterations * 255.0);
            uint16_t r = l;
            uint16_t g = l;
            uint16_t b = l;
            uint16_t color = ((r & 0x3f) << 11) | ((g & 0x1f) << 5) | (b & 0x3f);
            pixels[py * LCD_WIDTH + px] = color;
        }
    }

    lcd_render();

    for (;;) {}
}
