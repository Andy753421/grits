CFLAGS    = -Wall -g
CPPFLAGS  = -DSYS_X11
LDFLAGS   = -lGL -lGLU -lglut

CPPFLAGS += $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS  += $(shell pkg-config --libs   gtk+-3.0)

CPPFLAGS += -I../../src
VPATH     = ../../src

test: cube
	./cube

cube: cube.o gtkgl.o
	gcc $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c makefile
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f *.o cube
