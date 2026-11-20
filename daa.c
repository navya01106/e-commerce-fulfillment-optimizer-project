/*
 * daa.c  -  THE ALGORITHM LIBRARY (code)
 * ---------------------------------------------------------------
 * Sorting and searching, divide and conquer, greedy, and dynamic programming
 * (0/1 knapsack and matrix chain),
 * all written by hand (no library sort/search).
 * Each one counts its work in "steps" and its extra memory in memNow/memPeak.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "daa.h"

/* ================================================================
 * COUNTERS
 * ================================================================ */
long long steps = 0, memNow = 0, memPeak = 0;

void resetCounters(void) { steps = 0; memNow = 0; memPeak = 0; }

void memUse(long long bytes) {
    memNow += bytes;
    if (memNow > memPeak) memPeak = memNow;
}
void memFree(long long bytes) { memNow -= bytes; }


/* ================================================================
 * 1. SORTING AND SEARCHING
 * ================================================================ */
int keyOf(Order o, int sortBy) {
    if (sortBy == BY_ID) return o.id;
    if (sortBy == BY_START) return o.slotStart;
    return o.slotEnd;
}

/* ---- Merge sort: split in half, sort both halves, merge them.
 * T(n) = 2T(n/2) + n  ->  Θ(n log n) in every case. Extra space O(n). Stable. */
static Order temp[MAX_ORDERS];

static void merge(Order a[], int left, int mid, int right, int sortBy) {
    long long bytes = (long long)(right - left + 1) * sizeof(Order);
    memUse(bytes);                                     /* the part of temp[] we use */
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        steps++;
        if (keyOf(a[i], sortBy) <= keyOf(a[j], sortBy)) temp[k++] = a[i++];   /* <= keeps it stable */
        else temp[k++] = a[j++];
    }
    while (i <= mid)   temp[k++] = a[i++];
    while (j <= right) temp[k++] = a[j++];
    for (k = left; k <= right; k++) a[k] = temp[k];
    memFree(bytes);
}

void mergeSort(Order a[], int left, int right, int sortBy) {
    if (left >= right) return;                         /* 0 or 1 item: already sorted */
    memUse(CALL_FRAME);
    int mid = (left + right) / 2;
    mergeSort(a, left, mid, sortBy);
    mergeSort(a, mid + 1, right, sortBy);
    merge(a, left, mid, right, sortBy);
    memFree(CALL_FRAME);
}

/* ---- Quick sort: last item is the pivot; smaller keys go to its left.
 * Average T(n) = 2T(n/2) + n -> n log n.  Worst (sorted input) T(n) = T(n-1) + n -> n^2. Not stable. */
static void swapOrders(Order *x, Order *y) { Order t = *x; *x = *y; *y = t; }

static int partition(Order a[], int low, int high, int sortBy) {
    int pivot = keyOf(a[high], sortBy);
    int i = low - 1;
    for (int j = low; j < high; j++) {
        steps++;
        if (keyOf(a[j], sortBy) < pivot) { i++; swapOrders(&a[i], &a[j]); }
    }
    swapOrders(&a[i + 1], &a[high]);
    return i + 1;
}

void quickSort(Order a[], int low, int high, int sortBy) {
    if (low >= high) return;
    memUse(CALL_FRAME);                                /* each waiting call uses stack memory */
    int p = partition(a, low, high, sortBy);
    quickSort(a, low, p - 1, sortBy);
    quickSort(a, p + 1, high, sortBy);
    memFree(CALL_FRAME);
}

int isSorted(Order a[], int n, int sortBy) {
    for (int i = 1; i < n; i++)
        if (keyOf(a[i], sortBy) < keyOf(a[i - 1], sortBy)) return 0;
    return 1;
}

/* ---- Linear search: check one by one. Best O(1), worst O(n). */
int linearSearch(Order a[], int n, int id) {
    for (int i = 0; i < n; i++) {
        steps++;
        if (a[i].id == id) return i;
    }
    return -1;
}

/* ---- Binary search (loop). List must be sorted by ID. O(log n), O(1) extra space. */
int binarySearchIterative(Order a[], int n, int id) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        steps++;
        if (a[mid].id == id) return mid;
        if (a[mid].id < id) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

