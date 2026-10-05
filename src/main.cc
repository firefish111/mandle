#include "include/viewer.hh"
#include "include/terminal.hh"
#include "include/mandelbrot.hh"
#include "include/julia.hh"
#include "include/douady_rabbit.hh"
#include "include/burning_ship.hh"

#include <cctype>
#include <cstdio>
#include <csignal>

#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>

// handle an illegal instruction (because computation failed)
void handle_sigill(int) {
  fprintf(stderr, "This program relies on AVX-512 instructions to speed up computation.\n"
    "If you see this error, it means that your CPU does not support them, so cannot continue.\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
usage:
    fprintf(stderr, "usage:\t%s m - show Mandelbrot set\n"
      "\t%s j [real] [imaginary] - show Julia set with c = [real] + [imaginary] * i\n"
      "\t%s dr - show Douady rabbit (equivalent to j %f %f)\n"
      "\t%s bs - show Burning Ship fractal\n",
      argv[0], argv[0], argv[0], DOUADY_REAL, DOUADY_IMAG, argv[0]);
    return 1;
  }
  // argc is guaranteed to be more than 1

  terminal::set_winsize();

  // deal with signals
  {
    /* NOTE: SIGWINCH handler
     * this is actually useless, as boundbox has no easy way of updating ALL viewports
     * en masse, especially if zoomed in really far (will be VERY slow on each SIGWINCH),
     * especially if you consider that there may be multiple SIGWINCHes as window sizing is not instant.
     * therefore, this SIGWINCH handler only exists for completeness' sake.
     */
    std::signal(SIGWINCH, terminal::set_winsize);

    // handle what happens if we hit an illegal instruction (not an avx-512-compatible cpu)
    std::signal(SIGILL, handle_sigill);
  }

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
  } else if (argv[1][0] == 'b' && argv[1][1] == 's') {
    v = new BurningShip();
  } else {
    goto usage;
  }

  std::string inbuf;

  // buffer errors to show after draw
  std::vector<const char *> errorbuf;
  errorbuf.reserve(1); // we probably will only have at most one error, but we use a vector incase we have multiple

  for (;;) {
    v->walk();
    v->draw();

    // spit out buffer
    for (const char * err : errorbuf) {
      std::cerr << "Error: " << err << std::endl;
    }
    errorbuf.clear();

    // read line from stdin
    std::cout << "> " << std::flush;
    std::getline(std::cin, inbuf);

    if (inbuf.empty()) {
      break; // exit clause: empty line
    }

    for (char c : inbuf) {
      // whitespace
      if (std::isspace(c)) {
        continue;
      }

      switch (c) {
      case '\\':
        v->bounds.zoom_out();
        break;
      case '+': // camera up: view moves south
        v->bounds.nudge(terminal::SOUTH);
        break;
      case '-': // camera down: view nmoves orth
        v->bounds.nudge(terminal::NORTH);
        break;
      case '<': // camera left: view moves west
        v->bounds.nudge(terminal::WEST);
        break;
      case '>': // camera right: view moves east
        v->bounds.nudge(terminal::EAST);
        break;
      default:
        // zoom in
        try {
          v->bounds.zoom_in(c);
        } catch (const std::invalid_argument& e) {
          // buffer the error to be shown *after* next draw, as otherwise it won't be visible
          // write into end of error buffer, to avoid reallocations
          errorbuf.push_back(e.what());

          // whilst it would be good to have transactions, it gets really complicated with copying the boundbox,
          // as it contains a raw (non-owning) pointer to a visited list, that may get realloc'd in the copy,
          // complete breaking everything. so we just stop listening after the error, and reset to after the char loop,
          // using a goto because labelled break doesn't exist.
          //
          // it's not a bug, it's a feature.
          goto after_char_loop;
        }
      }
    }

  after_char_loop:
    // zoom has changed if we get to here, therefore we reinitialise visited
    v->bounds.initialise_visited_from_top();
  }

  return 0;
}
