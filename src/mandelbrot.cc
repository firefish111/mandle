#include "include/mandelbrot.hh"

Mandelbrot::InitialConditions Mandelbrot::initial(__m512 real_block, __m512 imag_block) const {
  // technically, for mandelbrot, initial z is 0.
  // but, after first iteration, z will always equal c, so we can optimise away first iteration,
  // by setting BOTH values to the same thing
  return (InitialConditions) {
    .z_real = real_block,
    .z_imag = imag_block,
    .c_real = real_block,
    .c_imag = imag_block,
  };
}

// set the escape radius to 2, as people with PhDs said so. we return this squared to ease computation
constexpr float Mandelbrot::squared_escape_radius() const {
  return 4.0f;
}

// call parent constructor with bounds info
Mandelbrot::Mandelbrot() :
  ComplexQuadratic(BoundInfo(-2.0f, 1.0f, -1.0f, 1.0f, 12, 8))
{}
