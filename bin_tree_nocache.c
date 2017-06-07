#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bin_tree_nocache.h"

static uint64_t alloced_memory = 0;
//=============================================//
static inline void *my_alloc(size_t __size)
{
    alloced_memory += __size;
    //return aligned_alloc(CACHE_LINE_SIZE, ROUND_UP(__size, CACHE_LINE_SIZE));
    return malloc(__size);
}

static inline void *my_calloc(size_t __nmemb, size_t __size)
{
    alloced_memory += __nmemb * __size;

    return calloc(__nmemb, __size);
}

static inline void *my_realloc(void* __ptr, size_t __size, size_t __new_size)
{    
    alloced_memory -= __size;
    alloced_memory += __new_size;

    return realloc(__ptr, __new_size);
}

static inline void my_free(void *__ptr, size_t __size)
{
    if (!__ptr)
    {
        return;
    }

    alloced_memory -= __size;
    free(__ptr);
}

#ifdef USE_ALLOCATOR
#define MEM_SIZE 65536
static struct
{
    size_t   array_line;
    size_t   array_pos;
    size_t   array_mem;
    NCLeaf**  array;
    NCLeaf**  stack;
    size_t   stack_mem;
    size_t   stack_size;
} allocator;


__attribute__ ((constructor))
static void allocatorInit()
{
    allocator.array      = my_alloc(10 * sizeof(NCLeaf*));
    allocator.array[0]   = my_calloc(MEM_SIZE , sizeof(NCLeaf));
    allocator.stack      = my_alloc(100 * sizeof(NCLeaf*));
    allocator.array_line = 0;
    allocator.array_pos  = -1;
    allocator.array_mem  = 10;
    allocator.stack_size = 0;
    allocator.stack_mem  = 100;
}

static void* allocatorGetMemory()
{
    if (allocator.stack_size > 0)
    {
        allocator.stack_size--;
        return allocator.stack[allocator.stack_size];
    }

    allocator.array_pos++;
    if (allocator.array_pos >= MEM_SIZE)
    {
        allocator.array_line++;
        if (allocator.array_line >= allocator.array_mem)
        {
            allocator.array = my_realloc(allocator.array, allocator.array_mem * sizeof(NCLeaf*), allocator.array_mem * 2 * sizeof(NCLeaf*));
            allocator.array_mem *= 2;
        }
        allocator.array[allocator.array_line] = my_calloc(MEM_SIZE, sizeof(NCLeaf));
        allocator.array_pos = 0;
    }
    return &allocator.array[allocator.array_line][allocator.array_pos];
}

static void allocatorFree(NCLeaf* tmp)
{
    memset(tmp, 0, sizeof(NCLeaf));
    if (allocator.stack_size >= allocator.stack_mem)
    {
        allocator.stack = realloc(allocator.stack, allocator.stack_mem * 2 * sizeof(NCLeaf*));
        allocator.stack_mem *= 2;
    }
    allocator.stack[allocator.stack_size++] = tmp;
}

__attribute__ ((destructor))
static void* allocatorFreeMemory()
{
    printf("%-30s %1d GB %3d Mb %3d Kb %3d B\n", "No-cache tree alloced memory:", GET_MEMSTAT(alloced_memory));
    int i;
    for (i = 0; i <= allocator.array_line; i++)
    {
        free(allocator.array[i]);
    }
    free(allocator.array);
    free(allocator.stack);
}
#else
__attribute__ ((destructor))
static void my_alloc_output() {
    printf("%-30s %1d GB %3d Mb %3d Kb %3d B\n", "No-cache tree alloced memory:", GET_MEMSTAT(alloced_memory));
}
#endif

//=============================================//

void nc_init(NCTree *tree)
{
    memset(tree, 0, sizeof(NCTree));
}

static inline NCLeaf* nc_alloc_memory()
{
    NCLeaf *tmp = NULL;
#ifdef USE_ALLOCATOR
    tmp = (NCLeaf*)allocatorGetMemory();
#else
    tmp = (NCLeaf*)my_alloc(sizeof(NCLeaf));
#endif

    memset((NCLeaf*)tmp, 0, sizeof(NCLeaf));
    return tmp;
}

static inline NCLeaf* get_right(NCLeaf *x)
{
    return x->right;
}

static inline NCLeaf* get_left(NCLeaf *x)
{
    return x->left;
}

static inline NCLeaf* get_parent(NCLeaf *x)
{
    return x->parent;
}

static inline NCLeaf* set_right(NCLeaf *x, NCLeaf *right)
{
    return x->right = right;
}

static inline NCLeaf* set_left(NCLeaf *x, NCLeaf *left)
{
    return x->left = left;
}

static inline NCLeaf* set_parent(NCLeaf *x, NCLeaf *parent)
{
    return x->parent = parent;
}

static inline Value* get_value(NCLeaf *x)
{
    return x->value;
}
/*
static inline void set_value(NCLeaf *x, Value* value)
{
    x->value = *value;
}*/

bool nc_search(NCTree* tree, Key key, Value **value)
{
    NCLeaf* ptr = tree->root;

    while (ptr != NULL && ptr->key != key)
    {
        if (key < ptr->key)
        {
            ptr = get_left(ptr);
        }
        else
        {
            ptr = get_right(ptr);
        }
    }

    if (!ptr)
    {
        return false;
    }
    *value = &ptr->value;
    return true;
}

static inline int get_balance(NCLeaf *x)
{
    return x->balance;
}