/* ---- Binary search (recursive). T(n) = T(n/2) + 1 -> O(log n), O(log n) stack. */
int binarySearchRecursive(Order a[], int low, int high, int id) {
    if (low > high) return -1;
    memUse(CALL_FRAME);
    int mid = (low + high) / 2, answer;
    steps++;
    if (a[mid].id == id) answer = mid;
    else if (a[mid].id < id) answer = binarySearchRecursive(a, mid + 1, high, id);
    else answer = binarySearchRecursive(a, low, mid - 1, id);
    memFree(CALL_FRAME);
    return answer;
}


/* ================================================================
 * 2. DIVIDE AND CONQUER
 * ================================================================ */

/* ---- Maximum subarray, simple: try every start and end day. O(n^2). */
Streak maxSubarraySimple(long long p[], int n) {
    Streak best = {p[0], 0, 0};
    for (int s = 0; s < n; s++) {
        long long sum = 0;
        for (int e = s; e < n; e++) {
            sum += p[e];
            steps++;
            if (sum > best.sum) { best.sum = sum; best.start = s; best.end = e; }
        }
    }
    return best;
}

static Streak crossingMiddle(long long p[], int low, int mid, int high) {
    long long sum = 0, leftBest = p[mid], rightBest = p[mid + 1];
    int s = mid, e = mid + 1;
    for (int i = mid; i >= low; i--) {
        sum += p[i]; steps++;
        if (sum > leftBest) { leftBest = sum; s = i; }
    }
    sum = 0;
    for (int j = mid + 1; j <= high; j++) {
        sum += p[j]; steps++;
        if (sum > rightBest) { rightBest = sum; e = j; }
    }
    Streak r = {leftBest + rightBest, s, e};
    return r;
}

/* ---- Maximum subarray, divide and conquer: best is in the LEFT half, the RIGHT half,
 * or CROSSES the middle.  T(n) = 2T(n/2) + n -> O(n log n). */
Streak maxSubarrayDivide(long long p[], int low, int high) {
    if (low == high) { Streak one = {p[low], low, low}; return one; }
    memUse(CALL_FRAME);
    int mid = (low + high) / 2;
    Streak left = maxSubarrayDivide(p, low, mid);
    Streak right = maxSubarrayDivide(p, mid + 1, high);
    Streak cross = crossingMiddle(p, low, mid, high);
    memFree(CALL_FRAME);
    if (left.sum >= right.sum && left.sum >= cross.sum) return left;
    if (right.sum >= cross.sum) return right;
    return cross;
}

/* ---- Power. Simple: multiply exp times, O(n). Fast: x^n = (x^(n/2))^2, O(log n). */
double powerSimple(double base, int exp) {
    double r = 1;
    for (int i = 0; i < exp; i++) { r *= base; steps++; }
    return r;
}

double powerFast(double base, int exp) {
    if (exp == 0) return 1;
    memUse(CALL_FRAME);
    double half = powerFast(base, exp / 2);
    double r = half * half; steps++;
    if (exp % 2 == 1) { r *= base; steps++; }
    memFree(CALL_FRAME);
    return r;
}

/* ---- Celebrity ("key supplier"): someone everybody depends on, who depends on nobody. */
int celebritySimple(int knows[], int n) {             /* check every pair: O(n^2) */
    for (int i = 0; i < n; i++) {
        int ok = 1;
        for (int j = 0; j < n && ok; j++) {
            if (i == j) continue;
            steps++;
            if (knows[i * n + j] || !knows[j * n + i]) ok = 0;
        }
        if (ok) return i;
    }
    return -1;
}

/* One question between two people rules one out.
 * Winner of left half vs winner of right half: T(n) = 2T(n/2) + 1 -> O(n). */
static int celebrityCandidate(int knows[], int n, int low, int high) {
    if (low == high) return low;
    memUse(CALL_FRAME);
    int mid = (low + high) / 2;
    int a = celebrityCandidate(knows, n, low, mid);
    int b = celebrityCandidate(knows, n, mid + 1, high);
    steps++;
    memFree(CALL_FRAME);
    return knows[a * n + b] ? b : a;      /* a depends on b -> a is out; else b is out */
}

int celebrityDivide(int knows[], int n) {
    if (n <= 0) return -1;
    int c = celebrityCandidate(knows, n, 0, n - 1);
    for (int i = 0; i < n; i++) {         /* check the survivor once: O(n) */
        if (i == c) continue;
        steps++;
        if (knows[c * n + i] || !knows[i * n + c]) return -1;
    }
    return c;
}

