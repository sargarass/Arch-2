#ifndef AVL_TREE_NOCACHE_H
#define AVL_TREE_NOCACHE_H
#include "include.h"

typedef struct NCLeaf NCLeaf;

struct NCLeaf {
    Key key;
    Value value;
    NCLeaf *parent;
    NCLeaf *right;
    NCLeaf *left;
    int balance;
};

typedef struct
{
    uint64_t height;
    NCLeaf *root;
} NCTree;

void   nc_init(NCTree *tree);
bool   nc_search(NCTree *tree, Key key, Value **value);
bool   nc_insert(NCTree *tree, Key key, Value value);
void   nc_in_order(NCTree *tree, void (*func)(NCLeaf *ptr));
#endif // AVL_TREE_NOCACHE_H
