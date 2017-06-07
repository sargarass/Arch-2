#include <bin_tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static uint8_t bfs_convert[C_SIZE];
#define pow2(n) \
    (1<<(n))

#define BFS_CONVERT(x) (bfs_convert[x])
//#define INORDER_TO_BFS(depth, x) (inorder_convert_to_bfs[depth][x])
#define INORDER_CONVERT(depth, x) (BFS_CONVERT(INORDER_TO_BFS(depth, x)))

#define BFS_RIGHT_CONVERT(x) (BFS_CONVERT(BFS_RIGHT(x)))
#define BFS_LEFT_CONVERT(x)  (BFS_CONVERT(BFS_LEFT(x)))

static uint64_t alloced_memory = 0 ;
static uint64_t items_inserted = 0;
static bool can_be_inserted = true;
//*****************************************************
static void* my_alloc(size_t __size)
{
    /*if (alloced_memory > 6ull*1024ull*1024ull*1024ull+200*1024ull*1024ull*1024ull) {
        can_be_inserted = false;
    }*/
    alloced_memory += ROUND_UP(__size, CACHE_LINE_SIZE);
    return aligned_alloc(CACHE_LINE_SIZE, ROUND_UP(__size, CACHE_LINE_SIZE));
}

static void* my_realloc(void* ptr, size_t __size, size_t new_size)
{
    alloced_memory -= ROUND_UP(__size, CACHE_LINE_SIZE);;
    void* _new = my_alloc(new_size);
    memcpy(_new, ptr, __size);
    free(ptr);

    return _new;
}

static inline void my_free(void *__ptr, size_t __size)
{
    if (!__ptr)
    {
        return;
    }

    alloced_memory -= ROUND_UP(__size, CACHE_LINE_SIZE);
    free(__ptr);
}


#ifdef USE_ALLOCATOR
#define MEM_SIZE 65536

static struct
{
    size_t   array_line;
    size_t   array_pos;
    size_t   array_mem;
    BinaryPage** array;
    BinaryPage** stack;
    size_t   stack_mem;
    size_t   stack_size;
} allocator;

__attribute__((constructor))
static void allocatorInit()
{

    allocator.array      = my_alloc(10 * sizeof(BinaryPage*));
    allocator.array[0]   = my_alloc(MEM_SIZE * sizeof(BinaryPage));
    allocator.stack      = my_alloc(100 * sizeof(BinaryPage*));
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
            allocator.array = my_realloc(allocator.array, allocator.array_mem * sizeof(BinaryPage*), allocator.array_mem * 2 * sizeof(BinaryPage*));
            allocator.array_mem *= 2;
        }

        allocator.array[allocator.array_line] = my_alloc(MEM_SIZE * sizeof(BinaryPage));
        allocator.array_pos = 0;
    }

    return &allocator.array[allocator.array_line][allocator.array_pos];
}

static void allocatorFree(BinaryPage* tmp)
{
    memset(tmp, 0, sizeof(BinaryPage));
    if (allocator.stack_size >= allocator.stack_mem)
    {
        allocator.stack = my_realloc(allocator.stack, allocator.stack_mem * sizeof(BinaryPage*), allocator.stack_mem * 2 * sizeof(BinaryPage*));
        allocator.stack_mem *= 2;
    }
    allocator.stack[allocator.stack_size++] = tmp;
}

