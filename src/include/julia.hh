#pragma once

#include "complex_quadratic.hh"

// julia set. instead of varying c, vary z and provide a constant c.
class Julia : public ComplexQuadratic {
  const float c_real;
  const float c_imag;

  InitialConditions initial(__m512 real_block, __m512 imag_block) const;

  // escape radius R > 0 must adhere to the following inequality: R^2 - R >= |c| given c.
  // we choose the smallest reasonable R by taking the ceiling
  // this is constexpr so we can afford to do some fancy things with operations
  constexpr float squared_escape_radius() const;

public:
  // call parent constructor with bounds info. also log the value of c
  Julia(float c_real, float c_imag);
};
