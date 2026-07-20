#pragma once

#include <cstdint>
#include <immintrin.h>

// everything is done in computation blocks.
// the compute() function calculates an entire computation block at once through SIMD,
// so we split the space into blocks, which are walked and calculated by the Viewer
#define BLOCK_WIDTH    4
#define BLOCK_HEIGHT   4
#define BLOCK_N_CELLS 16

#define BORDER_TOP_LEFT "┌"
#define BORDER_TOP_RIGHT "┐"
#define BORDER_SIDE "│"

#define CELL_FULL_STR " "
#define CELL_HALF_STR "▄"
// strlen won't work because unicode (grrrr)
#define CELL_N_CHARS 1

// how many cells per character
#define X_DENSITY 1
#define Y_DENSITY 2

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

// 0 = tends to 0
// 1-255 = tends to infinity, slowest = 1, fastest = 255
typedef uint8_t hue_t;

// abstract class for viewer of any type
class Viewer {
  union HueTable { // easier type punning of hue table
    hue_t   * hues;
    __m128i * xmmtab;
  };
  union HueTable huebuf;

  // bit vector
  void * visited;

protected:
  // info about the bounds of the window being created
  struct BoundInfo {
    // the edges of the viewport
    float left;
    float right;
    float top;
    float bottom;

    // the number of computation blocks across and down
    uint8_t n_blocks_x;
    uint8_t n_blocks_y;

    // need to be const as only const methods can be called in a const method
    constexpr float real_sep() const {
      // subtract one from number of cells to draw to ensure range is inclusive at both ends, making x-axis symmetry more obvious
      return (this->right - this->left) / (this->n_blocks_x * BLOCK_WIDTH - 1);
    }

    constexpr float imag_sep() const {
      // ditto
      return (this->bottom - this->top) / (this->n_blocks_y * BLOCK_HEIGHT - 1);
    }
  };

  const struct BoundInfo bounds;

  // compute function. takes in one block, and returns table of hues for that block.
  virtual __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const = 0;

private:
  void walk(unsigned block_x, unsigned block_y, __m512 real, __m512 imag) const;

public:
  void walk() const;
  void draw() const;

  Viewer(BoundInfo box);
  ~Viewer();
};
