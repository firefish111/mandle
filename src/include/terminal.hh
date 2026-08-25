#pragma once

#include <cstdint>

#include <cstdio>
#include <stack>
#include <vector>

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

// for layers beneath the master
#define SUBSQUARE_WIDTH 4
#define SUBSQUARE_HEIGHT 4

// 0 = tends to 0
// 1-255 = tends to infinity, slowest = 1, fastest = 255
typedef uint8_t hue_t;


// NOTE: helper macro to avoid implicit \n from normal puts
#define putstr(s) fputs(s, stdout)

using std::size_t;

// Functions that involve the terminal
namespace terminal {
  // generic offset struct
  // structs have default visibility public
  template <typename T>
  struct offset_t {
    T x;
    T y;
  };

  inline void clear_scr() {
    putstr("\033[2J\033H");
  }

  void put_dual_cell(hue_t top, hue_t bottom, bool reset);

  // unused parameter int for signal. we don't care, so it remains unnamed.
  // we have to declare it here, because of mangler and overloading
  void set_winsize(int = 0);

  offset_t<size_t> get_maximum_dimensions(
    size_t aspect_ratio_x, size_t aspect_ratio_y,
    size_t block_size_x_chars, size_t block_size_y_chars
  );

  // helper struct for limits on each axis
  struct Limits {
    const float left;
    const float right;
    const float bottom;
    const float top;
    const offset_t<uint8_t> aspect_ratio;

    Limits(
      float left,
      float right,
      float bottom,
      float top,
      uint8_t aspect_ratio_x,
      uint8_t aspect_ratio_y
    );
  };

  // info about the bounds of the window being created
  class BoundBox {
  private:
    // the edges of the viewport
    // the first element is the master dimensions: i.e. the main dimensions
    //
    // stack is a container adaptor, that sits infront of an existing container and provides an interface for it
    // by default it uses a deque, which is better for reallocs but worse for speed, so we tell it to use a vector instead
    std::stack<Limits, std::vector<Limits>> viewport;

    // the number of computation blocks across and down
    const offset_t<size_t> rect_size_blks;

    // ditto, but for layers deeper than top layer.
    // top layer may not be square, but we can guarantee that lower ones are
    const offset_t<size_t> square_size_blks;

  public:
    const Limits& lim() const;
    const offset_t<size_t>& get_size_blks() const;

    void zoom_in(size_t subsquare_x, size_t subsquare_y);
    //void zoom_in(char label);
    void zoom_out();

    // get the separators between pixels, as a pair of (real, imag)
    // done as one function to minimise repeated calls to get_size_blks() and lim()
    const offset_t<float> calculate_sep() const;

    BoundBox(Limits master);
  };
}
