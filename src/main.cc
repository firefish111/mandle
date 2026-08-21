#include <cstdio>

#include "include/viewer.hh"
#include "include/mandelbrot.hh"
#include "include/julia.hh"
#include "include/douady_rabbit.hh"

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

  class Viewer *v = nullptr; // "nullptr is better" - c++ spec
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

  v->walk();
  v->draw();

  return 0;
}
