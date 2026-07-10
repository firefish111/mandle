.DEFAULT_GOAL := mandel

# -no-pie is necessary for assembly code, as compiler otherwise gets squeamish about relocation data
# -march=native so that it actually generates fancy instructions.
# tested on -march=tigerlake.
CFLAGS := -O3 -march=native -no-pie

mandel: mandel.c mandel.s
	gcc $^ $(CFLAGS) -o $@
