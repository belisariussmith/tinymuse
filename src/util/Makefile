CC =  gcc
CFLAGS = -Wall -g -fpcc-struct-return -Wpointer-arith -I..
TOPDIR = ../..
LIBS = -L/usr/local/lib 

all: mkindx 

clean:
	rm -f *.o *.a mkindx

install: all
	cp mkindx ../../bin
