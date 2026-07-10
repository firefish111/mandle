#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <immintrin.h>

typedef uint8_t hue_t;
extern void mandelbrot(uint8_t iterations, const float *real_block, const float *imag_block, hue_t* output_hues);

#define BOUND_LEFT  -2.0
#define BOUND_RIGHT  1.0

#define BOUND_UP    -1.0
#define BOUND_DOWN   1.0


int main(void) {
  // to be loaded into a zmm register, it needs to be 64-byte aligned
  float real_block[16] __attribute__ ((aligned(64)));
  float imag_block[16] __attribute__ ((aligned(64)));
  // ditto for loading out of an xmm register
  hue_t out[256] __attribute__ ((aligned(16)));

  // fill real components with ascending numbers. this is so we render one row at a time
  int x = 0;
  const float real_sep = (BOUND_RIGHT - BOUND_LEFT) / 15;
  float real_v = BOUND_LEFT;
  while (x < 16) {
    real_block[x] = real_v;

    x++;
    real_v += real_sep;
  }

  int y = 0;
  const float imag_sep = (BOUND_DOWN - BOUND_UP) / 15;
  float imag_v = BOUND_UP;
  while (y < 16) {
    // fill imaginary components with the same number, i.e. the current row
    for (int i = 0; i < 16; ++i) {
      imag_block[i] = imag_v;
    }

    // do 255 iterations. the function writes to its out buffer how many iterations are left
    // when it surpasses the outer limit, so by starting with the maximum (255), we can just
    // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
    mandelbrot(0xff, real_block, imag_block, out + (y * 16));

    putchar('|');
    for (int i = 0; i < 16; ++i) {
      printf("\033[48;2;6;%u;6m   ", -((signed) out[i + (y * 16)]) & 0xff);
    }
    puts("|\033[0m");

    y++;
    imag_v += imag_sep;
  }


  return 0;
}
