#include "include/viewer.hh"
#include "include/terminal.hh"

#include <cstdio>
#include <csignal>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <utility>
#include <algorithm>

using std::size_t;

namespace Terminal {
  void put_dual_cell(hue_t top, hue_t bottom, bool reset) {
    static hue_t prev_top = 0, prev_bottom = 0;

    // draw cells. we use one sequence for both background and foreground.
    // we start with background, and if needed the foreground completes it.
    // NOTE: helper macro putstr (which invokes fputs) is used to circumvent puts' implicit \n after the string

    bool putting_top = (top != prev_top || reset), putting_bottom = (bottom != prev_bottom || reset);

    if (putting_top || putting_bottom) {
      /* background */
      putstr("\033[");
      if (putting_top) { // background has changed
        if (top) printf("48;2;10;%u;10", top & 0xff);
        else putstr("40");

        prev_top = top;

        if (putting_bottom) putchar(';');
      }

      /* foreground */
      if (putting_bottom) { // foreground has changed
        if (bottom) printf("38;2;10;%u;10", bottom & 0xff);
        else putstr("30");

        prev_bottom = bottom;
      }

      putchar('m');
    }

    if (top == bottom) {
      putstr(CELL_FULL_STR);
    } else {
      putstr(CELL_HALF_STR);
    }
  }

  // global window size: updated on sigwinch.
  // stored up here so we don't call ioctl too many times.
  struct winsize ws;

  // unused parameter int: this is to appease std::signal
  void set_winsize(int) {
    #if defined(_POSIX_VERSION) && _POSIX_VERSION >= 202405L
      #pragma message("using native")
      // NOTE: this was created in POSIX 2024, but not supported in glibc as of 2026
      tcgetwinsize(STDOUT_FILENO, &ws);
    #else
      #pragma message("using ioctl")
      // IMPORTANT: linux only!
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    #endif
  }

  void init_sigwinch() {
    std::signal(SIGWINCH, set_winsize);
  }

  // Calculates the largest possible rectangle of given aspect ratio that can fit inside the current terminal window, with lengths given in blocks across and down.
  // Therefore also has to take the size of a block in characters, so as to be able to round to the nearest.
  // returns a pair (x,y) of the dimensions of this rectangle.
  std::pair<size_t, size_t> get_maximum_dimensions(size_t aspect_ratio_x, size_t aspect_ratio_y, size_t block_size_x_chars, size_t block_size_y_chars) {
    // convert terminal to maximum number of clean multiples of our aspect ratio
    size_t scaled_width  = (ws.ws_col / aspect_ratio_x) / block_size_x_chars;
    size_t scaled_height = (ws.ws_row / aspect_ratio_y) / block_size_y_chars;

    // get minimum between them, as that is largest possible number of aspect ratio subcells that can be fit orthogonally into terminal
    // to form a square of them (and thus the same aspect ratio as each individual subcell)
    size_t common_factor = std::min(scaled_width, scaled_height);

    // return pair
    return std::make_pair(common_factor * aspect_ratio_x, common_factor * aspect_ratio_y);
  }
}
