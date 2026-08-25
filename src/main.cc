#include <cstdio>

#include "include/viewer.hh"
#include "include/terminal.hh"
#include "include/mandelbrot.hh"
#include "include/julia.hh"
#include "include/douady_rabbit.hh"

#include <csignal>

// deal with sigwinch
void init_sigwinch() {
  std::signal(SIGWINCH, terminal::set_winsize);

  /* NOTE:
   * this is actually useless, as boundbox has no easy way of updating ALL viewports
   * en masse, especially if zoomed in really far (will be VERY slow on each SIGWINCH).
   * therefore, this sigwinch handler only exists for completeness' sake.
   */
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
usage:
    fprintf(stderr, "usage:\t%s m - show mandelbrot set\n"
      "\t%s j [real] [imaginary] - show julia set with c = [real] + [imaginary] * i\n"
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

    v->bounds.zoom_in(1, 0);
    v->bounds.zoom_in(2, 1);
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

  v->walk();
  v->draw();

  return 0;
}
