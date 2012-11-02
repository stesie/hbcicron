DESTDIR ?=
PREFIX ?= /usr/local

LDFLAGS := $(shell aqbanking-config --libraries) $(shell gwenhywfar-config --libraries)
CFLAGS := $(shell aqbanking-config --includes) $(shell gwenhywfar-config --includes) -Wall -W

all: hbcicron
hbcicron: hbcicron.o

clean:
	rm -f hbcicron *.o *~

install: hbcicron hbcicron-mailwrap
	install $^ ${DESTDIR}/${PREFIX}/bin/

