#include "include/sierpinski_carpet.hh"
#include "include/viewer.hh"
#include <immintrin.h>

// 1.0f in hex
#define ONE 0x3F80'0000

__m128i SierpinskiCarpet::compute(uint8_t iterations, __m512 x_block, __m512 y_block) const noexcept {
  // number of iterations to be worked out from oom of sep.
  // we throw out the original value, it's completely useless for this
  const auto [x_sep, y_sep] = this->bounds.calculate_sep();

  // formula is as follows:
  // iterations = 2 - (floor(log2(min(x_sep, y_sep))) / 2)
  //
  // this starts at about 5 or 6, and as each zoom level is x4, this should grow by one with each zoom level
  signed int exponent;
  asm volatile (
    "minss %2, %1\n"
    "vgetexpss %1, %1, %1\n"
    "cvttss2si %1, %0"
    : "=r" (exponent)
    : "x" (x_sep),
      "x" (y_sep)
  );

  // throw away original value
  iterations = 2 - (exponent >> 1);

  // we mask away the ones we know for sure are holes, so can stop calculating
  // at first, set it to all ones. (optimised down to kxnor, but only if placed *after* the asm volatile)
  __mmask16 still_left = _cvtu32_mask16(0xffff);

  // progressively multiply by 3. if the integer part of both dimensions is 1, then is a hole.
  // then take fractional part, and repeat
  //
  // in other words, if both coordinates have the trit 1 in the same position in their ternary expansions, then hole
  for (; iterations > 0; iterations--) {
    // integer parts of [xy]_block
    __m512 x_int = _mm512_floor_ps(x_block);
    __m512 y_int = _mm512_floor_ps(y_block);

    // using integer comparison, because it's faster
    __mmask16 x_is_1 = _mm512_mask_cmpeq_epi32_mask(still_left, (__m512i) x_int, _mm512_set1_epi32(ONE));
    __mmask16 y_is_1 = _mm512_mask_cmpeq_epi32_mask(still_left, (__m512i) y_int, _mm512_set1_epi32(ONE));

    // we want still_left to turn off iff both of x_is_1 and y_is_1
    // therefore !(x1 & y1) & still_left
    still_left = _kandn_mask16(
      _kand_mask16(x_is_1, y_is_1),
      still_left
    );

    // set [xy]_block to fractional parts, then multiply by three
    x_block = _mm512_mul_ps(_mm512_sub_ps(x_block, x_int), _mm512_set1_ps(3.0f));
    y_block = _mm512_mul_ps(_mm512_sub_ps(y_block, y_int), _mm512_set1_ps(3.0f));
  }

  // set anything still left to always on (1 = brightest), otherwise zero
  return _mm_maskz_set1_epi8(still_left, 1);
}

// call parent constructor with bounds info
//
// go from 0 to 1, with 3x3 aspect ratio.
// this will decrease to 4x4 next layer down, but thats ok as [0, ⅓) usually looks cleaner than [0, 1)
SierpinskiCarpet::SierpinskiCarpet() :
  Viewer(terminal::Limits(0.0f, 1.0f, 0.0f, 1.0f, 3, 3))
{}
