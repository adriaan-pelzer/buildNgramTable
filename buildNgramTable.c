#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <hashTableO1.h>
#include <parseMap.h>

#include "populateIntermediate.h"
#include "populateNgramsLocations.h"

//#define INTERMEDIATE_HT_SIZE 134215680
#define INTERMEDIATE_HT_SIZE 4096
//#define NGRAMS_LOCATIONS_HT_SIZE 2097120 * 2
#define NGRAMS_LOCATIONS_HT_SIZE 4096


int main ( int argc, char **argv ) {
    int rc = EXIT_FAILURE;
    char *input_filename = NULL, *output_filename = NULL;
    handler elm_handler = element_handler;
    hashTable_t *intermediate = NULL;
    ngramsLocations_t *ngrams_locations = NULL;
    //size_t i = 0;

    if ( argc < 3 ) {
        fprintf ( stderr, "Usage: %s infile outfile\n", argv[0] );
        goto over;
    }

    input_filename = argv[1];
    output_filename = argv[2];

    if ( ( intermediate = hashTable_create ( INTERMEDIATE_HT_SIZE ) ) == NULL ) {
        fprintf ( stderr, "Cannot allocate memory for hashTable: %s\n", strerror ( errno ) );
        goto over;
    }

    parseMap_register_handler ( elm_handler );
    parseMap_register_user_data ( (void *) intermediate );

    if ( parseMap_read ( input_filename ) == EXIT_FAILURE ) {
        fprintf ( stderr, "Cannot parse map: %s\n", strerror ( errno ) );
        goto over;
    }

    if ( ( ngrams_locations = populate_ngrams_locations ( intermediate, NGRAMS_LOCATIONS_HT_SIZE ) ) == NULL ) {
        fprintf ( stderr, "Cannot populate ngrams -> locations hash table: %s\n", strerror ( errno ) );
        goto over;
    }

    if ( intermediate )
        hashTable_free ( intermediate );

    intermediate = NULL;

    if ( ngramsLocations_serialise ( ngrams_locations, output_filename ) == EXIT_FAILURE ) {
        fprintf ( stderr, "Cannot write ngrams -> locations hash table to output file: %s\n", strerror ( errno ) );
        goto over;
    }

    rc = EXIT_SUCCESS;
over:
    if ( ngrams_locations )
        ngramsLocations_free ( ngrams_locations );

    if ( intermediate )
        hashTable_free ( intermediate );

    return rc;
}
