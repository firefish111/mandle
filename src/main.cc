#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <immintrin.h>

#define BOUND_LEFT  -2.0
#define BOUND_RIGHT  2.0

#define BOUND_UP     1.0
#define BOUND_DOWN  -1.0

#define SECTION_WIDTH  3
#define SECTION_HEIGHT 3

#define BLOCKS_ACROSS (SECTION_WIDTH  * 4)
#define BLOCKS_DOWN   (SECTION_HEIGHT * 2)

#define BLOCK_WIDTH    4
#define BLOCK_HEIGHT   4
#define BLOCK_N_CELLS 16

#define CELL_STR "  "
#define CELL_N_CHARS strlen(CELL_STR)

// wrapper around bts instruction. inline so it gets optimised away.
inline bool bit_test_and_set_high(const void *src, unsigned int bit) {
  bool o;
  asm volatile ("btsl %2, (%1)" :
      "=@ccc" (o)
      : "r" (src),
        "ir" (bit)
      : "cc" );
  return o;
}

// 0 = tends to 0
// 1-255 = tends to infinity, slowest = 1, fastest = 255
typedef uint8_t hue_t;

// abstract class for viewer of any class
class Viewer {
  union HueTable { // easier type punning of hue table
    hue_t   * hues;
    __m128i * xmmtab;
  };
  union HueTable huebuf;

  struct BoundInfo {
    float left;
    float right;
    float top;
    float bottom;

    uint8_t n_blocks_x;
    uint8_t n_blocks_y;
  };
//  const struct BoundInfo bounds;

  // bit vector
  void * visited;

  const float real_sep;
  const float imag_sep;

  virtual __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const = 0;

  // walk dfs helper function. initially called in other overload
  void walk(unsigned block_x, unsigned block_y, __m512 real, __m512 imag) const {
    // block_x and block_y are which block we are pointing to. we convert this to a linear
    // block id, where blocks are numbered like so:
    //   0  1  2  3  4  5
    //   6  7  8  9 10 11
    //  12 13 14 15 16 17
    //  18 19 20 21 22 23
    unsigned block_id = block_x + (block_y * BLOCKS_ACROSS);

    // we set it to visited, but if it was already visited, we return
    bool was_visited = bit_test_and_set_high(this->visited, block_id);
    if (was_visited) return;

    // do 255 iterations. the function writes to its out buffer how many iterations are left
    // when it surpasses the outer limit, so by starting with the maximum (255), we can just
    // negate it to yield a 1-255 of how many iterations it took, whilst keeping 0 the same.
    this->huebuf.xmmtab[block_id] = this->compute(255, real, imag);

    if (block_x + 1 < BLOCKS_ACROSS) {
      /* recurse across in real direction */
      this->walk(
        block_x + 1,
        block_y,
        _mm512_add_ps(real, _mm512_set1_ps(this->real_sep * BLOCK_WIDTH)), // this is clever enough to optimise into single broadcast instruction, if possible
        imag
      );
    }

    if (block_y + 1 < BLOCKS_DOWN) {
      /* recurse down in imaginary direction */
      this->walk(
        block_x,
        block_y + 1,
        real,
        _mm512_add_ps(imag, _mm512_set1_ps(this->imag_sep * BLOCK_HEIGHT)) // ditto but for imaginary
      );
    }
  }

public:
  // publicly available, begins walking
  void walk() const {
    // to be loaded into a zmm register, it needs to be 64-byte aligned
    // instead of setting them to tyoe __m512 right away, we want to set it to some values,
    // which we can't do as just a __m512 type, so we do the conversion with a type pun at the dfs call
    float real_block[16] __attribute__ ((aligned(64)));
    float imag_block[16] __attribute__ ((aligned(64)));

    // create 4x4 block, starting with BOUND_UP and BOUND_LEFT like so:
    // 0+0i 1+0i 2+0i 3+0i, where each difference across is real_sep
    // 0+1i 1+1i 2+1i 3+1i
    // 0+2i 1+2i 2+2i 3+2i
    // 0+3i 1+3i 2+3i 3+3i
    // where each difference down is imag_sep
    for (int re = 0; re < BLOCK_WIDTH; re++) {
      for (int im = 0; im < BLOCK_HEIGHT; im++) {
        real_block[re + im*BLOCK_HEIGHT] = BOUND_LEFT + (re * this->real_sep); // + real_sep for each re
        imag_block[re + im*BLOCK_HEIGHT] = BOUND_UP   + (im * this->imag_sep);
      }
    }

    // *actually* do the search, by invoking our recursive function
    // start from top left, and crawl downwards and rightwards
    this->walk(
      0, // start from BOUND_LEFT
      0, // start from BOUND_UP
      *((__m512 *) real_block), // our initial block's real component
      *((__m512 *) imag_block)  // our initial block's imaginary component
    );
  }

