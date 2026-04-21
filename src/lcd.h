#ifndef LCD_H
#define LCD_H

#include "hal.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

uint16_t pixels[LCD_WIDTH * LCD_HEIGHT];

#define CS 1
#define CS_P GPIOD

#define DC 9
#define DC_P GPIOF

#define RST 0
#define RST_P GPIOG

#define BL 7
#define BL_P GPIOF

#define SDI 15
#define SCK 13

static inline void spi_byte(uint8_t byte) {
    while (!BIT_READ(SPI1->SR, SPI_SR_TXE_Pos)) {}
    *(volatile uint8_t *)&SPI1->DR = byte;
    while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
    (void)SPI1->DR;
}

static void spi1_lcd(void) {
    BIT_CLEAR(SPI1->CR1, SPI_CR1_SPE_Pos);
    // 8-bit data size
    SPI1->CR2 = (SPI1->CR2 & ~SPI_CR2_DS_Msk) | (0b0111 << SPI_CR2_DS_Pos);
    // RXNE event is generated if the FIFO level is greater than or equal to 1/4
    // (8-bit) NOTE: Since we only want to send bytes, the RXNE must trigger on
    // 8-bit data.
    BIT_SET(SPI1->CR2, SPI_CR2_FRXTH_Pos);
    BIT_CLEAR(SPI1->CR2, SPI_CR2_TXDMAEN_Pos);
    BIT_SET(SPI1->CR1, SPI_CR1_SPE_Pos);
}

static void spi1_dma(void) {
    PIN_LOW(CS_P, CS);
    PIN_HIGH(DC_P, DC);
    BIT_CLEAR(SPI1->CR1, SPI_CR1_SPE_Pos);
    // 16-bit data size
    SPI1->CR2 = (SPI1->CR2 & ~SPI_CR2_DS_Msk) | (0b1111 << SPI_CR2_DS_Pos);
    BIT_CLEAR(SPI1->CR2, SPI_CR2_FRXTH_Pos);
    BIT_SET(SPI1->CR2, SPI_CR2_TXDMAEN_Pos);
    BIT_SET(SPI1->CR1, SPI_CR1_SPE_Pos);
    BIT_SET(DMA1_Channel1->CCR, DMA_CCR_EN_Pos);
}

