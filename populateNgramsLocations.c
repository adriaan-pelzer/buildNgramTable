#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <hashTableO1.h>

#include "populateIntermediate.h"
#include "populateNgramsLocations.h"

typedef struct ref_list {
    const char *ref;
    struct ref_list *next;
} ref_list_t;

static size_t unique_ngrams = 0;

int add_lat_lon_list_to_hashTable ( const char *name, LatLongList_t* lat_lon_list, hashTable_t *ngrams_locations ) {
    int rc = EXIT_FAILURE;
    int _rc = HT_FAILURE;

    if ( ( _rc = hashTable_add_entry ( ngrams_locations, name, (void *) lat_lon_list ) ) == HT_FAILURE ) {
        fprintf ( stderr, "Cannot add entry '%s'\n", name );
        goto over;
    }
    
    if ( _rc == HT_EXISTS ) {
        LatLongList_t *lll = NULL;
        size_t i = 0, j = 0, realSize = 0;;

        if ( ( lll = (LatLongList_t *) hashTable_find_entry_value ( ngrams_locations, name ) ) == NULL ) {
            goto over;
        }

        if ( ( lll->lat_lon = realloc ( lll->lat_lon, ( lll->size + lat_lon_list->size ) * sizeof ( LatLong_t ) ) ) == NULL ) {
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

        if ( ( lll->lat_lon = realloc ( lll->lat_lon, realSize * sizeof ( LatLong_t ) ) ) == NULL ) {
            goto over;
        }

        //printf ( "Existing entry %s expanded from %d to %d\n", name, (int) lll->size, (int)( lll->size + lat_lon_list->size ) );

        lll->size = realSize;
        free ( lat_lon_list->lat_lon );
        free ( lat_lon_list );
    } else {
        unique_ngrams++;
        if ( unique_ngrams % 10000 == 0 )
            printf ( "%d\n", (int) unique_ngrams );
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

char is_in_list ( ref_list_t *ref_list, const char *ref ) {
    ref_list_t *i = ref_list;

    while ( i ) {
        if ( ! strncmp ( ref, i->ref, strlen ( ref ) ) && strlen ( ref ) == strlen ( i->ref ) )
            return 1;
        i = i->next;
    }

    return 0;
}

ref_list_t *add_to_list ( ref_list_t *ref_list, const char *ref ) {
    ref_list_t *rc = calloc ( 1, sizeof ( ref_list_t ) );

    if ( rc ) {
        rc->ref = strdup ( ref );
        rc->next = ref_list;
    }

    return rc;
}

ref_list_t *remove_from_list ( ref_list_t *ref_list, const char *ref ) {
    ref_list_t *rc = ref_list, *prev = NULL, *next = NULL;

    while ( rc ) {
        next = rc->next;
        if ( ! strncmp ( ref, rc->ref, strlen ( ref ) ) ) {
            free ( rc );
            if ( prev ) {
                prev->next = next;
                return ref_list;
            }
            return next; 
        }
        rc = next;
    }

    return ref_list;
}

void ref_list_free ( ref_list_t *ref_list ) {
    ref_list_t *next = NULL, *i = ref_list;

    while ( i ) {
        next = i->next;
        free ( i );
        i = next;
    }
}

int flatten_lat_lon ( void *hashTable_value, hashTable_t *intermediate, LatLongList_t *lat_lon_list, ref_list_t *ref_list ) {
    int rc = EXIT_FAILURE;

    if ( ( (enum itemType *) hashTable_value )[0] == ITEM_NODE ) {
        Node_t *node = (Node_t *) hashTable_value;

        if ( ( lat_lon_list->lat_lon = realloc ( lat_lon_list->lat_lon, ( lat_lon_list->size + 1 ) * sizeof ( LatLong_t ) ) ) == NULL )
            goto over;

        lat_lon_list->lat_lon[lat_lon_list->size].lat = node->lat;
        lat_lon_list->lat_lon[lat_lon_list->size].lon = node->lon;
        lat_lon_list->size++;
    } else if ( ( (enum itemType *) hashTable_value )[0] == ITEM_WAY ) {
        Way_t *way = (Way_t *) hashTable_value;
        size_t i = 0;

        for ( i = 0; i < way->size; i++ ) {
            if ( ! is_in_list ( ref_list, way->node_refs[i] ) ) {
                void *value = NULL;

                if ( ( value = hashTable_find_entry_value ( intermediate, way->node_refs[i] ) ) ) {
                    if ( flatten_lat_lon ( value, intermediate, lat_lon_list, add_to_list ( ref_list, way->node_refs[i] ) ) == EXIT_FAILURE )
                        fprintf ( stderr, "Failed to flatten node entry in way: %s\n", way->node_refs[i] );
                    else
                        remove_from_list ( ref_list, way->node_refs[i] );
                }
            }
        }
    } else if ( ( (enum itemType *) hashTable_value )[0] == ITEM_RELATION ) {
        Relation_t *relation = (Relation_t *) hashTable_value;
        size_t i = 0;

        for ( i = 0; i < relation->size; i++ ) {
            if ( ! is_in_list ( ref_list, relation->member_refs[i] ) ) {
                void *value = NULL;

                if ( ( value = hashTable_find_entry_value ( intermediate, relation->member_refs[i] ) ) ) {
                    if ( flatten_lat_lon ( value, intermediate, lat_lon_list, add_to_list ( ref_list, relation->member_refs[i] ) ) == EXIT_FAILURE )
                        fprintf ( stderr, "Failed to flatten member entry in relation: %s\n", relation->member_refs[i] );
                    else
                        remove_from_list ( ref_list, relation->member_refs[i] );
                }
            }
        }
    } else {
        fprintf ( stderr, "Unknown relation member type: %d\n", ( (enum itemType *) hashTable_value )[0] );
        goto over;
    }

    rc = EXIT_SUCCESS;
over:
    //ref_list_free ( ref_list );
    return rc;
}

int flatten_into_hashTable ( hashTable_t *ngrams_locations, const char *ngram, void *hashTable_value, hashTable_t *intermediate ) {
    LatLongList_t *lat_lon_list = NULL;

    if ( ( lat_lon_list = calloc ( 1, sizeof ( LatLongList_t ) ) ) == NULL )
        return EXIT_FAILURE;

    if ( flatten_lat_lon ( hashTable_value, intermediate, lat_lon_list, NULL ) == EXIT_FAILURE ) {
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

    if ( ( ngrams_locations = hashTable_create ( NGRAMS_LOCATIONS_HT_SIZE ) ) == NULL ) {
        fprintf ( stderr, "Cannot create ngrams -> locations hashTable\n" );
        return NULL;
    }

    for ( i = 0; i < intermediate->size; i++ ) {
        hashTable_entry_t *entry = intermediate->entries[i];

        while ( entry ) {
            if ( ( (enum itemType *) entry->value )[0] == ITEM_NODE ) {
                Node_t *node = (Node_t *) entry->value;

                if ( node->name )
                    flatten_into_hashTable ( ngrams_locations, (const char*) node->name, entry->value, intermediate );
            } else if ( ( (enum itemType *) entry->value )[0] == ITEM_WAY ) {
                Way_t *way = (Way_t *) entry->value;

                if ( way->name )
                    flatten_into_hashTable ( ngrams_locations, (const char*) way->name, entry->value, intermediate );
            } else if ( ( (enum itemType *) entry->value )[0] == ITEM_RELATION ) {
                Relation_t *relation = (Relation_t *) entry->value;

                if ( relation->name )
                    flatten_into_hashTable ( ngrams_locations, (const char*) relation->name, entry->value, intermediate );
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

        while ( entry ) {
            LatLongList_t *lll = (LatLongList_t *) entry->value;
            size_t key_len = strlen ( entry->key );

            if ( fwrite ( &key_len, sizeof ( size_t ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot write entry %d key length (%d) to file\n", (int) i, (int) key_len );
                goto over;
            }

            if ( fwrite ( entry->key, strlen ( entry->key ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot write entry %d key (%s) to file\n", (int) i, entry->key );
                goto over;
            }

            if ( fwrite ( &lll->size, sizeof ( size_t ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot write entry %d size (%d) to file\n", (int) i, (int) lll->size );
                goto over;
            }

            for ( j = 0; j < lll->size; j++ ) {
                if ( fwrite ( &lll->lat_lon[j].lat, sizeof ( long double ), 1, fp ) != 1 ) {
                    fprintf ( stderr, "Cannot write entry %d point %d latitude (%lf) to file\n", (int) i, (int) j, (double) lll->lat_lon[j].lat );
                    goto over;
                }

                if ( fwrite ( &lll->lat_lon[j].lon, sizeof ( long double ), 1, fp ) != 1 ) {
                    fprintf ( stderr, "Cannot write entry %d point %d longitude (%lf) to file\n", (int) i, (int) j, (double) lll->lat_lon[j].lon );
                    goto over;
                }
            }

            entry = entry->next;
        }
    }

    rc = EXIT_SUCCESS;
over:
    if ( fp )
        fclose ( fp );

    return rc;
}

hashTable_t *deserialise_ngrams_locations ( const char *filename ) {
    hashTable_t *ngrams_locations = NULL, *_ht = NULL;
    LatLongList_t *lat_lon_list = NULL;
    FILE *fp = NULL;
    size_t i = 0, tableSize = 0;

    if ( ( fp = fopen ( filename, "r" ) ) == NULL ) {
        fprintf ( stderr, "Cannot open file %s for writing\n", filename );
        goto over;
    }

    if ( fread ( &tableSize, sizeof ( size_t ), 1, fp ) != 1 ) {
        fprintf ( stderr, "Cannot read hash table size from file\n" );
        goto over;
    }

    if ( ( _ht = hashTable_create ( tableSize ) ) == NULL ) {
        fprintf ( stderr, "Cannot create ngrams -> locations hashTable\n" );
        goto over;
    }

    while ( ! feof ( fp ) ) {
        char *key = NULL;
        size_t key_len = 0;
        hashTable_rc_t hashTable_rc = HT_FAILURE;

        if ( fread ( &key_len, sizeof ( size_t ), 1, fp ) != 1 ) {
            if ( feof ( fp ) )
                break;
            fprintf ( stderr, "Cannot read entry key length from file\n" );
            goto over;
        }

        if ( ( key = calloc ( 1, key_len + 1 ) ) == NULL ) {
            fprintf ( stderr, "Cannot allocate memory for key\n" );
            goto over;
        }

        if ( fread ( key, key_len, 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot read entry key from file\n" );
            goto over;
        }

        if ( ( lat_lon_list = calloc ( 1, sizeof ( LatLongList_t ) ) ) == NULL ) {
            fprintf ( stderr, "Cannot allocate memory for lat/lon list\n" );
            goto over;
        }

        if ( fread ( &lat_lon_list->size, sizeof ( size_t ), 1, fp ) != 1 ) {
            fprintf ( stderr, "Cannot read lat/lon list size from file\n" );
            goto over;
        }

        if ( ( lat_lon_list->lat_lon = calloc ( 1, lat_lon_list->size * sizeof ( LatLong_t ) ) ) == NULL ) {
            fprintf ( stderr, "Cannot allocate memory for lat/lon array: %s\n", strerror ( errno ) );
            goto over;
        }

        for ( i = 0; i < lat_lon_list->size; i++ ) {
            if ( fread ( &lat_lon_list->lat_lon[i].lat, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot read entry point %d latitude from file\n", (int) i );
                goto over;
            }

            if ( fread ( &lat_lon_list->lat_lon[i].lon, sizeof ( long double ), 1, fp ) != 1 ) {
                fprintf ( stderr, "Cannot read entry point %d longitude from file\n", (int) i );
                goto over;
            }
        }

        if ( ( hashTable_rc = hashTable_add_entry ( _ht, (const char *) key, (void *) lat_lon_list ) ) != HT_SUCCESS ) {
            fprintf ( stderr, "Cannot add entry to hash table: %d\n", (int) hashTable_rc );
            goto over;
        }

        lat_lon_list = NULL;
        free ( key );
    }

    ngrams_locations = _ht;
over:
    if ( lat_lon_list ) {
        if ( lat_lon_list->lat_lon )
            free ( lat_lon_list->lat_lon );
        free ( lat_lon_list );
    }

    if ( _ht && _ht != ngrams_locations )
        hashTable_free ( _ht );

    if ( fp )
        fclose ( fp );

    return ngrams_locations;
}
