#pragma once

#include "complex_quadratic.hh"

class Mandelbrot : public ComplexQuadratic {
  // exclusively overrides

  InitialConditions initial(__m512 real_block, __m512 imag_block) const;

  // set the escape radius to 2, as people with PhDs said so. we return this squared to ease computation
  constexpr float squared_escape_radius() const;

public:
  Mandelbrot();
};
