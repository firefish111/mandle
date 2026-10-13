#include "viewer.hh"

class SierpinskiCarpet : public Viewer {
private:
  // will ignore iteration value here, as 255 is way too many
  __m128i compute(uint8_t, __m512 x_block, __m512 y_block) const noexcept override;

public:
  SierpinskiCarpet();
};
