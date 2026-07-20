#pragma once

#include <cstdint>
#include <utility>

// 0 = tends to 0
// 1-255 = tends to infinity, slowest = 1, fastest = 255
typedef uint8_t hue_t;

using namespace std;

// NOTE: helper macro to avoid implicit \n from normal puts
#define putstr(s) fputs(s, stdout)

// Functions that involve the terminal
namespace Terminal {
  void put_dual_cell(hue_t top, hue_t bottom);

  pair<size_t, size_t> get_maximum_dimensions(
    size_t aspect_ratio_x, size_t aspect_ratio_y,
    size_t block_size_x_chars, size_t block_size_y_chars
  );
}
