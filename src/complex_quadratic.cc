#include "include/complex_quadratic.hh"

__m128i ComplexQuadratic::compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const {
  // what we want to do is z <- z^2 + c, where z,c \in \mathbb{C}.
  // because they're both complex, we have to do slightly different things to both the real and imaginary components.
  // there is an intrinsic for complex multiplication, but it's only available on the fanciest xeons, and operates only on half floats, so we do it ourselves

  // can't not be auto
  auto [z_real, z_imag, c_real, c_imag] = this->initial(real_block, imag_block);

  // we mask away the ones we don't want to continue calculating, as otherwise they'll wind up at infinity or NaN.
  // at first, set it to all ones. (optimised down to kxnor)
  __mmask16 still_left = _cvtu32_mask16(0xffff);

  // hue table. this is set to the value of how many iterations are left once it surpasses the boundary, so smaller values are more intense colours, but 0 is black.
  // we set this value only exactly when we surpass the boundary, so we at first set it all to 0.
  __m128i hues = _mm_set1_epi8(0);

  for (; iterations > 0; iterations--) {
    // do the boundary check. people with PhDs have proven that if |z| >= R where R is an escape radius, it is guaranteed to tend to infinity.
    // R can mean different things depending on which sets, q.v. for more.
    // therefore, we do this check. to save us computing the square root, we instead do |z|^2 >= R^2,
    // which is easily calculable as Re(z)^2 + Im(z)^2.
    // don't worry, optimiser will be kind enough to reuse the squaring of these values.
    __m512 square_magnitude = _mm512_add_ps(
      _mm512_mul_ps(z_real, z_real),
      _mm512_mul_ps(z_imag, z_imag)
    );

    // compare with mode NLT_UQ, which means "Not Less Than Unordered Quiet", or in other words:
    // Not Less Than, i.e. greater than or equal to. the weird name is because this is the inverted mode of GE_OQ, which behaves "properly" with NaNs.
    // Unordered. typically, with ordered comparison, NaN is not less than, equal to, or greater than anything. Unordered is the opposite.
    // we want this behaviour, as the only way NaN can be achieved here is with Inf - Inf, which guarantees we've reached infinity, so we want true to be returned.
    // and finally Quiet, so it doesn't throw the toys out the pram if it finds a NaN
    // we save this to a mask, so we can do the operations respective to the results.
    __mmask16 infinite = _mm512_mask_cmp_ps_mask(
      still_left, // we mask out the operation with the still_left mask, as we only want to find ones that have *changed* since last iteration, as otherwise we'll be setting every colour.
      square_magnitude, // |z|^2 >= 4
      _mm512_set1_ps(this->squared_escape_radius()), // splat of escape radius squared. optimiser is clever enough to deal with this efficiently
      _CMP_NLT_UQ // mode, see above
    );

    // set hues for only the newly-found infinite values,
    // by masking out a broadcast of our loop counter
    hues = _mm_mask_set1_epi8(hues, infinite, iterations);

    // we don't want to continue calculation for those that we've already proven tend to infinity.
    // therefore, we turn off specifially those bits in our existing mask, eliminating those
    still_left = _kandn_mask16(infinite, still_left);

    // z = z^2 + c. in other words:
    // Re(z) = Re(z)^2 - Im(z)^2 + Re(c)
    // Im(z) = 2 * Re(z) * Im(z) + Im(c)

    // we don't set them right away, for two reasons: to add the masks later,
    // and that Im(z) depends on Re(z), and we don't want this to be the new Re(z).
    __m512 next_z_real = _mm512_add_ps(
      _mm512_sub_ps(
        _mm512_mul_ps(z_real, z_real), // optimiser is clever enough to squash this into a fused multiply-add instruction
        _mm512_mul_ps(z_imag, z_imag)
      ),
      c_real
    );

    __m512 next_z_imag = _mm512_add_ps(
      _mm512_mul_ps(
        _mm512_add_ps(z_real, z_real), // efficient 2 * Re(z)
        z_imag
      ),
      c_imag
    );

    // optimised away and into the most recent operation (in both cases, the add)
    z_real = _mm512_mask_mov_ps(z_real, still_left, next_z_real);
    z_imag = _mm512_mask_mov_ps(z_imag, still_left, next_z_imag);
  }

  return hues;
};
