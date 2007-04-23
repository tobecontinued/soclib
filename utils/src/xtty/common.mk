CC=gcc
CFLAGS+=-Wall -O2

all::
	@true

default:: $(BIN)
	@true

%:
	$(CC) -o $@ $^ $($@_LIBS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $($@_CFLAGS) $(CFLAGS)

clean:
	rm -f *~ *.o $(BIN)

