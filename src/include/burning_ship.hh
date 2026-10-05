#pragma once

#include "mandelbrot.hh"
#include <immintrin.h>

// the burning ship is basically the mandelbrot set, but on each iteration the absolute value of each component of z is taken.
// it's often flipped in the x axis to achieve images of a "burning ship"
// therefore z <- (|Re(z)| - i|Im(z)|)^2 + c.
//
// initial conditions are the same.
class BurningShip : public Mandelbrot {
  virtual void iterate(State * s, __mmask16 write_mask) const noexcept override;

public:
  BurningShip();
};
