#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

typedef uint8_t hue_t;
extern void mandelbrot(uint8_t iterations, hue_t* output_hues, __m512 real_block, __m512 imag_block);

#define BOUND_LEFT  -2.0
#define BOUND_RIGHT  1.0

#define BOUND_UP     1.0
#define BOUND_DOWN  -1.0

#define BOXES_ACROSS 15
#define BOXES_DOWN   10

#define BOX_WIDTH    4
#define BOX_HEIGHT   4
#define BOX_N_CELLS 16

inline bool bitset(const void *src, unsigned int bit) {
  bool o;
  asm volatile ("btsl %2, (%1)" :
      "=@ccc" (o)
      : "r" (src),
        "ir" (bit)
      : "cc" );
  return o;
}

void dfs(void* visited, unsigned col, unsigned row, __m512 real, __m512 imag, float block_gap_across, float block_gap_down, hue_t* out) {
  // block id, like so:
  //   0  1  2  3  4  5
  //   6  7  8  9 10 11
  //  12 13 14 15 16 17
  //  18 19 20 21 22 23
  unsigned block_id = row * BOXES_ACROSS + col;

  // we set it to visited, but if it was already visited with return
  bool was_visited = bitset(visited, block_id);
  if (was_visited) return;

  // do 255 iterations. the function writes to its out buffer how many iterations are left
  // when it surpasses the outer limit, so by starting with the maximum (255), we can just
  // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
  mandelbrot(255, out + (block_id * BOX_N_CELLS), real, imag);

  if (row + 1 < BOXES_DOWN) {
    /* recurse across */
    dfs(
      visited,
      col,
      row + 1,
      real,
      _mm512_add_ps(imag, _mm512_set1_ps(block_gap_down)), // this is clever enough to optimise into single broadcast instruction, if possible
      block_gap_across,
      block_gap_down,
      out
    );
  }

  if (col + 1 < BOXES_ACROSS) {
    /* recurse down */
    dfs(
      visited,
      col + 1,
      row,
      _mm512_add_ps(real, _mm512_set1_ps(block_gap_across)), // this is clever enough to optimise into single broadcast instruction, if possible
      imag,
      block_gap_across,
      block_gap_down,
      out
    );
  }
}


int main(int argc, char *argv[]) {
  // to be loaded into a zmm register, it needs to be 64-byte aligned
  float real_block[16] __attribute__ ((aligned(64)));
  float imag_block[16] __attribute__ ((aligned(64)));

  // ditto for loading out of an xmm register

  hue_t *out = aligned_alloc(16, BOXES_ACROSS * BOXES_DOWN * BOX_N_CELLS);
  void *visited = malloc(BOXES_DOWN * BOXES_ACROSS / 8); // 8 bits per byte

  const float real_sep = (BOUND_RIGHT - BOUND_LEFT) / BOXES_ACROSS / BOX_WIDTH;
  const float imag_sep = (BOUND_DOWN - BOUND_UP) / BOXES_DOWN / BOX_HEIGHT;

  // create 4x4 box, starting with BOUND_UP and BOUND_LEFT like so:
  // 0+0i 1+0i 2+0i 3+0i, where each difference across is real_sep
  // 0+1i 1+1i 2+1i 3+1i
  // 0+2i 1+2i 2+2i 3+2i
  // 0+3i 1+3i 2+3i 3+3i
  // where each difference down in imag_sep
  for (int re = 0; re < BOX_WIDTH; re++) {
    for (int im = 0; im < BOX_HEIGHT; im++) {
      real_block[re + im*BOX_HEIGHT] = BOUND_LEFT + (re * real_sep); // + real_sep for each re
      imag_block[re + im*BOX_HEIGHT] = BOUND_UP   + (im * imag_sep);
    }
  }

  // start from top left, and crawl downwards and rightwards
  dfs(
    visited,
    0,
    0,
    *((__m512 *) real_block),
    *((__m512 *) imag_block),
    real_sep * BOX_WIDTH,
    imag_sep * BOX_HEIGHT,
    out
  );

  for (unsigned int y = 0; y < (BOXES_DOWN * BOX_HEIGHT); ++y) {
    putchar('|');
    for (unsigned int x = 0; x < (BOXES_ACROSS * BOX_WIDTH); ++x) {
      unsigned block_id = (x / BOX_WIDTH) + (BOXES_ACROSS * (y / BOX_HEIGHT));
      unsigned block_cell = (x % BOX_WIDTH) + (BOX_WIDTH * (y % BOX_HEIGHT));

      hue_t colour = out[block_cell + (block_id * BOX_N_CELLS)];

      if (colour) {
        printf("\033[48;2;6;%u;6m  ", -((signed) colour) & 0xff);
      } else {
        printf("\033[0m  ");
      }

    }
    puts("\033[0m|");
  }

  free(out);
  free(visited);
  return 0;
}
