#include <cmath>

#include "include/julia.hh"

Julia::InitialConditions Julia::initial(__m512 real_block, __m512 imag_block) const {
  // parameters are given as what varies.
  // this varies z and sets c to our constant
  return (InitialConditions) {
    .z_real = real_block,
    .z_imag = imag_block,
    .c_real = _mm512_set1_ps(this->c_real),
    .c_imag = _mm512_set1_ps(this->c_imag),
  };
}

// escape radius R > 0 must adhere to the following inequality: R^2 - R >= |c| given c.
// we choose the smallest reasonable R by taking the ceiling
// this is constexpr so we can afford to do some fancy things with operations
constexpr float Julia::squared_escape_radius() const {
  // by completing the square, we get that: R >= 1/2 + sqrt(|c| + 1/4), taking the positive root as R > 0.
  // we take the ceil of that to get a reasonable value of R

  // we want to do hypot, but hypot is opaque to optimiser
  // float c_mag = hypotf(this->c_real, this->c_imag); // |c|
  float c_mag = sqrtf(this->c_real*this->c_real + this->c_imag*this->c_imag); // |c|
  float R_base = 0.5f + sqrtf(c_mag + 0.25f);
  return ceilf(R_base);
}

Julia::Julia(float c_real, float c_imag) :
  ComplexQuadratic(BoundInfo(-2.0f, 2.0f, -1.5f, 1.5f, 12, 9)),
  c_real(c_real),
  c_imag(c_imag)
{}
