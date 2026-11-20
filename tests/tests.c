/*
 * tests.c  -  THE TEST SUITE        run with:  make test
 * ---------------------------------------------------------------
 * 1. Unit tests with ORACLES: a slow, obviously-correct method (try everything)
 *    is compared with the clever algorithm on many random small inputs.
 *    Covers sorting, searching, divide and conquer and greedy.
 * 2. Known answers from textbooks.
 * 3. Edge cases and invalid inputs.
 * 4. Integration test: the full daily plan from data/orders.csv.
 * 5. Regression values: results we checked by hand must not change.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "daa.h"
#include "planner.h"

static int passed = 0, failed = 0;

static void check(int ok, const char *name) {
    if (ok) passed++;
    else { failed++; printf("  FAILED: %s\n", name); }
}

static int randomInt(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

/* ---------------------------------------------------------------- oracles */


static int bruteActivity(Order o[], int n) {
    int best = 0;
    for (int mask = 0; mask < (1 << n); mask++) {
        int ok = 1, c = 0;
        for (int i = 0; i < n && ok; i++) {
            if (!(mask & (1 << i))) continue;
            c++;
            for (int j = i + 1; j < n; j++)
                if ((mask & (1 << j)) && o[i].slotStart < o[j].slotEnd && o[j].slotStart < o[i].slotEnd) ok = 0;
        }
        if (ok && c > best) best = c;
    }
    return best;
}

static int maxOverlap(Order o[], int n) {
    int best = 0;
    for (int h = 0; h < 24; h++) {
        int c = 0;
        for (int i = 0; i < n; i++) if (o[i].slotStart <= h && h < o[i].slotEnd) c++;
        if (c > best) best = c;
    }
    return best;
}


static Order makeOrder(int id, int start, int end) {
    Order o;
    memset(&o, 0, sizeof(o));
    o.id = id; o.slotStart = start; o.slotEnd = end; o.weight = 1; strcpy(o.area, "Bopal");
    return o;
}

/* ---------------------------------------------------------------- 1. sorting and searching */
static void testSortingSearching(void) {
    static Order a[500], b[500];
    for (int round = 0; round < 60; round++) {
        int n = randomInt(0, 400);
        long long sumBefore = 0, sumA = 0, sumB = 0;
        for (int i = 0; i < n; i++) { a[i] = makeOrder(i, 0, randomInt(1, 6)); b[i] = a[i]; sumBefore += a[i].slotEnd * 1000 + i; }
        mergeSort(a, 0, n - 1, BY_END);
        quickSort(b, 0, n - 1, BY_END);
        check(isSorted(a, n, BY_END), "merge sort output is sorted");
        check(isSorted(b, n, BY_END), "quick sort output is sorted");
        for (int i = 0; i < n; i++) { sumA += a[i].slotEnd * 1000 + a[i].id; sumB += b[i].slotEnd * 1000 + b[i].id; }
        check(sumA == sumBefore && sumB == sumBefore, "sorting keeps every order (nothing lost or duplicated)");
        int stable = 1;
        for (int i = 1; i < n; i++) if (a[i].slotEnd == a[i - 1].slotEnd && a[i].id < a[i - 1].id) stable = 0;
        check(stable, "merge sort is stable");
    }
    for (int i = 0; i < 50; i++) a[i] = makeOrder(2 * i, 0, 1);
    for (int id = -2; id < 102; id++) {
        int lin = linearSearch(a, 50, id);
        check(binarySearchIterative(a, 50, id) == lin, "binary search (loop) = linear search");
        check(binarySearchRecursive(a, 0, 49, id) == lin, "binary search (recursive) = linear search");
    }
    check(binarySearchIterative(a, 0, 4) == -1, "binary search on an empty list");
}

