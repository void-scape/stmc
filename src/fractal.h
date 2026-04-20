#ifndef FRACTAL_H
#define FRACTAL_H

#include "lcd.h"

#include <stdint.h>

#define PREC 13
#define FP(X) ((int16_t)((X) * (1 << PREC)))
#define RADIUS (4 << PREC)

// `a` * `b`
static inline int32_t fp_mul(int16_t a, int16_t b) {
    return ((int32_t)a * (int32_t)b) >> PREC;
}

// (`a` * `b`) << `shift`
static inline int32_t fp_mul_shift(int16_t a, int16_t b, int32_t shift) {
    return ((int32_t)a * (int32_t)b) >> (PREC - shift);
}

typedef struct {
    float re, im;
    uint32_t iterations;

    int16_t lut_re[LCD_WIDTH / 2];
    int16_t lut_im[LCD_HEIGHT];
} julia_t;

// NOTE: The julia set has rotational symmetry around the origin, meaning
// that only half of the fractal needs to be computed. Furthermore, coordinates
// can be pre computed for repeated, scaled renders.
void julia_view(julia_t* julia, float zoom, float xoffset, float yoffset) {
    float aspect = (float)LCD_WIDTH / LCD_HEIGHT;
    float w2 = (LCD_WIDTH - 1) * 0.5f;
    float h2 = (LCD_HEIGHT - 1) * 0.5f;
    for (int x = 0; x < LCD_WIDTH / 2; x++)
        julia->lut_re[x] = FP(((float)x - w2) / w2 * zoom * aspect);
    for (int y = 0; y < LCD_HEIGHT; y++)
        julia->lut_im[y] = FP(((float)y - h2) / h2 * zoom);
}

static inline void julia_render_scaled(julia_t* julia, uint16_t shift, uint32_t scale) {
    int32_t cre = FP(julia->re);
    int32_t cim = FP(julia->im);

    int32_t re_squared, im_squared;
    int16_t re, im;
    int32_t radius;
    uint32_t i, x, y;
    uint32_t px, py, ox, oy, mx, my;
    for (y = 0; y < LCD_HEIGHT; y += scale) {
        for (x = 0; x < LCD_WIDTH / 2; x += scale) {
            re = julia->lut_im[y];
            im = -julia->lut_re[x];
            i = 0;
            radius = 0;
            while ((i < julia->iterations - 1) && (radius < RADIUS)) {
                re_squared = fp_mul(re, re);
                im_squared = fp_mul(im, im);
                im = fp_mul_shift(re, im, 1) + cim;
                re = re_squared - im_squared + cre;
                radius = re_squared + im_squared;
                i++;
            }
            uint16_t p = i << shift;
            for (py = 0; py < scale; py++) {
                oy = y + py;
                for (px = 0; px < scale; px++) {
                    ox = x + px;
                    pixels[oy * LCD_WIDTH + ox] = p;
                    my = (LCD_HEIGHT - 1 - oy);
                    mx = (LCD_WIDTH - 1 - ox);
                    pixels[my * LCD_WIDTH + mx] = p;
                }
            }
        }
    }
}

void julia_render(julia_t* julia, uint16_t shift) {
    // perf_start();
    julia_render_scaled(julia, shift, 16);
    // perf_end("julia scale 16");
    // perf_start();
    julia_render_scaled(julia, shift, 8);
    // perf_end("julia scale 8");
    // perf_start();
    julia_render_scaled(julia, shift, 4);
    // perf_end("julia scale 4");
    // perf_start();
    julia_render_scaled(julia, shift, 2);
    // perf_end("julia scale 2");
    // perf_start();
    julia_render_scaled(julia, shift, 1);
    // perf_end("julia scale 1");
}

void julia_render_naive(julia_t* julia, uint16_t shift) {
    float zoom = 0.5;
    float aspect = (float)LCD_WIDTH / LCD_HEIGHT;
    int16_t lre_start = FP(-1.0 * zoom * aspect);
    int16_t lim = FP(-1.0 * zoom);
    int16_t dre = FP((2.0 * zoom * aspect) / (LCD_WIDTH - 1));
    int16_t dim = FP(2.0 * zoom / (LCD_HEIGHT - 1));

    int32_t tmp1, tmp2;
    int16_t re, im;
    int32_t radius;
    int16_t lre;
    uint16_t i, x, y;

    int32_t cre = FP(julia->re);
    int32_t cim = FP(julia->im);

    for (y = 0; y < LCD_HEIGHT; y++) {
        lre = lre_start;
        for (x = 0; x < LCD_WIDTH; x++) {
            im = -lre;
            re = lim;
            i = 0;
            radius = 0;
            while ((i < julia->iterations - 1) && (radius < RADIUS)) {
                tmp1 = ((int32_t)re * re) >> PREC;
                tmp2 = ((int32_t)im * im) >> PREC;

                im = (((int32_t)re * im) >> (PREC - 1)) + cim;
                re = tmp1 - tmp2 + cre;

                radius = (uint32_t)tmp1 + tmp2;

                i++;
            }
            uint16_t p = i << shift;
            pixels[y * LCD_WIDTH + x] = p;
            lre += dre;
        }
        lim += dim;
    }
}

#endif // FRACTAL_H