__attribute__ ((destructor))
static void allocatorFreeMemory()
{
    printf("%-30s %1d GB %3d Mb %3d Kb %3d B\n","Cache-tree alloced memory:", GET_MEMSTAT(alloced_memory));
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
static void my_alloc_output()
{
    printf("Tree size %d\n", items_inserted);
    printf("%-30s %1zu GB %3zu Mb %3zu Kb %3zu B\n","Cache-tree alloced memory:", GET_MEMSTAT(alloced_memory));
    printf("%zu\n", alloced_memory);
}
#endif
//*****************************************************

static inline void bfs_convert_veb(BinaryTree* tree)
{
    tree->tree = vebnew(C_SIZE);
    for (int i = 1; i <= C_SIZE; i++) {
        bfs_convert[i - 1] = vebpos(tree->tree, i) - 1;
    }
}

static inline void bfs_convert_bfs()
{
    for (int i = 0; i < C_SIZE; i++)
    {
        bfs_convert[i] = i;
    }
}

static void bfs_convert_dfs_rec(int64_t bfs, int64_t dfs, int h)
{
    if (bfs >= C_SIZE) return;

    bfs_convert[bfs] = dfs - 1;
    bfs_convert_dfs_rec(BFS_LEFT(bfs), dfs + 1, h - 1);
    bfs_convert_dfs_rec(BFS_RIGHT(bfs),dfs + pow2(h - 1), h - 1);
}

static void bfs_convert_inorder_rec(int64_t bfs, int64_t inorder,int h)
{
    if (bfs >= C_SIZE) return;
    bfs_convert[bfs] = inorder - 1;
    bfs_convert_inorder_rec(BFS_LEFT(bfs),  inorder - pow2(h - 2), h - 1);
    bfs_convert_inorder_rec(BFS_RIGHT(bfs), inorder + pow2(h - 2), h - 1);
}

///////////////////////////////////////////////////////////////////////////////
/*static void inorder_convert_bfs_rec(int64_t bfs, int64_t inorder, int h, int depth)
{
    if (bfs >= ((1 << depth) - 1)) return;
    inorder_convert_to_bfs[depth][inorder - 1] = bfs;
    inorder_convert_bfs_rec(BFS_LEFT(bfs),  inorder - pow2(h - 2), h - 1, depth);
    inorder_convert_bfs_rec(BFS_RIGHT(bfs), inorder + pow2(h - 2), h - 1, depth);
}
*/
/*static inline void inorder_convert_bfs_start(int h)
{
    int new_c  = (1ull << h);
    int64_t inorder_start =  new_c / 2;
    printf("%d\n", new_c);
    inorder_convert_bfs_rec(0, inorder_start, h, h);
}*/
/*
static void inorder_convert_bfs()
{
    for (int d = C_LOG; d > 1; d--) {
        inorder_convert_bfs_start(d);
    }
}
*/
///////////////////////////////////////////////////////////////////////////////

static inline void bfs_convert_inorder()
{
    int h = ilog_2(C_SIZE + 1);

    int64_t inorder_start = (C_SIZE + 1) / 2;
    bfs_convert_inorder_rec(0, inorder_start, h);
}

static inline void bfs_convert_dfs()
{
    bfs_convert_dfs_rec(0, 1, ilog_2(C_SIZE + 1));
}

static inline BinaryPage* binary_page_alloc()
{
    BinaryPage* tmp;
    #ifdef USE_ALLOCATOR
        tmp = allocatorGetMemory();
    #else
        tmp = (BinaryPage*)my_alloc(sizeof(BinaryPage));
    #endif

    memset(tmp->exist,   0, sizeof(uint8_t) * EXIST_COUNT);
    tmp->size = 0;
    //tmp->parent_ptr = NULL;
    return tmp;
}

/// Inilization
void binarytree_init(BinaryTree* tree)
{
    tree->root = binary_page_alloc();
    tree->c_height = ilog_2(KEYS_COUNT + 1);
    items_inserted = 0;
#if defined(USE_VEB)
    bfs_convert_veb(tree);
#elif defined(USE_DFS)
    bfs_convert_dfs();
#elif defined(USE_BFS)
    bfs_convert_bfs();
#elif defined(USE_INORDER)
    bfs_convert_inorder();
#else
    #error USE_BFS or USE_DFS or USE_VEB or USE_INORDER must be defined
#endif
    //inorder_convert_bfs();
    /*for (int d = C_LOG; d > 1; d--) {
        for (int i = 0; i < ((1ull << d)-1); i++) {
            printf("(%d %d) ", i + 1, INORDER_TO_BFS(d, i) + 1);
        }
        printf("\n");
    }
*/
   /* for (int i = 0; i < C_SIZE; i++) {
        printf("bfs %2d convert to %2d\n", i + 1, BFS_CONVERT(i) + 1);
    }*/

    printf("Used layout: %s\n", LAYOUT_STRING);
    printf("C_SIZE: %d\n", C_SIZE);
    printf("KEYS_SIZE: %d\n", KEYS_COUNT);
    printf("NEXT_SIZE: %d\n", NEXT_COUNT);
    printf("EXIST_SIZE: %d\n", EXIST_COUNT);
    printf("CACHE_MISS on exist = %d\n", ROUND_UP(EXIST_COUNT * sizeof(uint8_t) * 8, CACHE_LINE_SIZE)/CACHE_LINE_SIZE);
}

void binarytree_shutdown(BinaryTree* tree)
{
    /*while(IS_EXIST(tree->root,0))
    {
        binarytree_delete(tree, tree->root->keys[0]);
    }*/
}

static inline bool is_key(int bfs_index)
{
    return bfs_index < KEYS_COUNT;
}

static inline bool is_next(int bfs_index)
{
    return !is_key(bfs_index);
}


static inline bool is_exist(BinaryPage *ptr, uint pos)
{
    return (ptr->exist[pos >> 3]) & pow2(pos & 0x7);
}

static inline void set_exist(BinaryPage *ptr, uint pos, bool exist)
{
    ptr->exist[pos >> 3] = ((ptr->exist[pos >> 3]) & ~(pow2(pos & 0x7))) | (exist << (pos & 0x7));
}

static inline void set_key(BinaryPage *ptr, int iterator, Key key)
{
    ptr->keys_and_next[iterator] = key;
}

static inline Key get_key(BinaryPage *ptr, int iterator)
{
    return ptr->keys_and_next[iterator];
}

static inline void set_value(BinaryPage *ptr, int bfs_number, Value value)
{
    ptr->values[bfs_number] = value;
}

static inline Value get_value(BinaryPage *ptr, int bfs_number)
{
    return ptr->values[bfs_number];
}

static inline bool binarytree_lookup(BinaryTree* tree, Key key, uint8_t *bfs, BinaryPage** page)
{
    if (!bfs || !page) {
        return false;
    }

    uint8_t bfs_number, iterator, i;
    BinaryPage *ptr = tree->root;

    for (;;)
    {
        bfs_number = 0;
        iterator = BFS_CONVERT(bfs_number);

        for (i = 0; i < tree->c_height; i++)
        {
            if ( !is_exist(ptr, iterator) )
            {
                *page = ptr;
                *bfs = bfs_number;
                return false;
            }

            if (get_key(ptr, iterator) > key)
            {
                bfs_number = BFS_LEFT(bfs_number);
            }
            else if (get_key(ptr, iterator) < key)
            {
                bfs_number = BFS_RIGHT(bfs_number);
            }
            else // found
            {
                *page = ptr;
                *bfs = bfs_number;
                return true;
            }

            iterator = BFS_CONVERT(bfs_number);
        }

        if ( !is_exist(ptr, iterator) )
        {
            *page = ptr;
            *bfs = bfs_number;
            return false;
        }
        ptr = (BinaryPage*)ptr->keys_and_next[iterator];
    }
}

/// CLUSTER SORT
#pragma pack(push)
#pragma pack(1)
typedef struct {
    Key key;
    Value value;
} element_t;

struct {
    element_t data[KEYS_COUNT];
    int iterator;
} stack;

struct {
    int data[10*KEYS_COUNT];
    int iterator;
} prog_stack;

#pragma pack(pop)

static inline int prog_stack_top()
{
    //assert(prog_stack.iterator > 0);
    return prog_stack.data[prog_stack.iterator - 1];
}

static inline void prog_stack_push(int x)
{
    prog_stack.data[prog_stack.iterator] = x;
    prog_stack.iterator++;
}

static inline void prog_stack_pop()
{
   // assert(prog_stack.iterator > 0);
    prog_stack.iterator--;
}

static inline void prog_stack_init()
{
    prog_stack.iterator = 0;
}

static inline element_t* cluster_stack_top()
{
    //assert(stack.iterator > 0);
    return &stack.data[stack.iterator - 1];
}

static inline void cluster_stack_push(Key key, Value value)
{
    stack.data[stack.iterator].key = key;
    stack.data[stack.iterator].value = value;
    stack.iterator++;
}

static inline void cluster_stack_pop()
{
    //assert(stack.iterator > 0);
    stack.iterator--;
}

static inline void cluster_stack_init()
{
    stack.iterator = 0;
}

static void binarytree_cluster_in_order(BinaryPage* ptr, uint8_t bfs_number, bool* insert, Key key, Value value)
{
    int iterator;
    start:

    iterator = BFS_CONVERT(bfs_number);

    if (is_exist(ptr, BFS_LEFT_CONVERT(bfs_number)) && is_key( BFS_LEFT(bfs_number) ))
    {
        binarytree_cluster_in_order(ptr, BFS_LEFT(bfs_number), insert, key, value);
    }

    if (get_key(ptr,iterator) > key && !(*insert))
    {
        cluster_stack_push(key, value);
        *insert = true;
    }

    cluster_stack_push(get_key(ptr, iterator), get_value(ptr, bfs_number));
    set_exist(ptr, iterator, false);

    if (is_exist(ptr, BFS_RIGHT_CONVERT(bfs_number)) && is_key( BFS_RIGHT(bfs_number) ))
    {
        bfs_number = BFS_RIGHT(bfs_number);
        goto start;
    }
}

static void binarytree_insert_rec(BinaryPage* ptr, int left, int right, uint8_t bfs_number)
{
    int mid;
    uint8_t iterator;

    start:

    mid = (left + right) / 2;
    if (left > right)
    {
        return;
    }

    iterator = BFS_CONVERT(bfs_number);

    set_key(ptr, iterator, stack.data[mid].key);
    set_value(ptr, bfs_number, stack.data[mid].value);
    set_exist(ptr, iterator, true);

    binarytree_insert_rec(ptr, left, mid - 1, BFS_LEFT(bfs_number));

    left = mid + 1;
    bfs_number = BFS_RIGHT(bfs_number);
    goto start;
}

static inline void binarytree_balance_in_cluster(BinaryPage *ptr, Key key, Value value)
{
    cluster_stack_init();
    bool insert = false;
    binarytree_cluster_in_order(ptr, 0, &insert, key, value);
    // если новый ключ - наибольший
    if (!insert)
    {
        cluster_stack_push(key, value);
    }

    ptr->size++;
    binarytree_insert_rec(ptr, 0, stack.iterator - 1, 0);
}
/*
static inline void binarytree_balance_in_cluster2(BinaryPage *ptr, Key key, Value value)
{
    cluster_stack_init();
    prog_stack_init();
    bool insert = false;
    binarytree_cluster_in_order(ptr, 0, &insert, key, value);
   /* prog_stack_push(0);

    while(prog_stack.iterator > 0) {
        int bfs_number = prog_stack_top();
        prog_stack_pop();

        int iterator = BFS_CONVERT(bfs_number);

        if (is_exist(ptr, BFS_LEFT_CONVERT(bfs_number)) && is_key( BFS_LEFT(bfs_number) ))
        {
            prog_stack_push(BFS_LEFT(bfs_number));
        }

        if (get_key(ptr,iterator) > key && !insert)
        {
            cluster_stack_push(key, value);
            insert = true;
        }

        cluster_stack_push(get_key(ptr, iterator), get_value(ptr, bfs_number));
        set_exist(ptr, iterator, false);

        if (is_exist(ptr, BFS_RIGHT_CONVERT(bfs_number)) && is_key( BFS_RIGHT(bfs_number) ))
        {
            prog_stack_push(BFS_RIGHT(bfs_number));
        }
    }*/

    // если новый ключ - наибольший
  /*  if (!insert)
    {
        cluster_stack_push(key, value);
    }
    ptr->size++;

    int mid;
    uint8_t iterator;
    uint8_t bfs_number;

    prog_stack_push(0);
    prog_stack_push(stack.iterator - 1);
    prog_stack_push(0);

    while(prog_stack.iterator > 0) {
        int left = prog_stack_top();
        prog_stack_pop();

        int right = prog_stack_top();
        prog_stack_pop();

        bfs_number = prog_stack_top();
        prog_stack_pop();

        mid = (left + right) / 2;
        if (left > right)
        {
            continue;
        }

        iterator = BFS_CONVERT(bfs_number);

        set_key(ptr, iterator, stack.data[mid].key);
        set_value(ptr, bfs_number, stack.data[mid].value);
        set_exist(ptr, iterator, true);

        prog_stack_push(BFS_LEFT(bfs_number));
        prog_stack_push(mid - 1);
        prog_stack_push(left);

        prog_stack_push(BFS_RIGHT(bfs_number));
        prog_stack_push(right);
        prog_stack_push(mid + 1);
    }
}*/


bool binarytree_insert(BinaryTree* tree, Key key, Value value)
{
    if (!can_be_inserted) {
        return false;
    }

    uint8_t bfs_number = 0;
    BinaryPage* ptr = NULL;

    if (binarytree_lookup(tree, key, &bfs_number, &ptr))
    {
        return false;
    }

    uint8_t iterator = BFS_CONVERT(bfs_number);

    if ( is_next(bfs_number) ) // сортировка кластерная
    {
#ifdef USE_CLUSTER_SORT
        if (ptr->size < KEYS_COUNT)
        {
            binarytree_balance_in_cluster(ptr, key, value);
            items_inserted++;
            return true;
        }
        else
#endif
        {
            ptr->keys_and_next[iterator] = (uintptr_t) binary_page_alloc();
            set_exist(ptr, iterator, true);
            ptr = (BinaryPage*)ptr->keys_and_next[iterator];
            bfs_number = 0;
            iterator = BFS_CONVERT(bfs_number);
        }
    }

    ptr->size++;
    set_key(ptr, iterator, key);
    set_value(ptr, bfs_number, value);
    set_exist(ptr, iterator, true);
    items_inserted++;
    return true;
}

bool binarytree_search(BinaryTree *tree, Key key, Value **value)
{
    uint8_t bfs_number = 0;
    BinaryPage* ptr = NULL;

    bool found = binarytree_lookup(tree, key, &bfs_number, &ptr);

    if (!found)
    {
        value = NULL;
        return false;
    }

    *value = &ptr->values[bfs_number];
    return true;
}

bool binarytree_delete(BinaryTree* tree, Key key)
{

}

/// Debugging tools
static void binarytree_inorder_rec(BinaryPage *ptr, int bfs_number, void (*func)(BinaryPage *page, int iterator, int left_son, int right_son))
{    
    int iterator = BFS_CONVERT(bfs_number);

    if (is_next(bfs_number))
    {

        if (is_exist(ptr, iterator))
        {
            ptr = (BinaryPage*)ptr->keys_and_next[iterator];
        }
        else
        {
            ptr = NULL;
        }

        bfs_number = 0;
        iterator = 0;
    }

    if (!ptr || !is_exist(ptr, iterator))
    {
        return;
    }

    binarytree_inorder_rec(ptr, BFS_LEFT(bfs_number), func);
    func(ptr, iterator, BFS_LEFT_CONVERT(bfs_number), BFS_RIGHT_CONVERT(bfs_number));
    binarytree_inorder_rec(ptr, BFS_RIGHT(bfs_number), func);
}

void binarytree_inorder(BinaryTree* tree, void (*func)(BinaryPage *page, int iterator, int left_son, int right_son))
{
    binarytree_inorder_rec(tree->root, 0, func);
}

static void binarytree_graphviz_output_rec(BinaryPage *ptr, int bfs_number, FILE *out)
{
    int iterator = BFS_CONVERT(bfs_number);

    if (is_next(bfs_number))
    {

        if (is_exist(ptr, iterator))
        {
            ptr = (BinaryPage*)ptr->keys_and_next[iterator];
        }
        else
        {
            ptr = NULL;
        }

        bfs_number = 0;
        iterator = 0;
    }

    if (!ptr || !is_exist(ptr, iterator))
    {
        return;
    }



    fprintf(out, "%" PRIu64 " -> {", ptr->keys_and_next[iterator]);
    int left_iterator = BFS_LEFT_CONVERT(bfs_number);
    if (is_exist(ptr, left_iterator))
    {
        if (is_key(BFS_LEFT(bfs_number)))
        {
            fprintf(out, "%" PRIu64 " ", ptr->keys_and_next[left_iterator]);
        }
        else
        {
            BinaryPage* left = (BinaryPage*)ptr->keys_and_next[left_iterator];
            fprintf(out, "%" PRIu64 " ", left->keys_and_next[BFS_CONVERT(0)]);
        }
    }
    int right_iterator = BFS_RIGHT_CONVERT(bfs_number);
    if (is_exist(ptr, right_iterator))
    {
        if (is_key(BFS_RIGHT(bfs_number)))
        {
            fprintf(out, "%" PRIu64 " ", ptr->keys_and_next[right_iterator]);
        }
        else
        {
            BinaryPage* right = (BinaryPage*)ptr->keys_and_next[right_iterator];
            fprintf(out, "%" PRIu64 " ", right->keys_and_next[BFS_CONVERT(0)]);
        }
    }
    fprintf(out, "}\n");

    binarytree_graphviz_output_rec(ptr, BFS_LEFT(bfs_number), out);
    binarytree_graphviz_output_rec(ptr, BFS_RIGHT(bfs_number), out);
}

void binarytree_graphviz_output(BinaryTree* tree, FILE *out)
{
    fprintf(out, "digraph BFS{\nnode [shape=circle width=0.5 style=filled]\n");
    binarytree_graphviz_output_rec(tree->root, 0, out);
    fprintf(out, "}");
}