/* ---------------------------------------------------------------- 2. divide and conquer */
static void testDivideConquer(void) {
    long long textbook[16] = {13, -3, -25, 20, -3, -16, -23, 18, 20, -7, 12, -5, -22, 15, -4, 7};
    Streak s = maxSubarrayDivide(textbook, 0, 15);
    check(s.sum == 43 && s.start == 7 && s.end == 10, "max subarray textbook answer 43 (days 7-10)");
    long long neg[3] = {-5, -2, -9};
    check(maxSubarrayDivide(neg, 0, 2).sum == -2, "all losses -> best single day");
    for (int r = 0; r < 200; r++) {
        long long v[40];
        int n = randomInt(1, 40);
        for (int i = 0; i < n; i++) v[i] = randomInt(-20, 20);
        check(maxSubarrayDivide(v, 0, n - 1).sum == maxSubarraySimple(v, n).sum, "max subarray: divide = simple");
    }
    check(powerFast(2, 10) == 1024 && powerFast(7, 0) == 1, "fast power 2^10 and x^0");
    for (int e = 0; e < 40; e++) {
        double a = powerFast(1.05, e), b = powerSimple(1.05, e);
        check(a - b < 1e-9 && b - a < 1e-9, "fast power = simple power");
    }
    for (int r = 0; r < 300; r++) {
        int n = randomInt(1, 12), k[144];
        for (int i = 0; i < n * n; i++) k[i] = rand() % 2;
        for (int i = 0; i < n; i++) k[i * n + i] = 0;
        if (r % 2 == 0) { int c = rand() % n; for (int i = 0; i < n; i++) { k[c * n + i] = 0; if (i != c) k[i * n + c] = 1; } }
        check(celebrityDivide(k, n) == celebritySimple(k, n), "celebrity: divide = simple");
    }
    int sizes[6] = {1, 2, 3, 5, 17, 33};
    for (int s2 = 0; s2 < 6; s2++) {
        int n = sizes[s2];
        long long *A = malloc(sizeof(long long) * n * n), *B = malloc(sizeof(long long) * n * n);
        long long *C1 = malloc(sizeof(long long) * n * n), *C2 = malloc(sizeof(long long) * n * n);
        for (int i = 0; i < n * n; i++) { A[i] = randomInt(-9, 9); B[i] = randomInt(-9, 9); }
        matrixClassical(A, B, C1, n);
        matrixStrassen(A, B, C2, n, 1);
        check(memcmp(C1, C2, sizeof(long long) * n * n) == 0, "Strassen = classical (any size)");
        free(A); free(B); free(C1); free(C2);
    }
    long long Z[64], C[64];
    for (int i = 0; i < 64; i++) Z[i] = 1;
    resetCounters(); matrixStrassen(Z, Z, C, 8, 1);
    check(steps == 343, "pure Strassen on 8x8 uses 7^3 = 343 multiplications");
}

/* ---------------------------------------------------------------- 3. greedy */
static void testGreedy(void) {
    static Order o[14];
    int chosen[14], group[14];
    for (int r = 0; r < 100; r++) {
        int n = randomInt(1, 12);
        for (int i = 0; i < n; i++) { int s = randomInt(0, 19); o[i] = makeOrder(i, s, s + randomInt(1, 4)); }
        check(activitySelection(o, n, chosen) == bruteActivity(o, n), "activity selection = best of all subsets");
        check(intervalPartitioning(o, n, group) == maxOverlap(o, n), "vans needed = most overlapping slots");
    }
    Goods g[3] = {{"A", 10, 60}, {"B", 20, 100}, {"C", 30, 120}};
    double t[3];
    check(fractionalKnapsack(g, 3, 50, t) == 240, "fractional knapsack textbook = 240");
    check(fractionalKnapsack(g, 3, 0, t) == 0, "van with 0 kg carries nothing");
    static int freq[256];
    static char code[256][MAX_CODE];
    check(huffmanBuild("aaaaabbbbcccdde", freq, code) == 33, "Huffman total bits for a known example = 33");
    const char *texts[4] = {"hello world", "zzzz", "ORDER 101 SHIPPED\nORDER 102 PACKED\n", "a"};
    for (int i = 0; i < 4; i++) { huffmanBuild(texts[i], freq, code); check(huffmanRoundTrip(texts[i], code), "Huffman decodes back exactly"); }

    static int w[MAX_V][MAX_V];
    static Edge e[MAX_V * MAX_V], ch[MAX_V];
    for (int r = 0; r < 60; r++) {
        int n = randomInt(2, 15), m = 0, count, parent[MAX_V], dist[MAX_V], prev[MAX_V];
        for (int x = 0; x < n; x++) for (int y = 0; y < n; y++) w[x][y] = 0;
        for (int x = 1; x < n; x++) { int y = rand() % x; w[x][y] = w[y][x] = randomInt(1, 20); }
        for (int k = 0; k < n; k++) { int x = rand() % n, y = rand() % n; if (x != y) w[x][y] = w[y][x] = randomInt(1, 20); }
        for (int x = 0; x < n; x++) for (int y = 0; y < n; y++) if (w[x][y] && x < y) { Edge ed = {x, y, w[x][y]}; e[m++] = ed; }
        check(prim(n, w, parent) == kruskal(n, e, m, ch, &count) && count == n - 1, "Prim total = Kruskal total");
        int ok = 1;
        for (int src = 0; src < n; src++) {
            int slow[MAX_V];                         /* oracle: improve every road n times */
            for (int y = 0; y < n; y++) slow[y] = INF;
            slow[src] = 0;
            for (int round = 0; round < n; round++)
                for (int x = 0; x < n; x++)
                    for (int y = 0; y < n; y++)
                        if (w[x][y] && slow[x] != INF && slow[x] + w[x][y] < slow[y]) slow[y] = slow[x] + w[x][y];
            dijkstra(n, w, src, dist, prev);
            for (int y = 0; y < n; y++) if (dist[y] != slow[y]) ok = 0;
        }
        check(ok, "Dijkstra = slow oracle (improve every road n times)");
    }
}


/* ---------------------------------------------------------------- 4. dynamic programming */
static int bruteKnapsack(int wt[], int val[], int n, int cap) {     /* oracle: try every subset */
    int best = 0;
    for (int mask = 0; mask < (1 << n); mask++) {
        int w = 0, v = 0;
        for (int i = 0; i < n; i++) if (mask & (1 << i)) { w += wt[i]; v += val[i]; }
        if (w <= cap && v > best) best = v;
    }
    return best;
}