static inline void set_balance(NCLeaf *x, int balance)
{
    x->balance = balance;
}

static inline int inc_balance(NCLeaf* x)
{
    return ++x->balance;
}

static inline int dec_balance(NCLeaf* x)
{
    return --x->balance;
}

static inline bool is_root(NCLeaf* x)
{
    return x->parent == NULL;
}

static inline void rotate_left(NCTree* avltree, NCLeaf* leaf)
{
    NCLeaf *p = leaf;
    NCLeaf *q = get_right(p);
    NCLeaf *parent = get_parent(p);

    if (!is_root(p))
    {
        if (get_left(parent) == p)
        {
            set_left(parent, q);
        }
        else
        {
            set_right(parent, q);
        }
    }
    else
    {
        avltree->root = q;
    }
    set_parent(q, parent);
    set_parent(p, q);
    set_right(p, get_left(q));

    if (get_right(p))
    {
        set_parent(get_right(p), p);
    }

    set_left(q, p);
}

static inline void rotate_right(NCTree* avltree, NCLeaf* leaf)
{
    NCLeaf *p = leaf;
    NCLeaf *q = get_left(p);
    NCLeaf *parent = get_parent(p);

    if (!is_root(p))
    {
        if (get_left(parent) == p)
        {
            set_left(parent, q);
        }
        else
        {
            set_right(parent, q);
        }
    }
    else
    {
        avltree->root = q;
    }
    set_parent(q, parent);
    set_parent(p, q);

    set_left(p, get_right(q));

    if (get_left(p)) {
        set_parent(get_left(p), p);
    }

    set_right(q, p);
}

static inline void set_left_child(NCLeaf* parent, NCLeaf* leaf)
{
    parent->left = leaf;
    leaf->parent = parent;
}

static inline void set_right_child(NCLeaf* parent, NCLeaf* leaf)
{
    parent->right = leaf;
    leaf->parent = parent;
}

static inline void balance(NCTree* avltree, NCLeaf* node,  NCLeaf* unbalanced)
{
    NCLeaf* parent = get_parent(node);

    for (;;)
    {
        if (parent)
        {
            if (get_left(parent) == node)
            {
                dec_balance(parent);
            }
            else
            {
                inc_balance(parent);
            }
        }

        if (parent == unbalanced)
        {
            break;
        }

        node = parent;
        parent = get_parent(parent);
    }

    if (!unbalanced)
    {
        return;
    }

    switch (get_balance(unbalanced)) {
        case 1:
        case -1:
            avltree->height++;
            break;

        case 2: {
            NCLeaf* right = get_right(unbalanced);

            if (get_balance(right) == 1) {
                set_balance(unbalanced, 0);
                set_balance(right, 0);
            } else {
                switch(get_balance(get_left(right))) {
                    case 1:
                        set_balance(unbalanced, -1);
                        set_balance(right, 0);
                        break;
                    case 0:
                        set_balance(unbalanced, 0);
                        set_balance(right, 0);
                        break;
                    case -1:
                        set_balance(unbalanced, 0);
                        set_balance(right, 1);
                        break;
                }

                set_balance(get_left(right), 0);
                rotate_right(avltree, right);


            }
            rotate_left(avltree, unbalanced);


            break;
        }

        case -2: {
            NCLeaf* left = get_left(unbalanced);

            if (get_balance(left) == -1)
            {
                set_balance(unbalanced, 0);
                set_balance(left, 0);
            }
            else
            {
                switch(get_balance(get_right(left)))
                {
                    case 1:
                        set_balance(unbalanced, 0);
                        set_balance(left, -1);
                        break;
                    case 0:
                        set_balance(unbalanced, 0);
                        set_balance(left, 0);
                        break;
                    case -1:
                        set_balance(unbalanced, 1);
                        set_balance(left, 0);
                        break;
                }

                set_balance(get_right(left), 0);
                rotate_left(avltree, left);
            }
            rotate_right(avltree, unbalanced);

            break;
        }
    }
}

bool nc_insert(NCTree* tree, Key key, Value value)
{
    NCLeaf* leaf = nc_alloc_memory();
    leaf->key = key;
    leaf->value = value;

    if (!tree->root)
    {
        tree->root = leaf;
        leaf->parent = NULL;

        return true;
    }
    NCLeaf* unbalanced = NULL;

    NCLeaf* x = tree->root;
    for(;;)
    {
        if (get_balance(x) != 0)
        {
            unbalanced = x;
        }

        if (x->key == key)
        {
            return false;
        }
        else
        {
            if (key < x->key)
            {
                if (get_left(x) == NULL)
                {
                    set_left(x, leaf);
                    set_parent(leaf, x);
                    break;
                }
                x = get_left(x);
            }
            else if (key > x->key)
            {
                if (x->right == NULL)
                {
                    set_right(x, leaf);
                    set_parent(leaf, x);
                    break;
                }
                x = get_right(x);
            }
        }
    }
    balance(tree, leaf, unbalanced);
    return true;
}

static void NCTreeInOrderRec(NCLeaf* ptr, void (*func)(NCLeaf* leaf))
{
    if (!ptr) {
        return;
    }

    NCTreeInOrderRec(ptr->left, func);
    func(ptr);
    NCTreeInOrderRec(ptr->right, func);

}

void nc_in_order(NCTree* bintree, void (*func)(NCLeaf* ptr))
{
    NCTreeInOrderRec(bintree->root, func);
}
