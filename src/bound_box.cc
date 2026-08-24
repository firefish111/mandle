#include "include/terminal.hh"

namespace Terminal {
  // need to be const as only const methods can be called in a const method
  float BoundBox::real_sep() const {
    // subtract one from number of cells to draw to ensure range is inclusive at both ends, making x-axis symmetry more obvious
    return (this->right - this->left) / (this->rect_size_blks.first * BLOCK_WIDTH - 1);
  }

  float BoundBox::imag_sep() const {
    // ditto
    return (this->bottom - this->top) / (this->rect_size_blks.second * BLOCK_HEIGHT - 1);
  }

  BoundBox::BoundBox(float left, float right, float bottom, float top, uint8_t aspect_ratio_x, uint8_t aspect_ratio_y) :
    left(left),
    right(right),
    bottom(bottom),
    top(top),
    rect_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      Terminal::get_maximum_dimensions(
        aspect_ratio_x, aspect_ratio_y,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    ),
    square_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      Terminal::get_maximum_dimensions(
        2, 2,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    ),
    layer(0)
  {
    if ((this->rect_size_blks.first == 0 || this->rect_size_blks.second == 0) ||
       (this->square_size_blks.first == 0 || this->square_size_blks.second == 0)) {
      throw "Cannot create BoundBox with width or height 0; failed to get terminal size or terminal too small";
    }
  }
}
