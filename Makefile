.DEFAULT_GOAL := mandle

# -march=native so that it actually generates fancy instructions.
# tested on -march=tigerlake.
CFLAGS := -O3 -march=native -Wall -Wextra -Werror -std=c++20

mandle: mandle.cc
	g++ $^ $(CFLAGS) -o $@