/* ---- Matrix multiplication. Classical: three loops, n^3 multiplications. */
void matrixClassical(long long A[], long long B[], long long C[], int n) {
    for (int i = 0; i < n * n; i++) C[i] = 0;
    for (int r = 0; r < n; r++)
        for (int k = 0; k < n; k++)
            for (int c = 0; c < n; c++) { C[r * n + c] += A[r * n + k] * B[k * n + c]; steps++; }
}

static long long *newMatrix(int n) {
    long long *m = calloc((size_t)n * n, sizeof(long long));
    if (m == NULL) { printf("Out of memory\n"); exit(1); }
    memUse((long long)n * n * sizeof(long long));
    return m;
}
static void freeMatrix(long long *m, int n) { free(m); memFree((long long)n * n * sizeof(long long)); }
static void addM(long long X[], long long Y[], long long Z[], int n) { for (int i = 0; i < n * n; i++) Z[i] = X[i] + Y[i]; }
static void subM(long long X[], long long Y[], long long Z[], int n) { for (int i = 0; i < n * n; i++) Z[i] = X[i] - Y[i]; }
static void getQ(long long M[], int n, long long Q[], int r, int c) {
    int h = n / 2;
    for (int i = 0; i < h; i++) for (int j = 0; j < h; j++) Q[i * h + j] = M[(r + i) * n + c + j];
}
static void putQ(long long M[], int n, long long Q[], int r, int c) {
    int h = n / 2;
    for (int i = 0; i < h; i++) for (int j = 0; j < h; j++) M[(r + i) * n + c + j] = Q[i * h + j];
}

/* ---- Strassen: 7 half-size products instead of 8.  T(n) = 7T(n/2) + n^2 -> n^2.81 */
static void strassenRec(long long A[], long long B[], long long C[], int n, int cutoff) {
    if (n <= cutoff || n == 1) { matrixClassical(A, B, C, n); return; }
    int h = n / 2;
    long long *A11 = newMatrix(h), *A12 = newMatrix(h), *A21 = newMatrix(h), *A22 = newMatrix(h);
    long long *B11 = newMatrix(h), *B12 = newMatrix(h), *B21 = newMatrix(h), *B22 = newMatrix(h);
    long long *M[7], *T1 = newMatrix(h), *T2 = newMatrix(h);
    for (int i = 0; i < 7; i++) M[i] = newMatrix(h);
    getQ(A, n, A11, 0, 0); getQ(A, n, A12, 0, h); getQ(A, n, A21, h, 0); getQ(A, n, A22, h, h);
    getQ(B, n, B11, 0, 0); getQ(B, n, B12, 0, h); getQ(B, n, B21, h, 0); getQ(B, n, B22, h, h);

    addM(A11, A22, T1, h); addM(B11, B22, T2, h); strassenRec(T1, T2, M[0], h, cutoff);  /* M1 */
    addM(A21, A22, T1, h);                        strassenRec(T1, B11, M[1], h, cutoff); /* M2 */
    subM(B12, B22, T2, h);                        strassenRec(A11, T2, M[2], h, cutoff); /* M3 */
    subM(B21, B11, T2, h);                        strassenRec(A22, T2, M[3], h, cutoff); /* M4 */
    addM(A11, A12, T1, h);                        strassenRec(T1, B22, M[4], h, cutoff); /* M5 */
    subM(A21, A11, T1, h); addM(B11, B12, T2, h); strassenRec(T1, T2, M[5], h, cutoff);  /* M6 */
    subM(A12, A22, T1, h); addM(B21, B22, T2, h); strassenRec(T1, T2, M[6], h, cutoff);  /* M7 */

    addM(M[0], M[3], T1, h); subM(T1, M[4], T1, h); addM(T1, M[6], T1, h); putQ(C, n, T1, 0, 0); /* C11 */
    addM(M[2], M[4], T1, h); putQ(C, n, T1, 0, h);                                              /* C12 */
    addM(M[1], M[3], T1, h); putQ(C, n, T1, h, 0);                                              /* C21 */
    subM(M[0], M[1], T1, h); addM(T1, M[2], T1, h); addM(T1, M[5], T1, h); putQ(C, n, T1, h, h); /* C22 */

    freeMatrix(A11, h); freeMatrix(A12, h); freeMatrix(A21, h); freeMatrix(A22, h);
    freeMatrix(B11, h); freeMatrix(B12, h); freeMatrix(B21, h); freeMatrix(B22, h);
    for (int i = 0; i < 7; i++) freeMatrix(M[i], h);
    freeMatrix(T1, h); freeMatrix(T2, h);
}

