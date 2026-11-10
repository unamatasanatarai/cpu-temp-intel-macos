cputemp: cputemp.c
	clang -O2 -Wall -Wextra -o $@ $< -framework IOKit

test: cputemp
	./test.sh

install: cputemp
	install -m 755 cputemp /usr/local/bin/cputemp

clean:
	rm -f cputemp

.PHONY: test install clean
