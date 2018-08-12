#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <parseMap.h>
#include <hashTableO1.h>

#include "populateIntermediate.h"

static size_t unique_items = 0;

struct El *get_significant_el ( struct ParserCtx *parser_ctx ) {
    size_t i = 0;

    if ( parser_ctx->size )
        for ( i = parser_ctx->size; i; i-- )
            if ( parser_ctx->els[i-1].id )
                return &parser_ctx->els[i-1];

    return NULL;

}

void *prepare_for_hashTable ( struct El *sig_el ) {
    void *rc = NULL;

    if ( ! strncmp ( sig_el->el, "node", 4 ) ) {
        struct Node *node = calloc ( 1, sizeof ( struct Node ) );
        node->type = ITEM_NODE;
        node->lat = strtold ( sig_el->lat, NULL );
        node->lon = strtold ( sig_el->lon, NULL );
        rc = (void *) node;
    } else if ( ! strncmp ( sig_el->el, "way", 3 ) ) {
        struct Way *way = calloc ( 1, sizeof ( struct Way ) );
        way->type = ITEM_WAY;
        rc = (void *) way;
    } else if ( ! strncmp ( sig_el->el, "relation", 8 ) ) {
        struct Relation *rel = calloc ( 1, sizeof ( struct Relation ) );
        rel->type = ITEM_RELATION;
        rc = (void *) rel;
    } else {
        fprintf ( stderr, "Not a known significant node: %s\n", sig_el->el );
    }

    return rc;
}

void toLowerCase ( char *str ) {
    size_t i = 0;

    for ( i = 0; i < strlen ( str ); i++ )
        str[i] = (char) tolower ( (int) str[i] );
}

int hydrate_node ( struct Node *node, struct El *curr_el ) {
    if ( curr_el->name && node->name == NULL ) {
        node->name = strndup ( curr_el->name, strlen ( curr_el->name ) );
        toLowerCase ( node->name );
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int hydrate_way ( struct Way *way, struct El *curr_el ) {
    if ( curr_el->name && way->name == NULL ) {
        way->name = strndup ( curr_el->name, strlen ( curr_el->name ) );
        toLowerCase ( way->name );
        return EXIT_SUCCESS;
    }

    if ( curr_el->ref ) {
        way->size++;
        way->node_refs = realloc ( way->node_refs, way->size * sizeof ( char * ) );
        way->node_refs[way->size - 1] = strndup ( curr_el->ref, strlen ( curr_el->ref ) );
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int hydrate_relation ( struct Relation *rel, struct El *curr_el ) {
    if ( curr_el->name && rel->name == NULL ) {
        rel->name = strndup ( curr_el->name, strlen ( curr_el->name ) );
        toLowerCase ( rel->name );
        return EXIT_SUCCESS;
    }

    if ( curr_el->ref ) {
        rel->size++;
        rel->member_refs = realloc ( rel->member_refs, rel->size * sizeof ( char * ) );
        rel->member_refs[rel->size - 1] = strndup ( curr_el->ref, strlen ( curr_el->ref ) );
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

void element_handler ( void *user_data, struct ParserCtx *parser_ctx, char tag_open ) {
    hashTable_t *ht = (hashTable_t *) user_data;
    struct El *sig_el = NULL, *curr_el = NULL;
    void *prepared_item = NULL, *value = NULL;
    int rc = HT_FAILURE;

    if ( tag_open ) {
        curr_el = &parser_ctx->els[parser_ctx->size - 1];

        if ( ( sig_el = get_significant_el ( parser_ctx ) ) ) {
            if ( ( value = hashTable_find_entry_value ( ht, sig_el->id ) ) ) {
                if ( ( (enum itemType *) value )[0] == ITEM_NODE ) {
                    hydrate_node ( (struct Node *) value, curr_el );
                } else if ( ( (enum itemType *) value )[0] == ITEM_WAY ) {
                    hydrate_way ( (struct Way *) value, curr_el );
                } else if ( ( (enum itemType *) value )[0] == ITEM_RELATION ) {
                    hydrate_relation ( (struct Relation *) value, curr_el );
                } else {
                    fprintf ( stderr, "Unknown type: %d\n", (enum itemType) value );
                }
            } else {
                if ( ( prepared_item = prepare_for_hashTable ( sig_el ) ) ) {
                    if ( ( rc = hashTable_add_entry ( ht, sig_el->id, prepared_item ) ) == HT_FAILURE ) {
                        fprintf ( stderr, "Cannot add entry: %s\n", sig_el->id );
                        free ( prepared_item );
                    }
                    
                    if ( rc == HT_EXISTS ) {
                        fprintf ( stderr, "Entry already exists: %s\n", sig_el->id );
                        free ( prepared_item );
                    } else {
                        unique_items++;
                        if ( unique_items % 1000000 == 0 )
                            printf ( "Intermediate: %d\n", (int) unique_items );
                    }
                }
            }
        }
    }
}