void matrixStrassen(long long A[], long long B[], long long C[], int n, int cutoff) {
    int size = 1;
    while (size < n) size *= 2;                   /* Strassen needs a power of 2: pad with zeros */
    long long *PA = newMatrix(size), *PB = newMatrix(size), *PC = newMatrix(size);
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++) { PA[r * size + c] = A[r * n + c]; PB[r * size + c] = B[r * n + c]; }
    strassenRec(PA, PB, PC, size, cutoff < 1 ? 1 : cutoff);
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++) C[r * n + c] = PC[r * size + c];
    freeMatrix(PA, size); freeMatrix(PB, size); freeMatrix(PC, size);
}


/* ================================================================
 * 3. GREEDY
 * ================================================================ */

/* ---- Activity selection: pick the slot that ENDS FIRST, skip clashes, repeat.
 * Gives the most slots one van (or one loading dock) can serve. O(n log n). */
int activitySelection(Order a[], int n, int chosenIds[]) {
    static Order copy[MAX_ORDERS];
    for (int i = 0; i < n; i++) copy[i] = a[i];
    mergeSort(copy, 0, n - 1, BY_END);
    int count = 0, freeAt = -1;
    for (int i = 0; i < n; i++) {
        steps++;
        if (copy[i].slotStart >= freeAt) { chosenIds[count++] = copy[i].id; freeAt = copy[i].slotEnd; }
    }
    return count;
}

/* ---- Interval partitioning: go by START time; give each slot to a group (van) that is
 * already free, otherwise open a new group. Uses the fewest groups = most overlapping slots. */
int intervalPartitioning(Order a[], int n, int groupOf[]) {
    static int freeAt[MAX_ORDERS];
    mergeSort(a, 0, n - 1, BY_START);
    int groups = 0;
    for (int i = 0; i < n; i++) {
        int chosen = -1;
        for (int g = 0; g < groups; g++) {
            steps++;
            if (freeAt[g] <= a[i].slotStart) { chosen = g; break; }
        }
        if (chosen == -1) chosen = groups++;
        groupOf[i] = chosen;
        freeAt[chosen] = a[i].slotEnd;
    }
    memUse((long long)groups * sizeof(int));
    memFree((long long)groups * sizeof(int));
    return groups;
}

/* ---- Fractional knapsack: highest Rs per kg first; split the last item. Optimal. */
double fractionalKnapsack(Goods g[], int n, double capacity, double taken[]) {
    for (int i = 0; i < n - 1; i++) {                  /* selection sort by Rs/kg, small list */
        int best = i;
        for (int j = i + 1; j < n; j++) {
            steps++;
            if (g[j].rupees / g[j].kg > g[best].rupees / g[best].kg) best = j;
        }
        Goods t = g[i]; g[i] = g[best]; g[best] = t;
    }
    double space = capacity, total = 0;
    for (int i = 0; i < n; i++) {
        taken[i] = 0;
        if (space <= 0) continue;
        taken[i] = (g[i].kg <= space) ? g[i].kg : space;
        total += taken[i] * g[i].rupees / g[i].kg;
        space -= taken[i];
    }
    return total;
}

/* ---- Huffman coding: always join the two LEAST frequent groups.
 * Frequent characters end up near the top of the tree -> short codes. */
static int hFreq[512], hLeft[512], hRight[512], hChar[512], hUsed[512];

static void huffmanCodes(int node, char path[], int depth, char code[256][MAX_CODE]) {
    if (hChar[node] != -1) {
        if (depth == 0) path[depth++] = '0';           /* text with only one kind of character */
        path[depth] = '\0';
        strcpy(code[hChar[node]], path);
        return;
    }
    if (depth >= MAX_CODE - 2) return;                 /* safety: code too long */
    path[depth] = '0'; huffmanCodes(hLeft[node], path, depth + 1, code);
    path[depth] = '1'; huffmanCodes(hRight[node], path, depth + 1, code);
}

static int smallestUnused(int count) {
    int best = -1;
    for (int i = 0; i < count; i++) {
        if (hUsed[i]) continue;
        steps++;
        if (best == -1 || hFreq[i] < hFreq[best]) best = i;
    }
    return best;
}

