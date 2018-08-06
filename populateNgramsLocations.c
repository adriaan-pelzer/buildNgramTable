#include <string.h>
#include <stdio.h>
#include <hashTableO1.h>

#include "populateIntermediate.h"
#include "populateNgramsLocations.h"

int add_lat_lon_list_to_hashTable ( const char *name, struct LatLongList* lat_lon_list, hashTable_t *ngrams_locations ) {
    int rc = EXIT_FAILURE;
    int _rc = HT_FAILURE;

    if ( ( _rc = hashTable_add_entry ( ngrams_locations, name, (void *) lat_lon_list ) ) == HT_FAILURE ) {
        fprintf ( stderr, "Cannot add entry '%s'\n", name );
        goto over;
    }
    
    if ( _rc == HT_EXISTS ) {
        struct hashTableEntry *entry = NULL;
        struct LatLongList *lll = NULL;
        size_t i = 0, j = 0, realSize = 0;;

        if ( ( lll = (struct LatLongList *) hashTable_find_entry_value ( ngrams_locations, name ) ) == NULL ) {
            goto over;
        }

        if ( ( lll->lat_lon = realloc ( lll->lat_lon, ( lll->size + lat_lon_list->size ) * sizeof ( struct LatLong ) ) ) == NULL ) {
            goto over;
        }

        realSize = lll->size;

        for ( i = 0; i < lat_lon_list->size; i++ ) {
            char exists = 0;

            for ( j = 0; j < realSize; j++ ) {
                if ( lll->lat_lon[j].lat == lat_lon_list->lat_lon[i].lat && lll->lat_lon[j].lon == lat_lon_list->lat_lon[i].lon ) {
                    exists = 1;
                    break;
                }
            }

            if ( ! exists ) {
                lll->lat_lon[realSize].lat = lat_lon_list->lat_lon[i].lat;
                lll->lat_lon[realSize].lon = lat_lon_list->lat_lon[i].lon;
                realSize++;
            }
        }

        if ( ( lll->lat_lon = realloc ( lll->lat_lon, realSize * sizeof ( struct LatLong ) ) ) == NULL ) {
            goto over;
        }

        //printf ( "Existing entry %s expanded from %d to %d\n", name, (int) lll->size, (int)( lll->size + lat_lon_list->size ) );

        lll->size = realSize;
        free ( lat_lon_list->lat_lon );
        free ( lat_lon_list );
    }

    //printf ( "Entry %s added successfully\n", name );

    rc = EXIT_SUCCESS;
over:
    if ( rc == EXIT_FAILURE && lat_lon_list ) {
        free ( lat_lon_list->lat_lon );
        free ( lat_lon_list );
    }

    return rc;
}

