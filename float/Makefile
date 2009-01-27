PREFIX =
CC = $(PREFIX)gcc
CFLAGS = -O3

.PHONEY: all clean

all : test testcxx

test : fastfloat.o
testcxx : fastfloat.o

clean :
	$(RM) test testcxx fastfloat.o
