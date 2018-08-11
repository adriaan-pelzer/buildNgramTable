#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <hashTableO1.h>
#include <ngramsLocations.h>

#include "populateIntermediate.h"

typedef struct ref_list {
    const char *ref;
    struct ref_list *next;
} ref_list_t;

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
    return rc;
}

int flatten_into_hashTable ( ngramsLocations_t *ngrams_locations, const char *ngram, void *hashTable_value, hashTable_t *intermediate ) {
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

    return ngramsLocations_add_lat_lon_list ( ngram, lat_lon_list, ngrams_locations );
}

ngramsLocations_t *populate_ngrams_locations ( hashTable_t *intermediate, size_t size ) {
    ngramsLocations_t *ngrams_locations = NULL;
    size_t i = 0;

    if ( ( ngrams_locations = ngramsLocations_create ( size ) ) == NULL ) {
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
