#include "test.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "bin_tree_nocache.h"
#include "bin_tree.h"
#ifdef  USE_THREAD
#include <pthread.h>
#endif
static FILE* out;
/*
static uint64_t g_seed = SEARCH_RAND_SEED;

//Used to seed the generator.

inline void mysrand( uint64_t seed )
{
    g_seed = seed;
}

//fastrand routine returns one integer, similar output value range as C lib.

inline uint64_t myrand()
{
    g_seed = (214013 * g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}*/

static void show(BinaryPage* ptr, int bfs_number)
{
    fprintf(out, "%zu\n", ptr->keys_and_next[bfs_number]);
}

static void show_no_cache(NCLeaf* ptr)
{
    fprintf(out, "%zu\n", ptr->key);
}

static void graphviz_show_no_cache(NCLeaf* ptr)
{
    fprintf(out, "%zu -> {", ptr->key);

    if (ptr->left) {
        fprintf(out, "%zu ", ((NCLeaf*)(ptr->left))->key);
    }

    if (ptr->right) {
        fprintf(out, "%zu ", ((NCLeaf*)(ptr->right))->key);
    }
    fprintf(out, "}\n");
}

static void open_input(const char *file_input, FILE** test_in)
{
    *test_in = fopen(file_input, "r");
    if (!(*test_in))
    {
        fprintf(stderr, "open_input: File not exist \"%s\"", file_input);
        exit(1);
    }
}

static void open_output(const char *file_output, FILE** test_out)
{
    *(test_out) = fopen(file_output, "w");
    if (!(*test_out))
    {
        fprintf(stderr, "void open_output: File not exist \"%s\"", file_output);
        exit(1);
    }
}

//*************************************
typedef struct
{
    uint64_t *val;
    uint64_t end;
} arg_struct;

static void show_progress(void* __args)
{
    arg_struct *args = (arg_struct*)(__args);

    int percent = 0;
    int new_percent = 0;

    while( percent != 100 )
    {
        new_percent =(((double)(*(args->val))) * 100) / args->end;
        if (new_percent != percent)
        {
            percent = new_percent;
            printf("\rPrepared for %d\% ",percent);
            fflush(stdout);
        }
    }
    printf("                                                        \r");
    fflush(stdout);
}
//*************************************

static void load_data(const char *file_input, uint64_t *num, Key **test_val)
{
    #ifdef  USE_THREAD
    pthread_t thread;
    arg_struct args;
    #endif
    FILE* test_in;
    char out[1000];
    uint64_t count;
    uint64_t i;


    open_input(file_input, &test_in);

    strcpy(out, file_input);
    strcat(out, "_out.txt");

    //open_output(out, test_out);
    printf("Loading data...\n");
    fscanf(test_in, "%zu", &count);
    count <<= 1;
    #ifdef  USE_THREAD
    args.end = count;
    args.val = &i;
    pthread_create(&thread, NULL, show_progress, &args);
    #endif
    Key *value = malloc(sizeof(Key) * count * 2);
    for (i = 0; i < count; i++)
    {
        fscanf(test_in, "%zu", &value[i]);
    }
    #ifdef  USE_THREAD
    pthread_join(thread, NULL);
    #endif
    fclose(test_in);

    *num = count;
    *test_val = value;
}

