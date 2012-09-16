PREFIX?=/usr/local

CFLAGS=-Wall -I${PREFIX}/include
LIBS=-L${PREFIX}/lib -lbsdconv

all: chiconv

chiconv: chiconv.c
	$(CC) ${CFLAGS} -o chiconv chiconv.c ${LIBS}

install:
	install -m 555 chiconv ${PREFIX}/bin

clean:
	rm -f chiconv
