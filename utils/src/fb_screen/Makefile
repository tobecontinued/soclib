CFLAGS=`sdl-config --cflags` -Wall -O2
LIBS=`sdl-config --libs`

default: soclib-fb

all: soclib-fb

soclib-fb: fb.o
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o soclib-fb