long long huffmanBuild(const char *text, int freq[256], char code[256][MAX_CODE]) {
    for (int c = 0; c < 256; c++) { freq[c] = 0; code[c][0] = '\0'; }
    for (int i = 0; text[i]; i++) freq[(unsigned char)text[i]]++;
    int count = 0;
    for (int c = 0; c < 256; c++)
        if (freq[c] > 0) { hFreq[count] = freq[c]; hChar[count] = c; hLeft[count] = hRight[count] = -1; hUsed[count] = 0; count++; }
    if (count == 0) return 0;
    int groups = count;
    while (groups > 1) {
        int a = smallestUnused(count); hUsed[a] = 1;
        int b = smallestUnused(count); hUsed[b] = 1;
        hFreq[count] = hFreq[a] + hFreq[b]; hLeft[count] = a; hRight[count] = b;
        hChar[count] = -1; hUsed[count] = 0;
        count++; groups--;
    }
    memUse((long long)count * 5 * sizeof(int));        /* the tree's node arrays */
    memFree((long long)count * 5 * sizeof(int));
    char path[MAX_CODE];
    huffmanCodes(count - 1, path, 0, code);
    long long bits = 0;
    for (int c = 0; c < 256; c++) bits += (long long)freq[c] * (long long)strlen(code[c]);
    return bits;
}

int huffmanRoundTrip(const char *text, char code[256][MAX_CODE]) {
    size_t total = 0;
    for (int i = 0; text[i]; i++) total += strlen(code[(unsigned char)text[i]]);
    char *bits = malloc(total + 1), *back = malloc(strlen(text) + 1);
    if (!bits || !back) { free(bits); free(back); return 0; }
    size_t pos = 0;
    for (int i = 0; text[i]; i++) { strcpy(bits + pos, code[(unsigned char)text[i]]); pos += strlen(code[(unsigned char)text[i]]); }
    bits[pos] = '\0';
    char cur[MAX_CODE];
    int len = 0, out = 0;
    for (size_t i = 0; i < pos; i++) {                 /* read bits until they match a code */
        cur[len++] = bits[i]; cur[len] = '\0';
        for (int c = 0; c < 256; c++)
            if (code[c][0] && strcmp(cur, code[c]) == 0) { back[out++] = (char)c; len = 0; break; }
        if (len >= MAX_CODE - 1) break;
    }
    back[out] = '\0';
    int same = strcmp(text, back) == 0;
    free(bits); free(back);
    return same;
}

/* ---- Dijkstra: take the closest unfinished node; it is now final; improve its neighbours.
 * Needs weights >= 0. O(V^2) with this simple table version. */
void dijkstra(int n, int w[][MAX_V], int src, int dist[], int prev[]) {
    int done[MAX_V] = {0};
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int i = 0; i < n; i++) {
            if (done[i]) continue;
            steps++;
            if (u == -1 || dist[i] < dist[u]) u = i;
        }
        if (u == -1 || dist[u] == INF) break;
        done[u] = 1;
        for (int v = 0; v < n; v++) {
            if (w[u][v] == 0 || done[v]) continue;
            steps++;
            if (dist[u] + w[u][v] < dist[v]) { dist[v] = dist[u] + w[u][v]; prev[v] = u; }
        }
    }
}

/* ---- Prim: grow one tree; always add the shortest edge that reaches a NEW node. O(V^2). */
int prim(int n, int w[][MAX_V], int parent[]) {
    int inTree[MAX_V] = {0}, cheapest[MAX_V], total = 0;
    for (int i = 0; i < n; i++) { cheapest[i] = INF; parent[i] = -1; }
    cheapest[0] = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int i = 0; i < n; i++) {
            if (inTree[i]) continue;
            steps++;
            if (u == -1 || cheapest[i] < cheapest[u]) u = i;
        }
        if (cheapest[u] == INF) return -1;
        inTree[u] = 1;
        total += cheapest[u];
        for (int v = 0; v < n; v++)
            if (w[u][v] != 0 && !inTree[v] && w[u][v] < cheapest[v]) { cheapest[v] = w[u][v]; parent[v] = u; }
    }
    return total;
}

/* ---- Kruskal: edges from shortest to longest; keep one unless it makes a loop. */
static int findGroup(int group[], int x) {
    while (group[x] != x) x = group[x];
    return x;
}

