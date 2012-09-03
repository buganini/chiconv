CFLAGS=-g -Wall -I/usr/include
LIBS=-L/usr/lib -lbsdconv

all: chiconv

chiconv: chiconv.c
	$(CC) ${CFLAGS} -o chiconv chiconv.c ${LIBS}

clean:
	rm -f chiconv