static void test_insert_no_cache(NCTree* tree, const Key* test_val, uint64_t size)
{
    FILE *no_cache_out;
    uint64_t i = 0;
    struct timespec start, end;
    open_output("no_cache_out.txt", &no_cache_out);
#ifdef  USE_THREAD
    pthread_t thread;
    arg_struct args;
    args.val = &i;
    args.end = size;
#endif
    printf("Test: insert no_cached tree\n");
#ifdef  USE_THREAD
    pthread_create(&thread, NULL, show_progress, &args);
#endif
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    for (i = 0; i < size; i += 2)
    {
        nc_insert(tree, test_val[i], test_val[i+1]);
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    long double time =  ((((long double)end.tv_sec)+ ((long double)end.tv_nsec)*1.e-9)-((long double)start.tv_sec+((long double)start.tv_nsec)*1.e-9));
#ifdef  USE_THREAD
    pthread_join(thread, NULL);
#endif
    out = no_cache_out;

    /*fprintf(out, "digraph BFS{\nnode [shape=circle width=0.5 style=filled]\n");
    nc_in_order(tree, graphviz_show_no_cache);
    fprintf(out, "}");*/

    //nc_in_order(tree, show_no_cache);


    fclose(no_cache_out);
    printf("time elapsed: %.*Lg\n", LDBL_DIG, time);
}

static void test_insert_cache(BinaryTree* tree, const Key* test_val, uint64_t size)
{
    printf("Test: insert cache_tree\n");
    FILE *cache_out;
    uint64_t i = 0;
    struct timespec start, end;
#ifdef USE_THREAD
    pthread_t thread;
    arg_struct args;
    args.val = &i;
    args.end = size;
#endif

    open_output("cache_out.txt", &cache_out);


#ifdef USE_THREAD
    pthread_create(&thread, NULL, show_progress, &args);
#endif

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    for (i = 0; i < size; i += 2)
    {
        binarytree_insert(tree, test_val[i], test_val[i + 1]);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

    long double time =  ((((long double)end.tv_sec)+((long double)end.tv_nsec)*1.e-9)-((long double)start.tv_sec+((long double)start.tv_nsec)*1.e-9));

#ifdef USE_THREAD
    pthread_join(thread, NULL);
#endif
    out = cache_out;
    //binarytree_inorder(tree, show);
    //binarytree_graphviz_output(tree, out);

    fclose(cache_out);

    printf("time elapsed: %.*Lg\n", LDBL_DIG, time);
}

static void test_tree_cache(BinaryTree* tree, const Key* test_val, uint64_t size)
{
    printf("Test: cache_tree on right work\n");
    Value* found;
    bool res = true;

    for (uint64_t i = 0; i < size; i += 2)
    {
        bool exist = binarytree_search(tree, test_val[i], &found);
        if (exist)
        {
            if (*found != test_val[i + 1])
            {
                printf("Node with key %zu associated with %zu. Expected with %zu\n", test_val[i], *found, test_val[i + 1]);
            }
        }
        else
        {
            printf("Node with key %zu in not exist\n", test_val[i]);
        }
        res &= exist;
    }

    if (res)
    {
        printf("OK\n");
    }
    else
    {
        printf("Some tests failed\n");
    }
}

static void test_search_cache(BinaryTree* tree, const Key* test_val, uint64_t size)
{
    printf("Test: search cache_tree\n");

    Value* found;
    uint64_t i = 0;
    struct timespec start, end;

#ifdef USE_THREAD
    pthread_t thread;
    arg_struct args;
    args.val = &i;
    args.end = size;
    pthread_create(&thread, NULL, show_progress, &args);
#endif



    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    for (i = 0; i < size; i++)
    {
        binarytree_search(tree, test_val[i], &found);
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

    long double time =  ((((long double)end.tv_sec)+((long double)end.tv_nsec)*1.e-9)-((long double)start.tv_sec+((long double)start.tv_nsec)*1.e-9));

#ifdef USE_THREAD
    pthread_join(thread, NULL);
#endif

    printf("time elapsed: %.*Lg\n", LDBL_DIG, time);
}

static void test_search_no_cache(NCTree* tree, const Key* test_val, uint64_t size)
{
    printf("Test: search no_cache_tree\n");

    Value* found;
    uint64_t i = 0;
    struct timespec start, end;

#ifdef USE_THREAD
    pthread_t thread;
    arg_struct args;
    args.val = &i;
    args.end = size;
    pthread_create(&thread, NULL, show_progress, &args);
#endif

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    for (i = 0; i < size; i++)
    {
        nc_search(tree, test_val[i], &found);
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

    long double time =  ((((long double)end.tv_sec)+((long double)end.tv_nsec)*1.e-9)-((long double)start.tv_sec+((long double)start.tv_nsec)*1.e-9));

#ifdef USE_THREAD
    pthread_join(thread, NULL);
#endif

    printf("time elapsed: %.*Lg\n", LDBL_DIG, time);
}

static void test_tree_no_cache(NCTree* tree, const Key* test_val, uint64_t size)
{
    printf("Test: no cache tree on right work\n");
    Value* found;
    bool res = true;

    for (uint64_t i = 0; i < size; i += 2)
    {
        bool exist = nc_search(tree, test_val[i], &found);
        if (exist)
        {
            if (*found != test_val[i + 1])
            {
                printf("Node with key %zu associated with %zu. Expected with %zu\n", test_val[i], *found, test_val[i + 1]);
            }
        }
        else
        {
            printf("Node with key %zu in not exist\n", test_val[i]);
        }
        res &= exist;
    }

    if (res)
    {
        printf("OK\n");
    }
    else
    {
        printf("Some tests failed\n");
    }
}

void run_test(const char *file_input, const char *search_test)
{
    uint64_t size, search_size;
    Key* test_val;
    Key* search_val;
    BinaryTree cache;
    NCTree no_cache;
    binarytree_init(&cache);
    nc_init(&no_cache);

    load_data(file_input, &size, &test_val);
#ifdef USE_CACHE_TEST
    test_insert_cache(&cache, test_val, size);
    load_data(search_test, &search_size, &search_val);
    test_search_cache(&cache, search_val, search_size);
#ifdef USE_TEST_ON_RIGHT_WORK
    test_tree_cache(&cache, test_val, size);
#endif
#else
    test_insert_no_cache(&no_cache, test_val, size);
    load_data(search_test, &search_size, &search_val);
    test_search_no_cache(&no_cache, search_val, search_size);
#ifdef USE_TEST_ON_RIGHT_WORK
    test_tree_no_cache(&no_cache, test_val, size);
#endif
#endif

    /*if (time_no_cache > time_cache) {
        printf("Cache-tree faster x%2.2f times\n", time_no_cache/time_cache);
    } else {
        printf("No-cache-tree faster x%2.2f times\n", time_cache/time_no_cache);
    }*/

    //fclose(test_out);
}