  void draw() const {
    printf("\033[90m| ");
    // print bounds
    int width; // do not initialise: that will happen in the printf %n
    printf("%f%n", BOUND_LEFT, &width);
    width = (CELL_N_CHARS * BLOCK_WIDTH * BLOCKS_ACROSS) - width - 2; // subtract from how wise our render is in the first place. - 2 for padding
    printf("%*f |\n", width, BOUND_RIGHT);

    for (unsigned int y = 0; y < (BLOCKS_DOWN * BLOCK_HEIGHT); ++y) {
      putchar('|');
      for (unsigned int x = 0; x < (BLOCKS_ACROSS * BLOCK_WIDTH); ++x) {
        unsigned block_id = (x / BLOCK_WIDTH) + (BLOCKS_ACROSS * (y / BLOCK_HEIGHT));
        unsigned block_cell = (x % BLOCK_WIDTH) + (BLOCK_WIDTH * (y % BLOCK_HEIGHT));

        hue_t colour = this->huebuf.hues[block_cell + (block_id * BLOCK_N_CELLS)];

        if (colour) {
          printf("\033[48;2;6;%u;6m" CELL_STR, -((signed) colour) & 0xff);
        } else {
          printf("\033[40m" CELL_STR);
        }
      }
      printf("\033[40m|");
      if (y == 0) { // first one
        printf(" %f", BOUND_UP);
      } else if (y + 1 == BLOCKS_DOWN * BLOCK_HEIGHT) { // last one
        printf(" %f", BOUND_DOWN);
      }
      putchar('\n');
    }
  }

  Viewer() :
    real_sep((BOUND_RIGHT - BOUND_LEFT) / BLOCKS_ACROSS / BLOCK_WIDTH),
    imag_sep((BOUND_DOWN - BOUND_UP) / BLOCKS_DOWN / BLOCK_HEIGHT)
  {
    // doesn't matter which union element we use, but using xmmtab to demonstrate 16-alignment
    this->huebuf.xmmtab = (__m128i *) aligned_alloc(16, BLOCKS_ACROSS * BLOCKS_DOWN * BLOCK_N_CELLS); // has to be aligned at 16 bytes for an xmm register
    this->visited = malloc(BLOCKS_DOWN * BLOCKS_ACROSS / 8); // 8 bits per byte
  }

  ~Viewer() {
    free(this->huebuf.xmmtab);
    free(this->visited);
  }
};

// superclass of Julia and Mandelbrot sets.
// this is an iterative function that creates a 2d plot is z <- z^2 + c, where z,c \in \mathbb{C}.
// because the only difference between them in the initial conditions, we have a method that defines them.
class ComplexQuadraticPolynomial : public Viewer {
protected:
  struct InitialConditions {
    __m512 z_real;
    __m512 z_imag;
    __m512 c_real;
    __m512 c_imag;
  };

  // return initial conditions based on the iterating block
  virtual InitialConditions initial(__m512 real_block, __m512 imag_block) const = 0;

  // the radius after which infinity is guaranteed.
  // squared to ease computation
  // constexpr so it can be evaluated at compile time
  virtual constexpr float squared_escape_radius() const = 0;

private:
  // has to take a this pointer, but it is never used.
  // this is because static methods can't be virtual, as an upcast to a parent class
  // will result in the wrong static method being called
  __m128i compute(uint8_t iterations, __m512 real_block, __m512 imag_block) const {
    // what we want to do is z <- z^2 + c, where z,c \in \mathbb{C}.
    // because they're both complex, we have to do slightly different things to both the real and imaginary components.
    // there is an intrinsic for complex multiplication, but it's only available on the fanciest xeons, and operates only on half floats, so we do it ourselves

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
};

class Mandelbrot : public ComplexQuadraticPolynomial {
  InitialConditions initial(__m512 real_block, __m512 imag_block) const {
    // technically, for mandelbrot, initial z is 0.
    // but, after first iteration, z will always equal c, so we can optimise away first iteration,
    // by setting BOTH values to the same thing
    return (InitialConditions) {
      .z_real = real_block,
      .z_imag = imag_block,
      .c_real = real_block,
      .c_imag = imag_block,
    };
  }

  // set the escape radius to 2, as people with PhDs said so. we return this squared to ease computation
  constexpr float squared_escape_radius() const {
    return 4.0f;
  }

public:
  // basic constructor. parent constructor and destructor are called automatically unless explicitly called
  Mandelbrot() {}
};

// julia set. instead of varying c, vary z and provide a constant c.
class Julia : public ComplexQuadraticPolynomial {
  const float c_real;
  const float c_imag;

  InitialConditions initial(__m512 real_block, __m512 imag_block) const {
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
  constexpr float squared_escape_radius() const {
    // by completing the square, we get that: R >= 1/2 + sqrt(|c| + 1/4), taking the positive root as R > 0.
    // we take the ceil of that to get a reasonable value of R

    // we want to do hypot, but hypot is opaque to optimiser
    // float c_mag = hypotf(this->c_real, this->c_imag); // |c|
    float c_mag = sqrtf(this->c_real*this->c_real + this->c_imag*this->c_imag); // |c|
    float R_base = 0.5f + sqrtf(c_mag + 0.25f);
    return ceilf(R_base);
  }

public:
  Julia(float c_real, float c_imag) :
    c_real(c_real),
    c_imag(c_imag)
    //box(-2.0f, 2.0f, -1.5f, 1.5f, 2, 2)
  {}
};

int main(int argc, char *argv[]) {
  if (argc <= 1) {
usage:
    fprintf(stderr, "usage:\t%s m - show mandelbrot set\n\t%s j [real] [imaginary] - show julia set with c = [real] + [imaginary] * i\n", argv[0], argv[0]);
    return 1;
  }
  // argc is guaranteed to be more than 1

  class Viewer *v = nullptr; // "nullptr is better" - c++ spec
  if (argv[1][0] == 'm') {
    v = new Mandelbrot();
  } else if (argv[1][0] == 'j' && argc >= 4) {
    v = new Julia(
      strtof(argv[2], nullptr), // this function is kind enough to tell me where the float ends, but i don't care so nullptr it is
      strtof(argv[3], nullptr)
    );
  } else {
    goto usage;
  }

  v->walk();
  v->draw();

  return 0;
}
