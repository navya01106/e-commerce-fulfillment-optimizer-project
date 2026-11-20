/*
 * daa.h  -  THE ALGORITHM LIBRARY (interface)
 * ---------------------------------------------------------------
 * Every algorithm the project uses is declared here, grouped by topic:
 * sorting and searching, divide and conquer, greedy, and dynamic programming
 * (0/1 knapsack and matrix chain).
 * The code is in daa.c. Any program can #include "daa.h" and use them.
 *
 * Consistent rules for every function:
 *   - inputs are plain arrays + their size
 *   - the answer is returned, or written into an output array
 *   - every algorithm adds to "steps" (work done) and records memory
 *     in memNow/memPeak, so any two methods can be compared fairly
 */
#ifndef DAA_H
#define DAA_H

#define MAX_ORDERS 20000   /* biggest list of orders (stress test) */
#define MAX_V      32      /* most nodes in a graph or items in a small problem */
#define INF        1000000000
#define CALL_FRAME 48      /* estimated bytes of stack memory one recursive call uses */

/* ---------------- shared record types ---------------- */
typedef struct {
    int id;              /* order number, e.g. 101 */
    char customer[30];
    char area[30];       /* delivery area name */
    double weight;       /* kg */
    double value;        /* rupees */
    int slotStart;       /* delivery slot start hour */
    int slotEnd;         /* delivery slot end hour */
} Order;

typedef struct { int from, to, weight; } Edge;            /* a road or link in a graph */
typedef struct { long long sum; int start, end; } Streak; /* answer of maximum subarray */
typedef struct { char name[20]; double kg; double rupees; } Goods;   /* splittable bulk goods */

/* ---------------- counters used by every algorithm ---------------- */
extern long long steps;     /* basic operations done (comparisons, additions, ...) */
extern long long memNow;    /* extra memory in use right now (bytes) */
extern long long memPeak;   /* most extra memory used at one time (bytes) */
void resetCounters(void);
void memUse(long long bytes);
void memFree(long long bytes);

/* =========== 1. SORTING AND SEARCHING =========== */
#define BY_END   0   /* earliest slot end first (most urgent) */
#define BY_START 1   /* earliest slot start first */
#define BY_ID    2   /* smallest order ID first */
int  keyOf(Order o, int sortBy);
void mergeSort(Order a[], int left, int right, int sortBy);   /* Θ(n log n), stable */
void quickSort(Order a[], int low, int high, int sortBy);     /* avg n log n, worst n^2 */
int  isSorted(Order a[], int n, int sortBy);
int  linearSearch(Order a[], int n, int id);                  /* O(n) */
int  binarySearchIterative(Order a[], int n, int id);         /* O(log n), list sorted by ID */
int  binarySearchRecursive(Order a[], int low, int high, int id);

/* =========== 2. DIVIDE AND CONQUER =========== */
Streak maxSubarraySimple(long long p[], int n);               /* O(n^2) */
Streak maxSubarrayDivide(long long p[], int low, int high);   /* O(n log n) */
double powerSimple(double base, int exp);                     /* O(n) */
double powerFast(double base, int exp);                       /* O(log n) */
int  celebritySimple(int knows[], int n);    /* knows[i*n+j] = 1 if i depends on j. O(n^2) */
int  celebrityDivide(int knows[], int n);    /* O(n) */
void matrixClassical(long long A[], long long B[], long long C[], int n);              /* n^3 */
void matrixStrassen(long long A[], long long B[], long long C[], int n, int cutoff);   /* n^2.81 */

/* =========== 3. GREEDY =========== */
int  activitySelection(Order a[], int n, int chosenIds[]);    /* most non-clashing slots */
int  intervalPartitioning(Order a[], int n, int groupOf[]);   /* fewest groups (vans); sorts a by start */
double fractionalKnapsack(Goods g[], int n, double capacity, double taken[]);   /* sorts g by Rs/kg */
#define MAX_CODE 64
long long huffmanBuild(const char *text, int freq[256], char code[256][MAX_CODE]);  /* returns bits */
int  huffmanRoundTrip(const char *text, char code[256][MAX_CODE]);   /* 1 = decoding gives the same text */
/* graphs as a table: w[a][b] = weight, 0 = no edge */
void dijkstra(int n, int w[][MAX_V], int src, int dist[], int prev[]);   /* O(V^2) */
int  prim(int n, int w[][MAX_V], int parent[]);                           /* O(V^2), -1 if not connected */
int  kruskal(int n, Edge e[], int m, Edge chosen[], int *count);         /* O(E log E) */

/* =========== 4. DYNAMIC PROGRAMMING =========== */
int  knapsackDP(int wt[], int val[], int n, int cap, int take[]);        /* O(n*W), always optimal */
int  knapsackGreedy(int wt[], int val[], int n, int cap, int take[]);    /* fast, not always optimal */
long long matrixChain(int dims[], int n, char order[]);   /* n matrices, dims has n+1 numbers; O(n^3) */

#endif
