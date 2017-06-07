#ifndef AVL_TREE_H
#define AVL_TREE_H
#include "include.h"
#include "veb.h"
#pragma pack(push)
#pragma pack(1)
#define NEXT_COUNT (((C_SIZE + 1) >> 1))
#define KEYS_COUNT ((C_SIZE) - (NEXT_COUNT))
#define EXIST_COUNT (ROUND_UP(C_SIZE, 8)/8)

struct __ALIGN(CACHE_LINE_SIZE) BinaryPage
{
    __ALIGN(CACHE_LINE_SIZE) byte exist[EXIST_COUNT];
    __ALIGN(CACHE_LINE_SIZE) Key keys_and_next[C_SIZE];
    __ALIGN(CACHE_LINE_SIZE) Value values[KEYS_COUNT];
    uint8_t size;
/*
    uint8_t     parent_bfs;
    BinaryPage* parent_ptr;*/
};

struct BinaryTree
{
    BinaryPage *root;
    Veb tree;
    uint8_t c_height;
    uint64_t items_inserted;
};

#pragma pack(pop)

void binarytree_init(BinaryTree *tree);
void binarytree_shutdown(BinaryTree* tree);
bool binarytree_insert(BinaryTree *tree, Key key, Value value);
bool binarytree_delete(BinaryTree *tree, Key key);
bool binarytree_search(BinaryTree *tree, Key key, Value **value);
void binarytree_inorder(BinaryTree *tree, void (*func)(BinaryPage *page, int iterator, int left_son, int right_son));
void binarytree_graphviz_output(BinaryTree *tree, FILE *out);

#endif // AVL_TREE_H
