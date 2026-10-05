#pragma once

#include "viewer.hh"
#include <immintrin.h>

// superclass of Julia and Mandelbrot sets.
// this is an iterative function that creates a 2d plot of an escape-time graph in the complex plane.
// by default in z <- z^2 + c, where z,c \in \mathbb{C}. the exact function can be changed by children
// because the only difference between them is the initial conditions, we have a virtual method for our children to define them.
class EscapeTime : public Viewer {
protected:
  struct State {
    __m512 z_real;
    __m512 z_imag;
    __m512 c_real;
    __m512 c_imag;
  };

  // return initial conditions based on the iterating block
  virtual State initial(__m512 real_block, __m512 imag_block) const noexcept = 0;

  // progress the iteration by one
  virtual void iterate(State * s, __mmask16 write_mask) const noexcept;

  // the radius after which infinity is guaranteed.
  // squared to ease computation
  // constexpr so it "can" be evaluated at compile time, might not be if too complex
  virtual constexpr float squared_escape_radius() const noexcept = 0;

private:
  // has to take a this pointer, but it is never used.
  // this is because static methods can't be virtual, as an upcast to a parent class
  // will result in the wrong static method being called
  __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const noexcept override;

  // force inherit constructor. this is because our children cannot inherit straight from Viewer
  using Viewer::Viewer;
};
