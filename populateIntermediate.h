#include <stdlib.h>
#include <parseMap.h>

#ifndef _POPULATE_INTERMEDIATE_H_
#define _POPULATE_INTERMEDIATE_H_

enum itemType {
    ITEM_NODE,
    ITEM_WAY,
    ITEM_RELATION
};

struct Node {
    enum itemType type;
    long double lat;
    long double lon;
    char *name;
};

struct Way {
    enum itemType type;
    size_t size;
    char **node_refs;
    char *name;
};

struct Relation {
    enum itemType type;
    size_t size;
    char **member_refs;
    char *name;
};

void element_handler ( void *user_data, struct ParserCtx *parser_ctx, char tag_open );

#endif
