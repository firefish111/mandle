#include "include/terminal.hh"

#define KEYBOARD_WIDTH  10
#define KEYBOARD_HEIGHT 4
// The keyboard, used for determining which keeys to press for zooming in/out.
// stored as a 10 x 4 matrix.
const char *keyboard = "1234567890qwertyuiopasdfghjkl;zxcvbnm,./";

namespace Terminal {
  Limits::Limits(float left, float right, float bottom, float top, uint8_t aspect_ratio_x, uint8_t aspect_ratio_y) :
    left(left),
    right(right),
    bottom(bottom),
    top(top),
    aspect_ratio(aspect_ratio_x, aspect_ratio_y)
  {}

  const Limits& BoundBox::lim() const {
    return this->viewport.top();
  }

  const std::pair<size_t, size_t>& BoundBox::get_size_blks() const {
    // rectangle if master, otherwise square
    if (this->viewport.size() <= 1) {
      return this->rect_size_blks;
    } else {
      return this->square_size_blks;
    }
  }

  void BoundBox::zoom_in(size_t subsquare_x, size_t subsquare_y) {
    const Limits& lim = this->lim();
    if (subsquare_x >= lim.aspect_ratio.first || subsquare_y >= lim.aspect_ratio.second) {
      throw "Can't zoom into selected coordinates; outside of bounds of grid";
    }


    const float spacer_across = (lim.right - lim.left) / ((float) lim.aspect_ratio.first),
                spacer_up     = (lim.top - lim.bottom) / ((float) lim.aspect_ratio.second);

    // NOTE: new_top is calculated, as the letter coordinates word top-down, not bottom-up like actual coordinates
    const float new_left = lim.left + (spacer_across * (float) subsquare_x),
                new_top  = lim.top  - (spacer_up * (float) subsquare_y);

    Limits next(new_left, new_left + spacer_across, new_top - spacer_up, new_top, SUBSQUARE_WIDTH, SUBSQUARE_HEIGHT);

    this->viewport.push(next);
  }

  void BoundBox::zoom_out() {
    this->viewport.pop();
  }

  // gets separator as pair (real, imag)
  // need to be const as only const methods can be called in a const method
  std::pair<float, float> BoundBox::calculate_sep() const {
    const Limits& lim = this->lim();
    const auto size = this->get_size_blks();
    return std::make_pair(
      (lim.right - lim.left) / (size.first * BLOCK_WIDTH - 1),
      (lim.bottom - lim.top) / (size.second * BLOCK_HEIGHT - 1)
    );
  }

  BoundBox::BoundBox(Limits master) :
    rect_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      Terminal::get_maximum_dimensions(
        master.aspect_ratio.first, master.aspect_ratio.second,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    ),
    square_size_blks( // largest possible rectangle of given aspect ratio, with blocks of the given size
      Terminal::get_maximum_dimensions(
        SUBSQUARE_WIDTH, SUBSQUARE_HEIGHT,
        BLOCK_WIDTH / X_DENSITY, BLOCK_HEIGHT / Y_DENSITY
      )
    )
  {
    this->viewport.push(master); // push master onto stack

    if ((this->rect_size_blks.first == 0 || this->rect_size_blks.second == 0) ||
       (this->square_size_blks.first == 0 || this->square_size_blks.second == 0)) {
      throw "Cannot create BoundBox with width or height 0; failed to get terminal size or terminal too small";
    }
  }
}
