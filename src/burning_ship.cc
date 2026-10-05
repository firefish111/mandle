#include "include/burning_ship.hh"
#include "include/escape_time.hh"
#include "include/mandelbrot.hh"
#include <immintrin.h>

// do the same as normal, just take the absolute value of each of the components first
// it looks like a "burning ship" if the y-axis is inverted.
// we do this as part of bounds, by swapping top and bottom, as it's not really the fractal itself we're flipping
void BurningShip::iterate(State * s, __mmask16 write_mask) const noexcept {
  // abs is just a shorthand for & 0x7f'ff'ff'ff
  s->z_real = _mm512_abs_ps(s->z_real);
  s->z_imag = _mm512_abs_ps(s->z_imag);

  // call base implementation
  Mandelbrot::iterate(s, write_mask);
}

// reflect in y axis, then translate "down" by 0.5, as then ship is visible in full
BurningShip::BurningShip() :
  Mandelbrot(terminal::Limits(-2.0f, 1.0f, 0.5f, -1.5f, 6, 4))
{}
