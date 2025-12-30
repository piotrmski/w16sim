appName := w13sim
CFLAGS  := -std=c23

srcFiles := $(shell find src -name "*.c")
objects  := $(patsubst %.c, %.o, $(srcFiles))

all: $(appName)

$(appName): $(objects)
	$(CC) $(CFLAGS) -O3 -o dist/$(appName) $(objects)
	cp COPYING dist/COPYING

clean:
	rm -f $(objects)