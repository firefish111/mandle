#pragma once

#include <cmath>
#include "complex_quadratic.hh"

// julia set. instead of varying c, vary z and provide a constant c.
class Julia : public ComplexQuadratic {
  const float c_real;
  const float c_imag;

  InitialConditions initial(__m512 real_block, __m512 imag_block) const override;

  // escape radius R > 0 must adhere to the following inequality: R^2 - R >= |c| given c.
  // we choose the smallest reasonable R by taking the ceiling
  // this is constexpr so we can afford to do some fancy things with operations
  // defined here, because contexpr implies inline, so as to not defy ODR
  constexpr float squared_escape_radius() const override {
    // by completing the square, we get that: R >= 1/2 + sqrt(|c| + 1/4), taking the positive root as R > 0.
    // we take the ceil of that to get a reasonable value of R

    // we want to do hypot, but hypot is opaque to optimiser
    // float c_mag = hypotf(this->c_real, this->c_imag); // |c|
    float c_mag = sqrtf(this->c_real*this->c_real + this->c_imag*this->c_imag); // |c|
    float R_base = 0.5f + sqrtf(c_mag + 0.25f);
    return ceilf(R_base);
  }


public:
  // call parent constructor with bounds info. also log the value of c
  Julia(float c_real, float c_imag);
};
