#pragma once

#include "complex_quadratic.hh"

class Mandelbrot : public ComplexQuadratic {
protected:
  // exclusively overrides

  InitialConditions initial(__m512 real_block, __m512 imag_block) const override;

  // set the escape radius to 2, as people with PhDs said so. we return this squared to ease computation
  // defined here, because contexpr implies inline, so as to not defy ODR
  constexpr float squared_escape_radius() const override {
    return 4.0f;
  }


public:
  Mandelbrot();
};
