#pragma once

#include "escape_time.hh"
#include "terminal.hh"

class Mandelbrot : public EscapeTime {
protected:
  // exclusively overrides

  State initial(__m512 real_block, __m512 imag_block) const noexcept override;

  // set the escape radius to 2, as people with PhDs said so. we return this squared to ease computation
  // defined here, because contexpr implies inline, so as to not defy ODR
  constexpr float squared_escape_radius() const noexcept override {
    return 4.0f;
  }


public:
  Mandelbrot();

protected:
  // constructor to specify custom limits
  Mandelbrot(terminal::Limits lim);
};
