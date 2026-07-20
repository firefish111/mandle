#include "include/viewer.hh"
#include "include/terminal.hh"

#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <utility>
#include <algorithm>

using namespace std;

namespace Terminal {
  void put_dual_cell(hue_t top, hue_t bottom) {
    // draw cells. we use one sequence for both background and foreground.
    // we start with background, and if needed the foreground completes it.
    // NOTE: helper macro putstr (which invokes fputs) is used to circumvent puts' implicit \n after the string

    // if top cell is not 0, write rgb(10, hue, 10), otherwise all black (40 is black bg).
    if (top) printf("\033[48;2;10;%u;10", top & 0xff);
    else putstr("\033[40");
    // if top and bottom are equal, i.e. no separate foreground needed
    if (top == bottom) {
      putstr("m" CELL_FULL_STR); // complete sequence; do nothing more, and put a space
    } else { // otherwise, separate foreground
      // complete ansi sequence.
      // if bottom cell is not 0, write rgb(10, hue, 10), otherwise all black (30 is black fg)
      if (bottom) printf(";38;2;10;%u;10m" CELL_HALF_STR, bottom & 0xff);
      else putstr(";30m" CELL_HALF_STR);
    }
  }

  // Calculates the largest possible rectangle of given aspect ratio that can fit inside the current terminal window, with lengths given in blocks across and down.
  // Therefore also has to take the size of a block in characters, so as to be able to round to the nearest.
  // returns a pair (x,y) of the dimensions of this rectangle.
  pair<size_t, size_t> get_maximum_dimensions(size_t aspect_ratio_x, size_t aspect_ratio_y, size_t block_size_x_chars, size_t block_size_y_chars) {
    // get size of terminal window in chars.
    struct winsize ws;
  #if 0
    // NOTE: this was created in POSIX 2024, but not supported in glibc as of 2026
    tcgetwinsize(STDOUT_FILENO, &ws);
  #else
    // IMPORTANT: linux only!
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  #endif


    // convert terminal to maximum number of clean multiples of our aspect ratio
    size_t scaled_width  = (ws.ws_col / aspect_ratio_x) / block_size_x_chars;
    size_t scaled_height = (ws.ws_row / aspect_ratio_y) / block_size_y_chars;

    // get minimum between them, as that is largest possible number of aspect ratio subcells that can be fit orthogonally into terminal
    // to form a square of them (and thus the same aspect ratio as each individual subcell)
    size_t common_factor = min(scaled_width, scaled_height);

    // return pair
    return make_pair(common_factor * aspect_ratio_x, common_factor * aspect_ratio_y);
  }
}
