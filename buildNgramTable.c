#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <hashTableO1.h>
#include <parseMap.h>

#include "populateIntermediate.h"
//#include "populateNgramsLocations.h"

int main ( int argc, char **argv ) {
    int rc = EXIT_FAILURE;
    char *input_filename = NULL;
    //char *input_filename = NULL, *output_filename = NULL;
    handler elm_handler = element_handler;
    hashTable_t *intermediate = NULL;
    //struct hashTable *intermediate = NULL, *ngrams_locations = NULL;
    //size_t i = 0;

    if ( argc < 2 ) {
        fprintf ( stderr, "Usage: %s infile\n", argv[0] );
        goto over;
    }

    input_filename = argv[1];
    //output_filename = argv[2];

    if ( ( intermediate = hashTable_create ( 65535 ) ) == NULL ) {
        fprintf ( stderr, "Cannot allocate memory for hashTable: %s\n", strerror ( errno ) );
        goto over;
    }

    parseMap_register_handler ( elm_handler );
    parseMap_register_user_data ( (void *) intermediate );

    if ( parseMap_read ( input_filename ) == EXIT_FAILURE ) {
        fprintf ( stderr, "Cannot parse map: %s\n", strerror ( errno ) );
        goto over;
    }

    /*if ( ( ngrams_locations = populate_ngrams_locations ( intermediate ) ) == NULL ) {
        fprintf ( stderr, "Cannot populate ngrams -> locations hash table: %s\n", strerror ( errno ) );
        goto over;
    }

    if ( intermediate )
        hashTable_free ( intermediate );

    intermediate = NULL;

    if ( serialise_ngrams_locations ( ngrams_locations, output_filename ) == EXIT_FAILURE ) {
        fprintf ( stderr, "Cannot write ngrams -> locations hash table to output file: %s\n", strerror ( errno ) );
        goto over;
    }*/

    /*if ( ngrams_locations )
        hashTable_free ( ngrams_locations );

    ngrams_locations = NULL;

    if ( ( ngrams_locations = deserialise_ngrams_locations ( output_filename ) ) == NULL ) {
        fprintf ( stderr, "Cannot read ngrams -> locations hash table from input file: %s\n", strerror ( errno ) );
        goto over;
    }

    for ( i = 0; i < ngrams_locations->size; i++ ) {
        struct hashTableEntry *entry = &ngrams_locations->entries[i];
        struct LatLongList *lat_lon_list = (struct LatLongList *) entry->v;
        size_t j = 0;

        printf ( "%d. %s:\n", (int) i, lat_lon_list->ngram );
        for ( j = 0; j < lat_lon_list->size; j++ ) {
            printf ( "%lf,%lf\n", (double) lat_lon_list->lat_lon[j].lat, (double) lat_lon_list->lat_lon[j].lon );
        }
        printf ( "\n" );
    }*/

    rc = EXIT_SUCCESS;
over:
    //if ( ngrams_locations )
    //    hashTable_free ( ngrams_locations );

    if ( intermediate )
        hashTable_free ( intermediate );

    return rc;
}
