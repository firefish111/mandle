.DEFAULT_GOAL := mandle

# -no-pie is necessary for assembly code, as compiler otherwise gets squeamish about relocation data
# -march=native so that it actually generates fancy instructions.
# tested on -march=tigerlake.
CFLAGS := -O3 -march=native -no-pie

mandle: mandle.cc mandle.s
	g++ $^ $(CFLAGS) -o $@
