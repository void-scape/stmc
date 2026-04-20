#include "fractal.h"
#include "gyro.h"
#include "hal.h"
#include "lcd.h"

#include <stdbool.h>

void curtains_render(uint32_t shift) {
    uint16_t r = 0;
    uint16_t* p = pixels;
    uint32_t x, y;
    for (y = 0; y < LCD_HEIGHT; y++) {
        r = 0;
        for (x = 0; x < LCD_WIDTH; x++) {
            r += 1;
            *p++ = r << shift;
        }
        if (y % (LCD_HEIGHT / 10) == 0) {
            delay_ms(15);
        }
    }
}

#define BLUE 0
#define GREEN 5
#define RED 11

int main(void) {
    clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);
    perf_configure();

    lcd_hal_init();
    // gyro_hal_init();

    curtains_render(RED);
    delay_ms(500);

    julia_t julia = {0};
    julia.iterations = 128;

    julia.re = 0.285;
    julia.im = 0.01;
    julia_view(&julia, 0.5, 0, 0);
    julia_render(&julia, BLUE);
    delay_ms(1000);

    julia.re = 0.21;
    julia.im = 0.565;
    julia_view(&julia, 0.5, 0, 0);
    julia_render(&julia, RED);
    delay_ms(1000);

    julia.re = -1.248;
    julia.im = 0.12;
    julia_view(&julia, 0.5, 0, 0);
    julia_render(&julia, GREEN);
    delay_ms(1000);

    curtains_render(RED);

    for (;;) {}
}
