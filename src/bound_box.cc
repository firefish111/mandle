#include "include/terminal.hh"

#include <algorithm>
#include <stdexcept>

#define KEYBOARD_WIDTH  10
#define KEYBOARD_HEIGHT 4
// The keyboard, used for determining which keeys to press for zooming in/out.
// stored as a 10 x 4 matrix.
const char *keyboard = "1234567890qwertyuiopasdfghjkl;zxcvbnm,./";

namespace terminal {
  Limits::Limits(float left, float right, float bottom, float top, uint8_t aspect_ratio_x, uint8_t aspect_ratio_y) :
    left(left),
    right(right),
    bottom(bottom),
    top(top),
    aspect_ratio(aspect_ratio_x, aspect_ratio_y)
  {}

  const Limits& BoundBox::lim() const {
    return this->viewport.back();
  }

  const offset_t<size_t>& BoundBox::get_size_blks() const {
    // rectangle if master, otherwise square
    if (this->viewport.size() <= 1) {
      return this->rect_size_blks;
    } else {
      return this->square_size_blks;
    }
  }

  void BoundBox::zoom_in(size_t subsquare_x, size_t subsquare_y) {
    const Limits& lim = this->lim();
    if (subsquare_x >= lim.aspect_ratio.x || subsquare_y >= lim.aspect_ratio.y) {
      throw std::invalid_argument("Can't zoom into selected coordinates; outside of bounds of grid");
    }


    const float spacer_across = (lim.right - lim.left) / ((float) lim.aspect_ratio.x),
                spacer_up     = (lim.top - lim.bottom) / ((float) lim.aspect_ratio.y);

    // NOTE: new_top is calculated, as the letter coordinates word top-down, not bottom-up like actual coordinates
    const float new_left = lim.left + (spacer_across * (float) subsquare_x),
                new_top  = lim.top  - (spacer_up * (float) subsquare_y);

    Limits next(new_left, new_left + spacer_across, new_top - spacer_up, new_top, SUBSQUARE_WIDTH, SUBSQUARE_HEIGHT);

    this->viewport.push_back(next);
  }

  // shorccut to above, but searches for the exact subsquare coordinates, given the key pressed
  // uses the keyboard as a map
  void BoundBox::zoom_in(char label) {
    // flat index, in other words x + y*10
    size_t index = std::find(
      keyboard,
      keyboard + (KEYBOARD_WIDTH * KEYBOARD_HEIGHT),
      label
    ) - keyboard;

    this->zoom_in(index % KEYBOARD_WIDTH, index / KEYBOARD_WIDTH);
  }

  void BoundBox::zoom_out() {
    // only zoom out if it won't remove the master
    if (this->viewport.size() > 1) {
      this->viewport.pop_back();
    }
  }

  // gets separator as pair (real, imag)
  // needs to be const as only const methods can be called in a const method
  const offset_t<float> BoundBox::calculate_sep() const {
    const Limits& lim = this->lim();
    const auto size = this->get_size_blks();
    return offset_t(
      (lim.right - lim.left) / (size.x * BLOCK_WIDTH - 1),
      (lim.bottom - lim.top) / (size.y * BLOCK_HEIGHT - 1)
    );
  }

  BoundBox::BoundBox(Limits master) :
    rect_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      terminal::get_maximum_dimensions(
        master.aspect_ratio.x, master.aspect_ratio.y,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    ),
    square_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      terminal::get_maximum_dimensions(
        SUBSQUARE_WIDTH, SUBSQUARE_HEIGHT,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    )
  {
    // after about 10-12 layers deep, so length of 11-13, starts to break down due to floating point errors.
    // 16 should be enough to stop reallocation at sensible depths
    this->viewport.reserve(16);

    this->viewport.push_back(master); // push master onto stack

    if ((this->rect_size_blks.x == 0 || this->rect_size_blks.y == 0) ||
       (this->square_size_blks.x == 0 || this->square_size_blks.y == 0)) {
      throw std::runtime_error("Cannot create BoundBox with width or height 0; failed to get terminal size or terminal too small");
    }
  }
}
