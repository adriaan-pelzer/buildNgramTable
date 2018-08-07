#include <hashTableO1.h>

#ifndef _POPULATE_NGRAMS_LOCATIONS_H_
#define _POPULATE_NGRAMS_LOCATIONS_H_

#define NGRAMS_LOCATIONS_HT_SIZE 2097120 * 2

typedef struct LatLong {
    long double lat;
    long double lon;
} LatLong_t;

typedef struct LatLongList {
    size_t size;
    LatLong_t *lat_lon;
    char *ngram;
} LatLongList_t;

hashTable_t *populate_ngrams_locations ( hashTable_t *intermediate );
int serialise_ngrams_locations ( hashTable_t *ngrams_locations, const char *filename );
hashTable_t *deserialise_ngrams_locations ( const char *filename );

#endif
