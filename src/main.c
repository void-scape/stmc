#include "fractal.h"
#include "gyro.h"
#include "hal.h"
#include "lcd.h"

int main(void) {
    clocks_enable();
    lpuart_enable(LPUART_INTERRUPT_DISABLE, 115200);
    perf_configure();

    lcd_hal_init();
    gyro_hal_init();

    julia_t julia = {0};

    julia.re = -0.4;
    julia.im = 0.6;

    // julia.re = 0.285;
    // julia.im = 0.01;

    // julia.re = -0.70176;
    // julia.im = -0.3842;

    // julia.re = -0.8;
    // julia.im = 0.156;
    julia.iterations = 128;
    julia_view(&julia, 1.0);
    for (;;) {
        julia_render(&julia);
    }
}