int kruskal(int n, Edge e[], int m, Edge chosen[], int *count) {
    Edge s[MAX_V * MAX_V];
    for (int i = 0; i < m; i++) s[i] = e[i];
    for (int i = 1; i < m; i++) {                      /* insertion sort by weight */
        Edge cur = s[i];
        int j = i - 1;
        while (j >= 0 && s[j].weight > cur.weight) { s[j + 1] = s[j]; j--; steps++; }
        s[j + 1] = cur;
    }
    int group[MAX_V], total = 0;
    for (int i = 0; i < n; i++) group[i] = i;
    *count = 0;
    for (int i = 0; i < m; i++) {
        int g1 = findGroup(group, s[i].from), g2 = findGroup(group, s[i].to);
        steps++;
        if (g1 != g2) { group[g1] = g2; chosen[(*count)++] = s[i]; total += s[i].weight; }
    }
    return (*count == n - 1) ? total : -1;
}


/* ================================================================
 * 4. DYNAMIC PROGRAMMING
 * ================================================================ */

/* ---- 0/1 knapsack. best[i][c] = most value using the first i items with c space.
 * Two choices per item: leave it, or take it (if it fits). O(n*W) time and space. */
#define MAX_CAP 2000
static int best[MAX_V + 1][MAX_CAP + 1];

int knapsackDP(int wt[], int val[], int n, int cap, int take[]) {
    if (n > MAX_V || cap > MAX_CAP || cap < 0) return -1;
    memUse((long long)(n + 1) * (cap + 1) * sizeof(int));
    for (int c = 0; c <= cap; c++) best[0][c] = 0;
    for (int i = 1; i <= n; i++)
        for (int c = 0; c <= cap; c++) {
            steps++;
            best[i][c] = best[i - 1][c];                                    /* leave item i */
            if (wt[i - 1] <= c && val[i - 1] + best[i - 1][c - wt[i - 1]] > best[i][c])
                best[i][c] = val[i - 1] + best[i - 1][c - wt[i - 1]];       /* take item i */
        }
    int c = cap;                                        /* walk back to see what was taken */
    for (int i = n; i >= 1; i--) {
        take[i - 1] = best[i][c] != best[i - 1][c];
        if (take[i - 1]) c -= wt[i - 1];
    }
    memFree((long long)(n + 1) * (cap + 1) * sizeof(int));
    return best[n][cap];
}

/* Greedy for items that cannot be split: best value per kg first, if it fits. */
int knapsackGreedy(int wt[], int val[], int n, int cap, int take[]) {
    int idx[MAX_V];
    if (n > MAX_V) return -1;
    for (int i = 0; i < n; i++) { idx[i] = i; take[i] = 0; }
    for (int i = 0; i < n - 1; i++) {
        int b = i;
        for (int j = i + 1; j < n; j++) {
            steps++;
            if ((double)val[idx[j]] / wt[idx[j]] > (double)val[idx[b]] / wt[idx[b]]) b = j;
        }
        int t = idx[i]; idx[i] = idx[b]; idx[b] = t;
    }
    int space = cap, total = 0;
    for (int k = 0; k < n; k++)
        if (wt[idx[k]] <= space) { take[idx[k]] = 1; space -= wt[idx[k]]; total += val[idx[k]]; }
    return total;
}

/* ---- Matrix chain: cheapest order to multiply tables A1 x A2 x ... x An.
 * cost[i][j] = min over k of cost[i][k] + cost[k+1][j] + d[i-1]*d[k]*d[j].  O(n^3). */
static void writeOrder(int split[][MAX_V], int i, int j, char out[]) {
    char piece[16];
    if (i == j) { sprintf(piece, "A%d", i); strcat(out, piece); return; }
    strcat(out, "(");
    writeOrder(split, i, split[i][j], out);
    strcat(out, " x ");
    writeOrder(split, split[i][j] + 1, j, out);
    strcat(out, ")");
}

long long matrixChain(int dims[], int n, char order[]) {
    static long long cost[MAX_V][MAX_V];
    static int split[MAX_V][MAX_V];
    memUse((long long)n * n * (sizeof(long long) + sizeof(int)));
    for (int i = 1; i <= n; i++) cost[i][i] = 0;
    for (int len = 2; len <= n; len++)
        for (int i = 1; i + len - 1 <= n; i++) {
            int j = i + len - 1;
            cost[i][j] = -1;
            for (int k = i; k < j; k++) {
                steps++;
                long long c = cost[i][k] + cost[k + 1][j] + (long long)dims[i - 1] * dims[k] * dims[j];
                if (cost[i][j] == -1 || c < cost[i][j]) { cost[i][j] = c; split[i][j] = k; }
            }
        }
    order[0] = '\0';
    writeOrder(split, 1, n, order);
    memFree((long long)n * n * (sizeof(long long) + sizeof(int)));
    return cost[1][n];
}