int flatten_lat_lon ( void *hashTable_value, hashTable_t *intermediate, struct LatLongList *lat_lon_list ) {
    if ( ( (enum itemType *) hashTable_value )[0] == ITEM_NODE ) {
        struct Node *node = (struct Node *) hashTable_value;

        if ( ( lat_lon_list->lat_lon = realloc ( lat_lon_list->lat_lon, ( lat_lon_list->size + 1 ) * sizeof ( struct LatLong ) ) ) == NULL ) {
            return EXIT_FAILURE;
        }

        lat_lon_list->lat_lon[lat_lon_list->size].lat = node->lat;
        lat_lon_list->lat_lon[lat_lon_list->size].lon = node->lon;
        lat_lon_list->size++;
    } else if ( ( (enum itemType *) hashTable_value )[0] == ITEM_WAY ) {
        struct Way *way = (struct Way *) hashTable_value;
        size_t i = 0;

        for ( i = 0; i < way->size; i++ ) {
            void *value = NULL;

            if ( ( value = hashTable_find_entry_value ( intermediate, way->node_refs[i] ) ) ) {
                if ( flatten_lat_lon ( value, intermediate, lat_lon_list ) == EXIT_FAILURE ) {
                    fprintf ( stderr, "Failed to flatten node entry in way: %s\n", way->node_refs[i] );
                }
            }
        }
    } else if ( ( (enum itemType *) hashTable_value )[0] == ITEM_RELATION ) {
        struct Relation *relation = (struct Relation *) hashTable_value;
        size_t i = 0;

        for ( i = 0; i < relation->size; i++ ) {
            void *value = NULL;

            if ( ( value = hashTable_find_entry_value ( intermediate, relation->member_refs[i] ) ) ) {
                if ( flatten_lat_lon ( value, intermediate, lat_lon_list ) == EXIT_FAILURE ) {
                    fprintf ( stderr, "Failed to flatten member entry in relation: %s\n", relation->member_refs[i] );
                }
            }
        }
    } else {
        fprintf ( stderr, "Unknown relation member type: %d\n", ( (enum itemType *) hashTable_value )[0] );
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int flatten_into_hashTable ( hashTable_t *ngrams_locations, const char *ngram, void *hashTable_value, hashTable_t *intermediate ) {
    struct LatLongList *lat_lon_list = NULL;

    if ( ( lat_lon_list = calloc ( 1, sizeof ( struct LatLongList ) ) ) == NULL )
        return EXIT_FAILURE;

    if ( flatten_lat_lon ( hashTable_value, intermediate, lat_lon_list ) == EXIT_FAILURE ) {
        if ( lat_lon_list ) {
            if ( lat_lon_list->lat_lon )
                free ( lat_lon_list->lat_lon );
            free ( lat_lon_list );
        }
        return EXIT_FAILURE;
    }

    lat_lon_list->ngram = strndup ( ngram, strlen ( ngram ) );

    return add_lat_lon_list_to_hashTable ( ngram, lat_lon_list, ngrams_locations );
}

hashTable_t *populate_ngrams_locations ( hashTable_t *intermediate ) {
    hashTable_t *ngrams_locations = NULL;
    size_t i = 0;

    if ( ( ngrams_locations = calloc ( 1, sizeof ( hashTable_t ) ) ) == NULL ) {
        fprintf ( stderr, "Cannot allocate memory for ngrams -> locations hashTable\n" );
        return NULL;
    }

    for ( i = 0; i < intermediate->size; i++ ) {
        hashTable_entry_t *entry = intermediate->entries[i];

        while ( entry ) {
            if ( ( (enum itemType *) entry->value )[0] == ITEM_NODE ) {
                struct Node *node = (struct Node *) entry->value;

                if ( node->name ) {
                    flatten_into_hashTable ( ngrams_locations, (const char*) node->name, entry->value, intermediate );
                }
            } else if ( ( (enum itemType *) entry->value )[0] == ITEM_WAY ) {
                struct Way *way = (struct Way *) entry->value;

                if ( way->name ) {
                    flatten_into_hashTable ( ngrams_locations, (const char*) way->name, entry->value, intermediate );
                }
            } else if ( ( (enum itemType *) entry->value )[0] == ITEM_RELATION ) {
                struct Relation *relation = (struct Relation *) entry->value;

                if ( relation->name ) {
                    flatten_into_hashTable ( ngrams_locations, (const char*) relation->name, entry->value, intermediate );
                }
            } else {
                fprintf ( stderr, "Unknown type: %d\n", (enum itemType) entry->value );
            }

            entry = entry->next;
        }
    }

    return ngrams_locations;
}

int serialise_ngrams_locations ( hashTable_t *ngrams_locations, const char *filename ) {
    int rc = EXIT_FAILURE;
    FILE *fp = NULL;
    size_t i = 0, j = 0;

    if ( ( fp = fopen ( filename, "w" ) ) == NULL ) {
        fprintf ( stderr, "Cannot open file %s for writing\n", filename );
        goto over;
    }

    if ( fwrite ( &ngrams_locations->size, sizeof ( size_t ), 1, fp ) != 1 ) {
        fprintf ( stderr, "Cannot write hash table size to file\n" );
        goto over;
    }

    for ( i = 0; i < ngrams_locations->size; i++ ) {
        hashTable_entry_t *entry = ngrams_locations->entries[i];
        struct LatLongList *lll = (struct LatLongList *) entry->value;

        if ( fwrite ( &entry->k, sizeof ( uint32_t ), 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot write entry %d key to file\n", (int) i );
            goto over;
        }

        if ( fwrite ( &lll->size, sizeof ( size_t ), 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot write entry %d size to file\n", (int) i );
            goto over;
        }

        for ( j = 0; j < lll->size; j++ ) {
            if ( fwrite ( &lll->lat_lon[j].lat, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot write entry %d point %d latitude to file\n", (int) i, (int) j );
                goto over;
            }

            if ( fwrite ( &lll->lat_lon[j].lon, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot write entry %d point %d longitude to file\n", (int) i, (int) j );
                goto over;
            }
        }
    }

    rc = EXIT_SUCCESS;
over:
    if ( fp )
        fclose ( fp );

    return rc;
}

struct hashTable *deserialise_ngrams_locations ( const char *filename ) {
    struct hashTable *ngrams_locations = NULL, *_ht = NULL;
    FILE *fp = NULL;
    size_t i = 0, j = 0, tableSize = 0;

    if ( ( _ht = calloc ( 1, sizeof ( struct hashTable ) ) ) == NULL ) {
        fprintf ( stderr, "Cannot allocate memory for ngrams -> locations hashTable\n" );
        goto over;
    }

    if ( ( fp = fopen ( filename, "r" ) ) == NULL ) {
        fprintf ( stderr, "Cannot open file %s for writing\n", filename );
        goto over;
    }

    if ( fread ( &tableSize, sizeof ( size_t ), 1, fp ) != 1 ) {
        fprintf ( stderr, "Cannot read hash table size from file\n" );
        goto over;
    }

    _ht->size = tableSize;

    if ( ( _ht->entries = calloc ( 1, _ht->size * sizeof ( struct hashTableEntry ) ) ) == NULL ) {
        fprintf ( stderr, "Cannot allocate memory for ngrams -> locations hashTable entries\n" );
        goto over;
    }

    for ( i = 0; i < _ht->size; i++ ) {
        struct LatLongList *lll = NULL;
        size_t listSize = 0;
        uint32_t key = 0;

        if ( ( lll = calloc ( 1, sizeof ( struct LatLongList ) ) ) == NULL ) {
            fprintf ( stderr, "Cannot allocate memory for entry %d locations list envelope\n", (int) i );
            goto over;
        }

        if ( fread ( &key, sizeof ( uint32_t ), 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot read entry %d key from file\n", (int) i );
            free ( lll );
            goto over;
        }

        if ( fread ( &listSize, sizeof ( size_t ), 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot read entry %d size from file\n", (int) i );
            free ( lll );
            goto over;
        }

        lll->size = listSize;

        if ( ( lll->lat_lon = calloc ( 1, lll->size * sizeof ( struct LatLong ) ) ) == NULL ) {
            fprintf ( stderr, "Cannot allocate memory for entry %d locations list\n", (int) i );
            free ( lll );
            goto over;
        }

        for ( j = 0; j < lll->size; j++ ) {
            long double lat = 0.0, lon = 0.0;

            if ( fread ( &lat, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot read entry %d point %d latitude from file\n", (int) i, (int) j );
                free ( lll->lat_lon );
                free ( lll );
                goto over;
            }

            if ( fread ( &lon, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot read entry %d point %d longitude from file\n", (int) i, (int) j );
                free ( lll->lat_lon );
                free ( lll );
                goto over;
            }

            lll->lat_lon[j].lat = lat;
            lll->lat_lon[j].lon = lon;
        }

        _ht->entries[i].k = key;
        _ht->entries[i].v = (void *) lll;
    }

    ngrams_locations = _ht;
over:
    if ( _ht && _ht != ngrams_locations )
        hashTable_free ( _ht );

    if ( fp )
        fclose ( fp );

    return ngrams_locations;
}
