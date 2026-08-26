#include "include/viewer.hh"
#include "include/terminal.hh"
#include "include/mandelbrot.hh"
#include "include/julia.hh"
#include "include/douady_rabbit.hh"

#include <cstdio>
#include <csignal>
#include <cstring>

#include <string>
#include <iostream>

// deal with SIGWINCH
void init_sigwinch() {
  std::signal(SIGWINCH, terminal::set_winsize);

  /* NOTE:
   * this is actually useless, as boundbox has no easy way of updating ALL viewports
   * en masse, especially if zoomed in really far (will be VERY slow on each SIGWINCH).
   * therefore, this SIGWINCH handler only exists for completeness' sake.
   */
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
usage:
    fprintf(stderr, "usage:\t%s m - show Mandelbrot set\n"
      "\t%s j [real] [imaginary] - show Julia set with c = [real] + [imaginary] * i\n"
      "\t%s dr - show Douady rabbit (equivalent to j %f %f)\n",
      argv[0], argv[0], argv[0], DOUADY_REAL, DOUADY_IMAG);
    return 1;
  }
  // argc is guaranteed to be more than 1

  terminal::set_winsize();
  init_sigwinch();

  Viewer *v = nullptr; // "nullptr is better" - c++ spec
  if (argv[1][0] == 'm') {
    v = new Mandelbrot();
  } else if (argv[1][0] == 'j' && argc >= 4) {
    v = new Julia(
      strtof(argv[2], nullptr), // this function is kind enough to tell me where the float ends, but i don't care so nullptr it is
      strtof(argv[3], nullptr)
    );
  } else if (argv[1][0] == 'd' && argv[1][1] == 'r') {
    v = new DouadyRabbit();
  } else {
    goto usage;
  }

  std::string inbuf;
  for (;;) {
    v->walk();
    v->draw();

    std::cout << "> ";
    std::cin >> inbuf;

    if (inbuf.empty()) {
      break; // exit clause: empty line
    }

    for (char c : inbuf) {
      if (c == '\\') {
        v->bounds.zoom_out();
        continue;
      }

      // zoom in
      v->bounds.zoom_in(c);
    }
  }

  return 0;
}
