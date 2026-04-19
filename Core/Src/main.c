#include "hal.h"
#include "lcd.c"

#include <stdbool.h>

#define PREC 16
#define FP(X) ((int32_t)((X) * (1 << PREC)))

void curtains(uint32_t delay) {
    uint16_t r = 0;
    uint16_t* p = pixels;

    static bool dir = false;
    uint16_t shift;
    if (dir)
        shift = 11;
    else
        shift = 5;
    dir = !dir;

    uint32_t x, y;
    for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
            *p++ = r << shift;
        }
        r += 1;
        if (y % (LCD_HEIGHT / 10) == 0) {
            delay_ms(delay);
        }
    }
}

void draw_curtains(void) {
    for (int i = 0; i < 100; i++) {
        if (i < 1) {
            curtains(100);
        } else {
            curtains(0);
        }
    }
}

void julia(uint32_t cre, uint32_t cim, uint32_t iteration, uint16_t shift) {
    float zoom = 1.5;
    float aspect = (float)LCD_WIDTH / LCD_HEIGHT;
    int32_t lre_start = FP(-1.0 * zoom * aspect);
    int32_t lim = FP(-1.0 * zoom);
    int32_t dre = FP((4.0 * zoom * aspect) / (LCD_WIDTH - 1));
    int32_t dim = FP(4.0 * zoom / (LCD_HEIGHT - 1));

    int32_t tmp1, tmp2;
    int32_t re, im;
    int32_t radius;
    int32_t lre;
    uint32_t i, x, y;

    for (y = 0; y < LCD_HEIGHT; y += 2) {
        lre = lre_start;
        for (x = 0; x < LCD_WIDTH; x += 2) {
            im = -lre;
            re = lim;
            i = 0;
            radius = 0;
            while ((i < iteration - 1) && (radius < 4)) {
                tmp1 = ((int64_t)re * re) >> PREC;
                tmp2 = ((int64_t)im * im) >> PREC;

                im = (((int64_t)re * im) >> (PREC - 1)) + cim;
                re = tmp1 - tmp2 + cre;

                radius = (tmp1 + tmp2) >> PREC;

                i++;
            }
            uint16_t p = i << shift;
            pixels[y * LCD_WIDTH + x] = p;
            pixels[y * LCD_WIDTH + x + 1] = p;
            pixels[(y + 1) * LCD_WIDTH + x] = p;
            pixels[(y + 1) * LCD_WIDTH + x + 1] = p;
            lre += dre;
        }
        lim += dim;
    }
}

int main(void) {
    clocks_enable();
    gpio_clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);

    lcd_hal_init();

    draw_curtains();
    julia(FP(0.285f), FP(0.01f), 64, 0);
    delay_ms(1000);

    draw_curtains();
    julia(FP(0.21f), FP(0.565f), 64, 11);
    delay_ms(1000);

    draw_curtains();
    julia(FP(-1.248f), FP(0.120f), 64, 5);
    delay_ms(1000);

    draw_curtains();
    for (;;) {}
}
