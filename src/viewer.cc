#include <cstdio>
#include <cstdlib>

#include "include/viewer.hh"

// walk dfs helper function. initially called in other overload
void Viewer::walk(unsigned block_x, unsigned block_y, __m512 real, __m512 imag) const {
  // block_x and block_y are which block we are pointing to. we convert this to a linear
  // block id, where blocks are numbered like so:
  //   0  1  2  3  4  5
  //   6  7  8  9 10 11
  //  12 13 14 15 16 17
  //  18 19 20 21 22 23
  unsigned block_id = block_x + (block_y * this->bounds.n_blocks_x);

  // we set it to visited, but if it was already visited, we return
  bool was_visited = bit_test_and_set_high(this->visited, block_id);
  if (was_visited) return;

  // do 255 iterations. the function writes to its out buffer how many iterations are left
  // when it surpasses the outer limit, so by starting with the maximum (255), we can just
  // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
  this->huebuf.xmmtab[block_id] = this->compute(255, real, imag);

  if (block_x + 1 < this->bounds.n_blocks_x) {
    /* recurse across in real direction */
    this->walk(
      block_x + 1,
      block_y,
      _mm512_add_ps(real, _mm512_set1_ps(this->bounds.real_sep() * BLOCK_WIDTH)), // this is clever enough to optimise into single broadcast instruction, if possible
      imag
    );
  }

  if (block_y + 1 < this->bounds.n_blocks_y) {
    /* recurse down in imaginary direction */
    this->walk(
      block_x,
      block_y + 1,
      real,
      _mm512_add_ps(imag, _mm512_set1_ps(this->bounds.imag_sep() * BLOCK_HEIGHT)) // ditto but for imaginary
    );
  }
}

// publicly available, begin walking.
// this generates the starting conditions for the privvate helper function
void Viewer::walk() const {
  // to be loaded into a zmm register, it needs to be 64-byte aligned
  // instead of setting them to tyoe __m512 right away, we want to set it to some values,
  // which we can't do as just a __m512 type, so we do the conversion with a type pun at the dfs call
  float real_block[16] __attribute__ ((aligned(64)));
  float imag_block[16] __attribute__ ((aligned(64)));

  // create 4x4 block, starting with this->bounds.top and this->bounds.left like so:
  // 0+0i 1+0i 2+0i 3+0i, where each difference across is real_sep
  // 0+1i 1+1i 2+1i 3+1i
  // 0+2i 1+2i 2+2i 3+2i
  // 0+3i 1+3i 2+3i 3+3i
  // where each difference down is imag_sep
  for (int re = 0; re < BLOCK_WIDTH; re++) {
    for (int im = 0; im < BLOCK_HEIGHT; im++) {
      real_block[re + im*BLOCK_HEIGHT] = this->bounds.left + (re * this->bounds.real_sep()); // + real_sep for each re
      imag_block[re + im*BLOCK_HEIGHT] = this->bounds.top   + (im * this->bounds.imag_sep());
    }
  }

  // *actually* do the search, by invoking our recursive function
  // start from top left, and crawl downwards and rightwards
  this->walk(
    0, // start from this->bounds.left
    0, // start from this->bounds.top
    *((__m512 *) real_block), // our initial block's real component
    *((__m512 *) imag_block)  // our initial block's imaginary component
  );
}

void Viewer::draw() const {
  printf("\033[90m| ");
  // print bounds
  int width; // do not initialise: that will happen in the printf %n
  printf("%f%n", this->bounds.left, &width);
  width = (CELL_N_CHARS * BLOCK_WIDTH * this->bounds.n_blocks_x) - width - 2; // subtract from how wise our render is in the first place. - 2 for padding
  printf("%*f |\n", width, this->bounds.right);

  for (unsigned int y = 0; y < (this->bounds.n_blocks_y * BLOCK_HEIGHT); ++y) {
    putchar('|');
    for (unsigned int x = 0; x < (this->bounds.n_blocks_x * BLOCK_WIDTH); ++x) {
      unsigned block_id = (x / BLOCK_WIDTH) + (this->bounds.n_blocks_x * (y / BLOCK_HEIGHT));
      unsigned block_cell = (x % BLOCK_WIDTH) + (BLOCK_WIDTH * (y % BLOCK_HEIGHT));

      hue_t colour = this->huebuf.hues[block_cell + (block_id * BLOCK_N_CELLS)];

      if (colour) {
        printf("\033[48;2;6;%u;6m" CELL_STR, -((signed) colour) & 0xff);
      } else {
        printf("\033[40m" CELL_STR);
      }
    }
    printf("\033[40m|");
    if (y == 0) { // first one
      printf(" %f", this->bounds.top);
    } else if (y + 1 == this->bounds.n_blocks_y * BLOCK_HEIGHT) { // last one
      printf(" %f", this->bounds.bottom);
    }
    putchar('\n');
  }
}

Viewer::Viewer(BoundInfo box) : bounds(box) {
  // doesn't matter which union element we use, but using xmmtab to demonstrate 16-alignment
  this->huebuf.xmmtab = (__m128i *) aligned_alloc(16, this->bounds.n_blocks_x * this->bounds.n_blocks_y * BLOCK_N_CELLS); // has to be aligned at 16 bytes for an xmm register
  this->visited = malloc(this->bounds.n_blocks_y * this->bounds.n_blocks_x / 8); // 8 bits per byte
}

Viewer::~Viewer() {
  free(this->huebuf.xmmtab);
  free(this->visited);
}
