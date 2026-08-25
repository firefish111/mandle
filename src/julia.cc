#include "include/julia.hh"
#include "include/terminal.hh"

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

Julia::Julia(float c_real, float c_imag) :
  ComplexQuadratic(Terminal::Limits(-2.0f, 2.0f, -1.5f, 1.5f, 4, 3)),
  c_real(c_real),
  c_imag(c_imag)
{}
