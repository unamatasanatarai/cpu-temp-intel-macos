CC = clang
CFLAGS = -O2 -Wall -Wextra
LDLIBS = -framework IOKit
BINS = cputemp osxtemps

all: $(BINS)

cputemp: cputemp.c smc.c smc.h
	$(CC) $(CFLAGS) -o $@ cputemp.c smc.c $(LDLIBS)

osxtemps: osxtemps.c smc.c smc.h
	$(CC) $(CFLAGS) -o $@ osxtemps.c smc.c $(LDLIBS)

test: $(BINS)
	./test.sh

install: $(BINS)
	install -m 755 $(BINS) /usr/local/bin/

clean:
	rm -f $(BINS)

.PHONY: all test install clean
