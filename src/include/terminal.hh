#pragma once

#include <cstdint>
#include <utility>

// everything is done in computation blocks.
// the compute() function calculates an entire computation block at once through SIMD,
// so we split the space into blocks, which are walked and calculated by the Viewer
#define BLOCK_WIDTH    4
#define BLOCK_HEIGHT   4
#define BLOCK_N_CELLS 16

#define CELL_FULL_STR " "
#define CELL_HALF_STR "▄"
// strlen won't work because unicode (grrrr)
#define CELL_N_CHARS 1

// how many cells per character
#define X_DENSITY 1
#define Y_DENSITY 2

// 0 = tends to 0
// 1-255 = tends to infinity, slowest = 1, fastest = 255
typedef uint8_t hue_t;

// NOTE: helper macro to avoid implicit \n from normal puts
#define putstr(s) fputs(s, stdout)

using std::size_t;

// Functions that involve the terminal
namespace Terminal {
  void put_dual_cell(hue_t top, hue_t bottom, bool reset);

  // unused parameter int for signal. we don't care, so it remains unnamed.
  // we have to declare it here, because of mangler and overloading
  void set_winsize(int = 0);
  void init_sigwinch();

  std::pair<size_t, size_t> get_maximum_dimensions(
    size_t aspect_ratio_x, size_t aspect_ratio_y,
    size_t block_size_x_chars, size_t block_size_y_chars
  );

  // info about the bounds of the window being created
  class BoundBox {
  private:
  public:
    // the edges of the viewport
    // in other words, the master dimensions
    const float left;
    const float right;
    const float bottom;
    const float top;

    // the number of computation blocks across and down
    const std::pair<size_t, size_t> rect_size_blks;

    // ditto, but for layers deeper than top layer.
    // top layer may not be square, but we can guarantee that lower ones are
    const std::pair<size_t, size_t> square_size_blks;

    // which layer we are on
    unsigned layer;

  public:
    float real_sep() const;
    float imag_sep() const;
    BoundBox(
      float left,
      float right,
      float bottom,
      float top,
      uint8_t aspect_ratio_x,
      uint8_t aspect_ratio_y
    );
  };
}