static void testDP(void) {
    for (int r = 0; r < 200; r++) {
        int n = randomInt(1, 12), wt[MAX_V], val[MAX_V], take[MAX_V], cap = randomInt(0, 60);
        for (int i = 0; i < n; i++) { wt[i] = randomInt(1, 20); val[i] = randomInt(1, 100); }
        int best = bruteKnapsack(wt, val, n, cap);
        int dp = knapsackDP(wt, val, n, cap, take), w = 0, v = 0;
        for (int i = 0; i < n; i++) if (take[i]) { w += wt[i]; v += val[i]; }
        check(dp == best, "knapsack DP = try every subset");
        check(v == dp && w <= cap, "knapsack DP's chosen parcels fit and add up");
        check(knapsackGreedy(wt, val, n, cap, take) <= best, "greedy is never better than the optimum");
    }
    int none[1] = {5}, nv[1] = {10}, take[1];
    check(knapsackDP(none, nv, 1, 0, take) == 0, "van with 0 kg carries Rs 0");
    int dims[7] = {30, 35, 15, 5, 10, 20, 25};
    char order[200];
    check(matrixChain(dims, 6, order) == 15125, "matrix chain textbook answer 15125");
    int two[3] = {10, 20, 30};
    check(matrixChain(two, 2, order) == 6000, "matrix chain with two matrices = 10 x 20 x 30");
    int one[2] = {5, 7};
    check(matrixChain(one, 1, order) == 0, "matrix chain with one matrix needs no work");
}

/* ---------------------------------------------------------------- 5. integration + invalid input + regression */
static void testIntegration(void) {
    resetFleet();
    buildMap();
    printf("\n  (the warnings below are expected: broken rows are fed in on purpose)\n");
    check(loadOrders("tests/bad_orders.csv") == 2, "invalid rows are rejected, valid rows kept");
    check(loadOrders("tests/no_such_file.csv") == -1, "missing file is reported");
    check(loadOrders("data/orders.csv") == 15, "sample data: 15 valid orders (1 bad row skipped)");

    planDay();
    int perHub[HUBS] = {0}, onTime = 1, fits = 1, stopsInOrder = 1;
    for (int i = 0; i < orderCount; i++) {
        if (hubOf[i] != POSTPONED) perHub[hubOf[i]]++;
        if (vanOf[i] != -1 && arriveAt[i] > orders[i].slotEnd * 60) onTime = 0;
        if (vanOf[i] != -1 && arriveAt[i] < orders[i].slotStart * 60) onTime = 0;
    }
    for (int v = 0; v < vanPlanCount; v++) {
        if (vanPlan[v].load > vanPlan[v].capacity) fits = 0;
        for (int i = 0; i < orderCount; i++)          /* stops of one van never overlap in time */
            for (int j = 0; j < orderCount; j++)
                if (i != j && vanOf[i] == v && vanOf[j] == v && arriveAt[i] < arriveAt[j] && arriveAt[i] + HANDOVER_MIN > arriveAt[j]) stopsInOrder = 0;
    }
    check(servedCount() == 15, "integration: every order is served on a normal day");
    check(perHub[0] <= hubs[0].capacity && perHub[1] <= hubs[1].capacity, "integration: hub limits respected");
    check(onTime, "integration: every delivery is inside its slot");
    check(fits, "integration: no van is overloaded");
    check(stopsInOrder, "integration: a van is never in two places at once");
    char text[120];
    int i110 = 0;
    while (orders[i110].id != 110) i110++;
    check(strncmp(orderStatus(i110, 6, text), "PACKED", 6) == 0, "tracking: before the van leaves -> PACKED");
    check(strncmp(orderStatus(i110, 23, text), "DELIVERED", 9) == 0, "tracking: late evening -> DELIVERED");
    check(exportPlan("results/test_plan.csv", "results/test_plan.json") == 1, "export to CSV and JSON");

    /* regression values (checked by hand once; they must not change) */
    static int g[MAX_ORDERS];
    check(vanPlanCount == 6 && hiredVans() == 2 && totalVanKm() == 237, "regression: 6 vans, 2 hired, 237 km");
    hubs[1].capacity = 2;
    check(assignCentresGreedy(g) == 13, "regression: shock day, greedy serves 13");
    resetFleet();
    int parent[MAX_V], wt[MAX_V], val[MAX_V], take[MAX_V];
    for (int i = 0; i < 15; i++) { wt[i] = (int)(orders[i].weight + 0.999); val[i] = (int)orders[i].value; }
    check(knapsackDP(wt, val, 15, 35, take) == 25747 && knapsackGreedy(wt, val, 15, 35, take) == 25096,
          "regression: 35 kg van, DP Rs 25747, greedy Rs 25096");
    check(prim(AREAS, roadKm, parent) == 54, "regression: road network 54 km");
}

int main(void) {
    srand(2026);
    testSortingSearching();
    testDivideConquer();
    testGreedy();
    testDP();
    testIntegration();
    printf("\n%d checks passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
