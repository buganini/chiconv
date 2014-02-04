DESTDIR?=
PREFIX?=/usr/local
LOCALBASE?=${PREFIX}

CFLAGS=-Wall -I${LOCALBASE}/include -O2
LIBS=-L${LOCALBASE}/lib -lbsdconv

all: chiconv

chiconv: chiconv.c
	$(CC) ${CFLAGS} -o chiconv chiconv.c ${LIBS}

install:
	install -m 555 chiconv ${DESTDIR}${PREFIX}/bin

clean:
	rm -f chiconv
