.DEFAULT_GOAL := mandle

# -march=native so that it actually generates fancy instructions.
# tested on -march=tigerlake.
CFLAGS := -O3 -march=native -Wall -Wextra -Werror -std=c++20

SRC_FILES := $(wildcard src/*.cc)
HEADER_FILES := $(wildcard src/include/*.hh)

mandle: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CFLAGS) -o $@
