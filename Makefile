DESTDIR?=
PREFIX?=/usr/local

CFLAGS=-g -Wall -I${PREFIX}/include
LIBS=-L${PREFIX}/lib -lbsdconv

all: chiconv

chiconv: chiconv.c
	$(CC) ${CFLAGS} -o chiconv chiconv.c ${LIBS}

install:
	install -m 555 chiconv ${DESTDIR}${PREFIX}/bin

clean:
	rm -f chiconv
