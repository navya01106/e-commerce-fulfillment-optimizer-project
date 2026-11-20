# E-Commerce Order-to-Delivery Planner
#   make          builds the program "optimizer"
#   ./optimizer   runs it (from this folder, so it can find data/)
#   make test     builds and runs the test suite
#   make bench    runs the benchmark without the menu
#   make clean    deletes built files

CC     = gcc
CFLAGS = -std=c17 -Wall -Wextra -O2
LIB    = daa.c planner.c
HDRS   = daa.h planner.h

optimizer: main.c $(LIB) $(HDRS)
	$(CC) $(CFLAGS) main.c $(LIB) -o optimizer

run_tests: tests/tests.c $(LIB) $(HDRS)
	$(CC) $(CFLAGS) -I. tests/tests.c $(LIB) -o run_tests

test: run_tests
	./run_tests

bench: optimizer
	printf '13\n0\n' | ./optimizer

clean:
	rm -f optimizer run_tests

.PHONY: test bench clean
