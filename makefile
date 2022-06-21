# this makefiles compiles the cobs library (cobs.c)
# and the cobs_test program (cobs_test.c)

all: cobs_test

cobs_test: cobs.o cobs_test.o
	clang -g -o cobs_test cobs_test.o cobs.o

clean:
	rm -f cobs_test cobs.o cobs_test.o