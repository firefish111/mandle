#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>
#include <string.h>

typedef uint8_t hue_t;
extern void mandelbrot(uint8_t iterations, hue_t* output_hues, __m512 real_block, __m512 imag_block);

#define BOUND_LEFT  -2.0
#define BOUND_RIGHT  1.0

#define BOUND_UP     1.0
#define BOUND_DOWN  -1.0

#define SECTION_WIDTH  5
#define SECTION_HEIGHT 5

#define BLOCKS_ACROSS (SECTION_WIDTH  * 3)
#define BLOCKS_DOWN   (SECTION_HEIGHT * 2)

#define BLOCK_WIDTH    4
#define BLOCK_HEIGHT   4
#define BLOCK_N_CELLS 16

#define CELL_STR "  "
#define CELL_N_CHARS strlen(CELL_STR)

// wrapper around bts instruction. inline so it gets optimised away.
inline bool bit_test_and_set_high(const void *src, unsigned int bit) {
  bool o;
  asm volatile ("btsl %2, (%1)" :
      "=@ccc" (o)
      : "r" (src),
        "ir" (bit)
      : "cc" );
  return o;
}

struct searcher_t {
  void  * visited;
  hue_t * out;

  float block_gap_across;
  float block_gap_down;
};

void dfs(const struct searcher_t * search, unsigned block_x, unsigned block_y, __m512 real, __m512 imag) {
  // block_x and block_y are which block we are pointing to. we convert this to a linear
  // block id, where blocks are numbered like so:
  //   0  1  2  3  4  5
  //   6  7  8  9 10 11
  //  12 13 14 15 16 17
  //  18 19 20 21 22 23
  unsigned block_id = block_x + (block_y * BLOCKS_ACROSS);

  // we set it to visited, but if it was already visited with return
  bool was_visited = bit_test_and_set_high(search->visited, block_id);
  if (was_visited) return;

  // do 255 iterations. the function writes to its out buffer how many iterations are left
  // when it surpasses the outer limit, so by starting with the maximum (255), we can just
  // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
  mandelbrot(255, search->out + (block_id * BLOCK_N_CELLS), real, imag);


  if (block_x + 1 < BLOCKS_ACROSS) {
    /* recurse across in real direction */
    dfs(
      search,
      block_x + 1,
      block_y,
      _mm512_add_ps(real, _mm512_set1_ps(search->block_gap_across)), // this is clever enough to optimise into single broadcast instruction, if possible
      imag
    );
  }

  if (block_y + 1 < BLOCKS_DOWN) {
    /* recurse down in imaginary direction */
    dfs(
      search,
      block_x,
      block_y + 1,
      real,
      _mm512_add_ps(imag, _mm512_set1_ps(search->block_gap_down)) // ditto but for imaginary
    );
  }
}


int main(int argc, char *argv[]) {

  // to be loaded into a zmm register, it needs to be 64-byte aligned
  // instead of setting them to tyoe __m512 right away, we want to set it to some values,
  // which we can't do as just a __m512 type, so we do the conversion with a type pun at the dfs call
  float real_block[16] __attribute__ ((aligned(64)));
  float imag_block[16] __attribute__ ((aligned(64)));

  // ditto for loading out of an xmm register

  const float real_sep = (BOUND_RIGHT - BOUND_LEFT) / BLOCKS_ACROSS / BLOCK_WIDTH;
  const float imag_sep = (BOUND_DOWN - BOUND_UP) / BLOCKS_DOWN / BLOCK_HEIGHT;

  // create 4x4 block, starting with BOUND_UP and BOUND_LEFT like so:
  // 0+0i 1+0i 2+0i 3+0i, where each difference across is real_sep
  // 0+1i 1+1i 2+1i 3+1i
  // 0+2i 1+2i 2+2i 3+2i
  // 0+3i 1+3i 2+3i 3+3i
  // where each difference down is imag_sep
  for (int re = 0; re < BLOCK_WIDTH; re++) {
    for (int im = 0; im < BLOCK_HEIGHT; im++) {
      real_block[re + im*BLOCK_HEIGHT] = BOUND_LEFT + (re * real_sep); // + real_sep for each re
      imag_block[re + im*BLOCK_HEIGHT] = BOUND_UP   + (im * imag_sep);
    }
  }

  struct searcher_t search = {
    .visited = malloc(BLOCKS_DOWN * BLOCKS_ACROSS / 8), // 8 bits per byte
    .out     = aligned_alloc(16, BLOCKS_ACROSS * BLOCKS_DOWN * BLOCK_N_CELLS), // has to be aligned at 16 bytes for an xmm register

    .block_gap_across = real_sep * BLOCK_WIDTH,
    .block_gap_down   = imag_sep * BLOCK_HEIGHT
  };

  // start from top left, and crawl downwards and rightwards
  dfs(
    &search,
    0,
    0,
    *((__m512 *) real_block),
    *((__m512 *) imag_block)
  );

  printf("\033[90m| ");
  // print bounds
  int width; // do not initialise: that will happen in the printf %n
  printf("%f%n", BOUND_LEFT, &width);
  width = (CELL_N_CHARS * BLOCK_WIDTH * BLOCKS_ACROSS) - width - 2; // subtract from how wise our render is in the first place. - 2 for padding
  printf("%*f |\n", width, BOUND_RIGHT);

  for (unsigned int y = 0; y < (BLOCKS_DOWN * BLOCK_HEIGHT); ++y) {
    putchar('|');
    for (unsigned int x = 0; x < (BLOCKS_ACROSS * BLOCK_WIDTH); ++x) {
      unsigned block_id = (x / BLOCK_WIDTH) + (BLOCKS_ACROSS * (y / BLOCK_HEIGHT));
      unsigned block_cell = (x % BLOCK_WIDTH) + (BLOCK_WIDTH * (y % BLOCK_HEIGHT));

      hue_t colour = search.out[block_cell + (block_id * BLOCK_N_CELLS)];

      if (colour) {
        printf("\033[48;2;6;%u;6m" CELL_STR, -((signed) colour) & 0xff);
      } else {
        printf("\033[40m" CELL_STR);
      }
    }
    printf("\033[40m|");
    if (y == 0) { // first one
      printf(" %f", BOUND_UP);
    } else if (y + 1 == BLOCKS_DOWN * BLOCK_HEIGHT) { // last one
      printf(" %f", BOUND_DOWN);
    }
    putchar('\n');
  }

  free(search.visited);
  free(search.out);

  return 0;
}
