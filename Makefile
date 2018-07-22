CFLAGS=-g -Wall -fPIC -I/usr/local/include
LIBS=-L/usr/local/lib -lParseMap -lHashTable

all: buildNgramTable

buildNgramTable: Makefile buildNgramTable.o populateIntermediate.o populateNgramsLocations.o
	gcc ${LIBS} -o buildNgramTable buildNgramTable.o populateIntermediate.o populateNgramsLocations.o

clean:
	rm core*; rm *.o; rm buildNgramTable