static void spi1_lcd_enable(void) {
    // configure the pins
    gpio_configure_push_pull_low_speed(DC_P, DC, GPIO_MODE_OUTPUT);
    gpio_configure_push_pull_low_speed(RST_P, RST, GPIO_MODE_OUTPUT);
    gpio_configure_push_pull_low_speed(BL_P, BL, GPIO_MODE_OUTPUT);
    gpio_configure_pin(CS_P, CS, GPIO_MODE_OUTPUT, GPIO_OUTPUT_TYPE_PUSH_PULL,
                       GPIO_SPEED_VERY_HIGH, GPIO_PULLUPDOWN_NO_PULLUP_DOWN);

    // configure the pins to the correct alternate functions
    uint32_t pins[] = {SDI, SCK};
    for (int i = 0; i < 2; i++) {
        uint32_t pin = pins[i];
        gpio_configure_pin(GPIOE, pin, GPIO_MODE_ALTERNATE,
                           GPIO_OUTPUT_TYPE_PUSH_PULL, GPIO_SPEED_VERY_HIGH,
                           GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
        // AF5 = 0b0101
        uint32_t afr_idx = pin / 8;
        uint32_t bit_pos = (pin % 8) * 4;
        BIT_SET(GPIOE->AFR[afr_idx], bit_pos);
        BIT_CLEAR(GPIOE->AFR[afr_idx], bit_pos + 1);
        BIT_SET(GPIOE->AFR[afr_idx], bit_pos + 2);
        BIT_CLEAR(GPIOE->AFR[afr_idx], bit_pos + 3);
    }

    // enable SPI1 clock
    BIT_SET(RCC->APB2ENR, RCC_APB2ENR_SPI1EN_Pos);
    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    // master mode
    BIT_SET(SPI1->CR1, SPI_CR1_MSTR_Pos);
    // software slave management
    BIT_SET(SPI1->CR1, SPI_CR1_SSM_Pos);
    // internal slave select high
    BIT_SET(SPI1->CR1, SPI_CR1_SSI_Pos);

    // turn on lcd
    PIN_HIGH(BL_P, BL);

    spi1_lcd();
}

static inline void lcd_write_cmd(uint32_t cmd) {
    PIN_LOW(CS_P, CS);
    PIN_LOW(DC_P, DC);
    spi_byte(cmd);
    PIN_HIGH(CS_P, CS);
}

static inline void lcd_write_data(uint32_t data) {
    PIN_LOW(CS_P, CS);
    PIN_HIGH(DC_P, DC);
    spi_byte(data);
    PIN_HIGH(CS_P, CS);
}

static void lcd_reset(void) {
    PIN_HIGH(CS_P, CS);
    delay_ms(100);
    PIN_LOW(RST_P, RST);
    delay_ms(100);
    PIN_HIGH(RST_P, RST);
    delay_ms(100);
}

static void lcd_init_sequence(void) {
    lcd_write_cmd(0x36);
    lcd_write_data(0x00);

    lcd_write_cmd(0x3A);
    lcd_write_data(0x55);
    delay_ms(10);

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

static void lcd_init(void) {
    spi1_lcd();
    lcd_reset();
    lcd_init_sequence();
}

// sets the cursor to the top left and defines the size of the draw window
static void lcd_prepare_for_render(void) {
    spi1_lcd();

    lcd_write_cmd(0x2a);
    lcd_write_data(0);
    lcd_write_data(0);
    lcd_write_data((LCD_WIDTH - 1) >> 8);
    lcd_write_data((LCD_WIDTH - 1) & 0xff);

    lcd_write_cmd(0x2b);
    lcd_write_data(0);
    lcd_write_data(0);
    lcd_write_data((LCD_HEIGHT - 1) >> 8);
    lcd_write_data((LCD_HEIGHT - 1) & 0xff);

    lcd_write_cmd(0x2C);
}

static void dma_lcd_enable(void) {
    // enable clock
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUX1EN;

    DMA1_Channel1->CCR = 0;
    // read from memory
    BIT_SET(DMA1_Channel1->CCR, DMA_CCR_DIR_Pos);
    // increment memory position after read
    BIT_SET(DMA1_Channel1->CCR, DMA_CCR_MINC_Pos);

    // read from the pixel buffer
    DMA1_Channel1->CM0AR = (uint32_t)&pixels;
    // write to the SPI1 data register
    DMA1_Channel1->CPAR = (uint32_t)&SPI1->DR;
    // read 2 bytes from memory
    DMA1_Channel1->CCR |= 1 << DMA_CCR_MSIZE_Pos;
    // write 2 bytes to SPI1 data register
    DMA1_Channel1->CCR |= 1 << DMA_CCR_PSIZE_Pos;
    // define the number of elements in the buffers
    DMA1_Channel1->CNDTR = sizeof(pixels) / sizeof(*pixels);

    // SPI1_TX triggers dma1 ch1
    DMAMUX1_Channel0->CCR = 12;

    irq_enable(DMA1_Channel1_IRQn, 0);
    BIT_SET(DMA1_Channel1->CCR, DMA_CCR_TCIE_Pos);
    BIT_SET(DMA1_Channel1->CCR, DMA_CCR_TEIE_Pos);
}

void DMA1_Channel1_IRQHandler(void) {
    if (BIT_READ(DMA1->ISR, DMA_ISR_TEIF1_Pos)) {
        BIT_SET(DMA1->IFCR, DMA_IFCR_CTEIF1_Pos);
        PANIC("DMA transfer error");
    }
    if (BIT_READ(DMA1->ISR, DMA_ISR_TCIF1_Pos)) {
        BIT_SET(DMA1->IFCR, DMA_IFCR_CTCIF1_Pos);
        while (!BIT_READ(SPI1->SR, SPI_SR_TXE_Pos)) {}
        while (BIT_READ(SPI1->SR, SPI_SR_BSY_Pos)) {}
        BIT_CLEAR(DMA1_Channel1->CCR, DMA_CCR_EN_Pos);
        DMA1_Channel1->CNDTR = LCD_WIDTH * LCD_HEIGHT;
        lcd_prepare_for_render();
        spi1_dma();
    }
}

void lcd_hal_init(void) {
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        pixels[i] = 0;
    }
    spi1_lcd_enable();
    lcd_init();
    dma_lcd_enable();
    lcd_prepare_for_render();
    spi1_dma();
}

#endif // LCD_H
