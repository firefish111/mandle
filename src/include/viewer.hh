#pragma once

#include "terminal.hh"
#include <cstdint>
#include <immintrin.h>

#define BORDER_TOP_LEFT "┌"
#define BORDER_TOP_RIGHT "┐"
#define BORDER_SIDE "│"

// wrapper around bts instruction. inline so it gets optimised away.
inline bool bit_test_and_set_high(const void *src, uint32_t bit) {
  bool o;
  asm volatile ("btsl %2, (%1)" :
      "=@ccc" (o)
      : "r" (src),
        "ir" (bit)
      : "cc" );
  return o;
}

// abstract class for viewer of any type
class Viewer {
  union HueTable { // easier type punning of hue table
    hue_t   * hues;
    __m128i * xmmtab;
  };
  union HueTable huebuf;

  // bit vector
  void * visited;

public:
  terminal::BoundBox bounds;

protected:
  // compute function. takes in one block, and returns table of hues for that block.
  virtual __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const = 0;

private:
  void walk(unsigned block_x, unsigned block_y, __m512 real, __m512 imag) const;

public:
  void walk() const;
  void draw() const;

  Viewer(terminal::Limits lim);
  ~Viewer();
};
