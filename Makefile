CFLAGS=-g -Wall -fPIC -I/usr/local/include
LIBS=-L/usr/local/lib -lParseMap -lHashTableO1

all: buildNgramTable

buildNgramTable: Makefile buildNgramTable.o populateIntermediate.o
	gcc ${LIBS} -o buildNgramTable buildNgramTable.o populateIntermediate.o

clean:
	rm core*; rm *.o; rm buildNgramTable
