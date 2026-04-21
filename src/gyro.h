#ifndef GYRO_H
#define GYRO_H

#include "hal.h"

#include <stdlib.h>

#define GYRO_ADDR 0b1101000

static void i2c1_configure(void) {
    BIT_SET(RCC->APB1ENR1, RCC_APB1ENR1_I2C1EN_Pos);

    gpio_configure_pin(GPIOB, 8, GPIO_MODE_ALTERNATE,
                       GPIO_OUTPUT_TYPE_OPEN_DRAIN, GPIO_SPEED_HIGH,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
    gpio_configure_pin(GPIOB, 9, GPIO_MODE_ALTERNATE,
                       GPIO_OUTPUT_TYPE_OPEN_DRAIN, GPIO_SPEED_HIGH,
                       GPIO_PULLUPDOWN_NO_PULLUP_DOWN);
    // PB8 AF4
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xF << 0)) | (4 << 0);
    // PB9 AF4
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xF << 4)) | (4 << 4);

    BIT_SET(RCC->APB1RSTR1, RCC_APB1RSTR1_I2C1RST_Pos);
    BIT_CLEAR(RCC->APB1RSTR1, RCC_APB1RSTR1_I2C1RST_Pos);
    I2C1->TIMINGR = (3 << I2C_TIMINGR_PRESC_Pos) |
                    (4 << I2C_TIMINGR_SCLDEL_Pos) |
                    (2 << I2C_TIMINGR_SDADEL_Pos) |
                    (15 << I2C_TIMINGR_SCLH_Pos) | (19 << I2C_TIMINGR_SCLL_Pos);
    BIT_SET(I2C1->CR1, I2C_CR1_PE_Pos);
}

static void i2c1_write(uint8_t slv_addr, uint8_t *data, uint8_t data_length) {
    while (BIT_READ(I2C1->ISR, I2C_ISR_BUSY_Pos)) {}
    I2C1->CR2 = 0;
    I2C1->CR2 |= (slv_addr << 1);
    BIT_CLEAR(I2C1->CR2, I2C_CR2_RD_WRN_Pos);
    I2C1->CR2 |= (data_length << I2C_CR2_NBYTES_Pos);
    BIT_SET(I2C1->CR2, I2C_CR2_AUTOEND_Pos);
    BIT_SET(I2C1->CR2, I2C_CR2_START_Pos);
    for (uint8_t i = 0; i < data_length; i++) {
        while (!BIT_READ(I2C1->ISR, I2C_ISR_TXE_Pos)) {}
        I2C1->TXDR = data[i];
    }
    while (!BIT_READ(I2C1->ISR, I2C_ISR_TXE_Pos)) {}
}

static void i2c1_read(uint8_t slv_addr, uint8_t reg_addr, uint8_t *buf,
                      uint8_t data_length) {
    uint8_t reg[1] = {reg_addr};
    i2c1_write(slv_addr, reg, 1);
    while (BIT_READ(I2C1->ISR, I2C_ISR_RXNE_Pos)) {
        (void)I2C1->RXDR;
    }
    I2C1->CR2 = 0;
    I2C1->CR2 |= (slv_addr << 1);
    BIT_SET(I2C1->CR2, I2C_CR2_RD_WRN_Pos);
    I2C1->CR2 |= (data_length << I2C_CR2_NBYTES_Pos);
    BIT_SET(I2C1->CR2, I2C_CR2_AUTOEND_Pos);
    BIT_SET(I2C1->CR2, I2C_CR2_START_Pos);
    for (uint8_t i = 0; i < data_length; i++) {
        while (!BIT_READ(I2C1->ISR, I2C_ISR_RXNE_Pos)) {}
        buf[i] = (uint8_t)I2C1->RXDR;
    }
}

static void gyro_reset(void) {
    uint8_t cmd[2];
    cmd[1] = 0;
    for (int i = 0x0D; i < 0x75; i++) {
        cmd[0] = i;
        i2c1_write(GYRO_ADDR, cmd, 2);
    }

    uint8_t buf[1];
    i2c1_read(GYRO_ADDR, 0x75, buf, 1);
    ASSERT(buf[0] == 0x68, "unknown gyro %02X ", buf[0]);
}

static volatile int32_t __gyro_velocity;
int32_t gyro_velocity(void) { return __gyro_velocity; }

void EXTI1_IRQHandler(void) {
    // clear interrupt
    EXTI->RPR1 |= 1 << 1;

    uint8_t av_bytes[6];
    i2c1_read(GYRO_ADDR, 0x43, av_bytes, 6);
    int16_t xav = ((uint16_t)av_bytes[0] << 8) | (uint16_t)av_bytes[1];
    int16_t yav = ((uint16_t)av_bytes[2] << 8) | (uint16_t)av_bytes[3];
    int16_t zav = ((uint16_t)av_bytes[4] << 8) | (uint16_t)av_bytes[5];

    int32_t xymax = MAX(abs(xav), abs(yav));
    int32_t max = MAX(xymax, abs(zav));
    __gyro_velocity = max;
}

void gyro_hal_init(void) {
    i2c1_configure();
    gyro_reset();

    // sample once every 8ms
    uint8_t cmd[2] = {0x19, 0x3f};
    i2c1_write(GYRO_ADDR, cmd, 2);

    // enable gyro interrupt when all the sensor registers have been written
    cmd[0] = 0x38;
    cmd[1] = 0x01;
    i2c1_write(GYRO_ADDR, cmd, 2);

    // PG1
    gpio_configure_push_pull_low_speed(GPIOG, 1, GPIO_MODE_INPUT);
    EXTI->EXTICR[0] = 0x6 << 8;
    // rising edge
    EXTI->RTSR1 |= 1 << 1;
    // enable interrupt
    EXTI->IMR1 |= 1 << 1;
    irq_enable(EXTI1_IRQn, 0);
}

#endif // GYRO_H
