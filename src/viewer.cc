#include <cstdio>
#include <cstdlib>

#include "include/viewer.hh"
#include "include/terminal.hh"

// walk dfs helper function. initially called in other overload
void Viewer::walk(unsigned block_x, unsigned block_y, __m512 real, __m512 imag) const {
  // block_x and block_y are which block we are pointing to. we convert this to a linear
  // block id, where blocks are numbered like so:
  //   0  1  2  3  4  5
  //   6  7  8  9 10 11
  //  12 13 14 15 16 17
  //  18 19 20 21 22 23
  const auto size = this->bounds.get_size_blks();
  const auto [real_sep, imag_sep] = this->bounds.calculate_sep();

  unsigned block_id = block_x + (block_y * size.first);

  // we set it to visited, but if it was already visited, we return
  bool was_visited = bit_test_and_set_high(this->visited, block_id);
  if (was_visited) return;

  // do 255 iterations. the function writes to its out buffer how many iterations are left
  // when it surpasses the outer limit, so by starting with the maximum (255), we can just
  // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
  this->huebuf.xmmtab[block_id] = this->compute(255, real, imag);

  if (block_x + 1 < size.first) {
    /* recurse across in real direction */
    this->walk(
      block_x + 1,
      block_y,
      _mm512_add_ps(real, _mm512_set1_ps(real_sep * BLOCK_WIDTH)), // this is clever enough to optimise into single broadcast instruction, if possible
      imag
    );
  }

  if (block_y + 1 < size.second) {
    /* recurse down in imaginary direction */
    this->walk(
      block_x,
      block_y + 1,
      real,
      _mm512_add_ps(imag, _mm512_set1_ps(imag_sep * BLOCK_HEIGHT)) // ditto but for imaginary
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

  const Terminal::Limits& lim = this->bounds.lim();
  const auto [real_sep, imag_sep] = this->bounds.calculate_sep();

  // create 4x4 block, starting with this->bounds.top and this->bounds.left like so:
  // 0+0i 1+0i 2+0i 3+0i, where each difference across is real_sep
  // 0+1i 1+1i 2+1i 3+1i
  // 0+2i 1+2i 2+2i 3+2i
  // 0+3i 1+3i 2+3i 3+3i
  // where each difference down is imag_sep
  for (unsigned re = 0; re < BLOCK_WIDTH; re++) {
    for (unsigned im = 0; im < BLOCK_HEIGHT; im++) {
      real_block[re + im*BLOCK_HEIGHT] = lim.left + (re * real_sep); // + real_sep for each re
      imag_block[re + im*BLOCK_HEIGHT] = lim.top  + (im * imag_sep);
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
  const auto size = this->bounds.get_size_blks();
  for (unsigned int y = 0; y < (size.second * BLOCK_HEIGHT); y += Y_DENSITY) {
    for (unsigned int x = 0; x < (size.first * BLOCK_WIDTH); x += X_DENSITY) {
      unsigned block_id = (x / BLOCK_WIDTH) + (size.first * (y / BLOCK_HEIGHT));
      unsigned block_cell = (x % BLOCK_WIDTH) + (BLOCK_WIDTH * (y % BLOCK_HEIGHT));

      // for double density, a bottom-half block is shown, with background being top colour, and foreground being bottom colour.
      // two cells' colour, top and bottom.
      // HACK: we can safely assume that y is always even so thus will never straddle the border between blocks
      // this allows us to just simply add BLOCK_WIDTH to the index to get the one beneath
      hue_t col_hi = this->huebuf.hues[block_cell + (block_id * BLOCK_N_CELLS)];
      hue_t col_lo = this->huebuf.hues[block_cell + (block_id * BLOCK_N_CELLS) + BLOCK_WIDTH];

      Terminal::put_dual_cell(-((signed) col_hi), -((signed) col_lo), !(x == 0 && y == 0));
    }

    // reset all style
    printf("\033[0m");

    putchar('\n'); // new line; next row
  }
}

Viewer::Viewer(Terminal::Limits lim) : bounds(lim) {
  const auto size = this->bounds.get_size_blks();
  // doesn't matter which union element we use, but using xmmtab to demonstrate 16-alignment
  this->huebuf.xmmtab = (__m128i *) aligned_alloc(16, size.first * size.second * BLOCK_N_CELLS); // has to be aligned at 16 bytes for an xmm register
  this->visited = malloc(size.first * size.second / 8); // 8 bits per byte
}

Viewer::~Viewer() {
  free(this->huebuf.xmmtab);
  free(this->visited);
}
