#pragma once

#include "viewer.hh"

// superclass of Julia and Mandelbrot sets.
// this is an iterative function that creates a 2d plot is z <- z^2 + c, where z,c \in \mathbb{C}.
// because the only difference between them is the initial conditions, we have a virtual method for our children to define them.
class ComplexQuadratic : public Viewer {
protected:
  struct InitialConditions {
    __m512 z_real;
    __m512 z_imag;
    __m512 c_real;
    __m512 c_imag;
  };

  // return initial conditions based on the iterating block
  virtual InitialConditions initial(__m512 real_block, __m512 imag_block) const = 0;

  // the radius after which infinity is guaranteed.
  // squared to ease computation
  // constexpr so it "can" be evaluated at compile time, might not be if too complex
  virtual constexpr float squared_escape_radius() const = 0;

private:
  // has to take a this pointer, but it is never used.
  // this is because static methods can't be virtual, as an upcast to a parent class
  // will result in the wrong static method being called
  __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const;

  // force inherit constructor. this is because our children cannot inherit straight from Viewer
  using Viewer::Viewer;
};
