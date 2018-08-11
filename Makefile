CFLAGS=-g -Wall -fPIC -I/usr/local/include
LIBS=-L/usr/local/lib -lParseMap -lHashTableO1 -lNgramsLocations

all: buildNgramTable

buildNgramTable: Makefile buildNgramTable.o populateIntermediate.o populateNgramsLocations.o
	gcc ${LIBS} -o buildNgramTable populateNgramsLocations.o buildNgramTable.o populateIntermediate.o

populateNgramsLocations.o: populateNgramsLocations.c populateNgramsLocations.h
populateIntermediate.o: populateIntermediate.c populateIntermediate.h

clean:
	rm core*; rm *.o; rm buildNgramTable
