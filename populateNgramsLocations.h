#include <hashTable.h>

#ifndef _POPULATE_NGRAMS_LOCATIONS_H_
#define _POPULATE_NGRAMS_LOCATIONS_H_

struct LatLong {
    long double lat;
    long double lon;
};

struct LatLongList {
    size_t size;
    struct LatLong *lat_lon;
    char *ngram;
};

struct hashTable *populate_ngrams_locations ( struct hashTable *intermediate );
int serialise_ngrams_locations ( struct hashTable *ngrams_locations, const char *filename );
struct hashTable *deserialise_ngrams_locations ( const char *filename );

#endif
