#include "include/mandelbrot.hh"
#include "include/terminal.hh"

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

// call parent constructor with bounds info
Mandelbrot::Mandelbrot() :
  ComplexQuadratic(Terminal::BoundBox(-2.0f, 1.0f, -1.0f, 1.0f, 3, 2))
{}
