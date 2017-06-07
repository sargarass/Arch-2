#ifndef INCLUDE_H
#define INCLUDE_H
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#if defined(_MSC_VER)
#define __ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__)
#define __ALIGN(n) __attribute__((aligned(n)))
#else
#error Unsupported compiler!
#endif

typedef uint8_t byte;
#define HEIGHT_T uint16_t

#define MAX(x, y) ((x) ^ (((x) ^ (y)) & -((x) < (y))))
#define MIN(x, y) ((y) ^ (((x) ^ (y)) & -((x) < (y))));

#define USE_ALLOCATOR

#define CACHE_LINE_SIZE 64 // размер кэш линии

//#define USE_THREAD
#define C_SIZE ((1 << 5)-1) // размер кластера
//#define USE_TEST_ON_RIGHT_WORK
#define USE_CACHE_TEST //
#define USE_CLUSTER_SORT // поддержка кластера полным

// Выбор порядка расположения элементов в кластере
//#define USE_BFS
#define USE_VEB
//#define USE_DFS
//#define USE_INORDER

#ifndef USE_THREAD
#define CLOCK_THREAD_CPUTIME_ID CLOCK_PROCESS_CPUTIME_ID
#endif
#if defined (USE_VEB)
#define LAYOUT_STRING "vEB"
#elif defined (USE_BFS)
#define LAYOUT_STRING "BFS"
#elif defined (USE_DFS)
#define LAYOUT_STRING "DFS"
#elif defined (USE_INORDER)
#define LAYOUT_STRING "INORDER"
#else
#define LAYOUT_STRING "NOTHING"
#endif

#ifndef USE_CACHE_TEST
#undef LAYOUT_STRING
#define LAYOUT_STRING "NOTHING (Pointers)"
#endif

#define inline __attribute__((always_inline))
typedef uint64_t Key;
typedef uint64_t Value;
typedef struct BinaryPage BinaryPage;
typedef struct BinaryTree BinaryTree;
uint32_t ilog_2(uint32_t v);

#define GET_HEIGHT(page, pos)
#define SET_HEIGHT(page, pos, height)
#define SWAP_HEIGHT(page1, pos1, page2, pos2)
#define ROUND_UP(x, y) (  ((x) + ((y) - 1)) & ~((y) - 1) )
//#define ROUND_UP(x, y) ((x + y - 1)/y)
#define NULL_KEY NAN
#define BFS_LEFT(x)  (2*(x) + 1)
#define BFS_RIGHT(x) (2*(x) + 2)
#define BFS_PARENT(x) ((((x) - 1)) >> 1)
#define BFS_HEIGHT(x) (ilog_2((x)))

#define GET_BYTE(x) ((x) % (1073741824) % (1048576) % (1024))
#define GET_KBYTE(x) ((x) % (1073741824) % (1048576) / (1024))
#define GET_MBYTE(x) ((x) % (1073741824) / (1048576))
#define GET_GBYTE(x) ((x) / (1073741824))
#define GET_MEMSTAT(x) GET_GBYTE((x)), GET_MBYTE((x)), GET_KBYTE((x)), GET_BYTE((x))

#endif // INCLUDE_H
