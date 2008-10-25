LDFLAGS := $(shell aqbanking-config --libraries)
CFLAGS := $(shell aqbanking-config --includes) -Wall -W

all: hbcicron
hbcicron: hbcicron.o

clean:
	rm -f hbcicron *.o *~
