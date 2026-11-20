/*
 * main.c  -  THE ORDER-TO-DELIVERY PLANNING CONSOLE
 * ---------------------------------------------------------------
 * A menu that (1) builds the daily delivery plan, (2) demonstrates each
 * algorithm topic on shop data with a comparison of strategies,
 * (3) runs the three demonstration scenarios, and (4) runs the benchmark.
 * No algorithm is written here: they all come from daa.c and planner.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "daa.h"
#include "planner.h"

static char ordersFile[200] = "data/orders.csv";

/* ---------------- small helpers ---------------- */
static double nowMs(void) { return 1000.0 * (double)clock() / CLOCKS_PER_SEC; }

static int readInt(const char *question, int *x) {
    printf("%s", question);
    if (scanf("%d", x) == 1) return 1;
    int c;
    while ((c = getchar()) != '\n' && c != EOF) ;
    printf("Please type a number.\n");
    return 0;
}

static void title(const char *t) { printf("\n---------- %s ----------\n", t); }

/* one row of a comparison table */
static void row(const char *strategy, const char *quality, long long work, long long bytes, double ms) {
    printf("  %-28s %-26s %12lld %11lld %9.3f\n", strategy, quality, work, bytes, ms);
}
/* parcels (orders) as whole-kg weights and rupee values, for the knapsack problems */
static int parcelsFromOrders(int wt[], int val[], int maxN) {
    int n = orderCount < maxN ? orderCount : maxN;
    for (int i = 0; i < n; i++) { wt[i] = (int)(orders[i].weight + 0.999); val[i] = (int)orders[i].value; }
    return n;
}

static void printIds(int take[], int n) {
    for (int i = 0; i < n; i++) if (take[i]) printf(" #%d", orders[i].id);
    printf("\n");
}

static void tableHead(void) {
    printf("  %-28s %-26s %12s %11s %9s\n", "Strategy", "Result", "Steps", "Memory(B)", "Time(ms)");
}


/* =====================================================================
 *  FIXED SHOP DATA USED BY THE MODULE DEMOS
 * ===================================================================== */
#define DAYS 14
static long long profit[DAYS] = {12, -8, -15, 20, -3, -16, -23, 18, 20, -7, 12, -5, -22, 15};
static const char *dayName[DAYS] = {"Nov-01", "Nov-02", "Nov-03", "Nov-04", "Nov-05", "Nov-06", "Nov-07",
                                    "Nov-08", "Nov-09", "Nov-10", "Nov-11", "Nov-12", "Nov-13", "Nov-14"};

#define SUPPLIERS 6
static const char *supplierName[SUPPLIERS] = {"PackRight Cartons", "Gujarat Textiles", "SabarPrint Labels",
                                              "Ahmedabad Electronics", "Narmada Foods", "KiteTape Packaging"};
static int dependsOn[SUPPLIERS * SUPPLIERS] = {   /* row depends on column */
    0, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0,
    1, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 0
};

#define GOODS 6
static Goods bulk[GOODS] = {
    {"Rice", 40, 2400}, {"Sugar", 25, 1100}, {"Detergent", 20, 3000},
    {"Tea leaves", 10, 4000}, {"Cooking oil", 35, 5250}, {"Atta", 35, 1400}
};


/* =====================================================================
 *  DAILY PLAN (options 1-6)
 * ===================================================================== */
static void loadData(void) {
    char name[200];
    printf("CSV file to load (type d for %s): ", ordersFile);
    if (scanf("%199s", name) != 1) return;
    if (strcmp(name, "d") != 0) strcpy(ordersFile, name);
    int n = loadOrders(ordersFile);
    if (n < 0) { printf("Could not load. Keeping the old data.\n"); return; }
    printf("Loaded %d valid orders from %s.\n", n, ordersFile);
    planDay();
}

static void showOrders(void) {
    printf("\n%-5s %-10s %-12s %6s %7s %7s\n", "ID", "Customer", "Area", "Kg", "Rs", "Slot");
    for (int i = 0; i < orderCount && i < 60; i++)
        printf("%-5d %-10s %-12s %6.1f %7.0f %4d-%d\n", orders[i].id, orders[i].customer, orders[i].area,
               orders[i].weight, orders[i].value, orders[i].slotStart, orders[i].slotEnd);
    if (orderCount > 60) printf("... and %d more\n", orderCount - 60);
}

static void showCentres(void) {
    title("Choose a fulfilment centre for each order (greedy)");
    printf("Hubs: %s (limit %d orders), %s (limit %d). A hub only serves areas within %d km.\n",
           hubs[0].name, hubs[0].capacity, hubs[1].name, hubs[1].capacity, ZONE_KM);
    printf("Rule: most urgent order first; send it to the NEAREST hub that still has space.\n");
    resetCounters();
    double t = nowMs();
    int served = assignCentresGreedy(hubOf);
    t = nowMs() - t;
    printf("Served %d of %d orders, %d km from hubs to customers, %lld steps, %.3f ms\n",
           served, orderCount, totalHubKm(hubOf), steps, t);
    for (int i = 0; i < orderCount && i < 30; i++) {
        printf("  #%d %-12s -> %s", orders[i].id, orders[i].area,
               hubOf[i] == POSTPONED ? "POSTPONED" : hubs[hubOf[i]].name);
        if (hubOf[i] != POSTPONED) printf(" (%d km)", shortest[hubs[hubOf[i]].area][findArea(orders[i].area)]);
        printf("\n");
    }
    planDay();
}

static void trackOrder(void) {
    static Order byId[MAX_ORDERS];
    int id, hour;
    if (!readInt("Order ID (e.g. 110): ", &id)) return;
    for (int i = 0; i < orderCount; i++) byId[i] = orders[i];
    mergeSort(byId, 0, orderCount - 1, BY_ID);       /* the order index: sorted by ID */
    resetCounters();
    int pos = binarySearchIterative(byId, orderCount, id);
    printf("Binary search: %lld steps.\n", steps);
    if (pos == -1) { printf("Order %d not found.\n", id); return; }
    int i = 0;
    while (orders[i].id != id) i++;
    printf("Order %d: %s, %s, %.1f kg, slot %d:00-%d:00\n", id, orders[i].customer, orders[i].area,
           orders[i].weight, orders[i].slotStart, orders[i].slotEnd);
    if (!readInt("Time now (hour 0-23): ", &hour) || hour < 0 || hour > 23) { printf("Hour must be 0-23.\n"); return; }
    char text[120];
    printf("Status at %02d:00 -> %s\n", hour, orderStatus(i, hour, text));
}

static void exportNow(void) {
    if (exportPlan("results/plan.csv", "results/plan.json")) printf("Saved results/plan.csv and results/plan.json\n");
    else printf("Could not write to the results folder.\n");
}


/* =====================================================================
 *  MODULE DEMOS (options 7-10; each compares at least two strategies)
 * ===================================================================== */
static void moduleSorting(void) {
    static Order a[MAX_ORDERS], b[MAX_ORDERS];
    char q[40];
    title("Sorting: arrange orders by urgency");
    tableHead();
    for (int i = 0; i < orderCount; i++) { a[i] = orders[i]; b[i] = orders[i]; }
    resetCounters(); double t = nowMs(); mergeSort(a, 0, orderCount - 1, BY_END); t = nowMs() - t;
    sprintf(q, "sorted: %s, stable", isSorted(a, orderCount, BY_END) ? "yes" : "NO");
    row("Merge sort", q, steps, memPeak, t);
    resetCounters(); t = nowMs(); quickSort(b, 0, orderCount - 1, BY_END); t = nowMs() - t;
    sprintf(q, "sorted: %s, not stable", isSorted(b, orderCount, BY_END) ? "yes" : "NO");
    row("Quick sort", q, steps, memPeak, t);
    printf("Most urgent first:");
    for (int i = 0; i < orderCount && i < 15; i++) printf(" #%d(%d)", a[i].id, a[i].slotEnd);
    printf("\n");

    title("Searching: find an order by ID");
    int id;
    if (!readInt("Order ID to find (e.g. 105): ", &id)) return;
    for (int i = 0; i < orderCount; i++) a[i] = orders[i];
    mergeSort(a, 0, orderCount - 1, BY_ID);
    tableHead();
    resetCounters(); int p1 = linearSearch(orders, orderCount, id);
    row("Linear search", p1 >= 0 ? "found" : "not found", steps, memPeak, 0);
    resetCounters(); int p2 = binarySearchIterative(a, orderCount, id);
    row("Binary search (loop)", p2 >= 0 ? "found" : "not found", steps, memPeak, 0);
    resetCounters(); int p3 = binarySearchRecursive(a, 0, orderCount - 1, id);
    row("Binary search (recursive)", p3 >= 0 ? "found" : "not found", steps, memPeak, 0);
    printf("Same answer: %s. Recursive uses stack memory; the loop does not.\n",
           ((p1 >= 0) == (p2 >= 0) && p2 == p3) ? "yes" : "NO");
}

static void moduleDivide(void) {
    char q[60];
    title("Maximum subarray: best run of profit days");
    tableHead();
    resetCounters(); Streak s1 = maxSubarraySimple(profit, DAYS);
    sprintf(q, "%lld thousand Rs", s1.sum); row("Simple (every start/end)", q, steps, memPeak, 0);
    resetCounters(); Streak s2 = maxSubarrayDivide(profit, 0, DAYS - 1);
    sprintf(q, "%lld thousand Rs", s2.sum); row("Divide and conquer", q, steps, memPeak, 0);
    printf("Best streak: %s to %s.\n", dayName[s2.start], dayName[s2.end]);

    title("Fast power: order growth forecast");
    tableHead();
    resetCounters(); double g1 = powerSimple(1.05, 30);
    sprintf(q, "%.0f orders", 1000 * g1); row("Simple power", q, steps, memPeak, 0);
    resetCounters(); double g2 = powerFast(1.05, 30);
    sprintf(q, "%.0f orders", 1000 * g2); row("Fast power", q, steps, memPeak, 0);
    printf("1000 orders today, +5%% a day, after 30 days.\n");

    title("Celebrity: the key supplier everyone depends on");
    tableHead();
    resetCounters(); int c1 = celebritySimple(dependsOn, SUPPLIERS);
    row("Simple (every pair)", c1 >= 0 ? supplierName[c1] : "none", steps, memPeak, 0);
    resetCounters(); int c2 = celebrityDivide(dependsOn, SUPPLIERS);
    row("Divide and conquer", c2 >= 0 ? supplierName[c2] : "none", steps, memPeak, 0);
    printf("Risk: if it fails, every other supplier is affected. Keep a backup supplier.\n");

    title("Matrix multiplication: count 2-road routes (A x A)");
    static long long A[AREAS * AREAS], C1[AREAS * AREAS], C2[AREAS * AREAS];
    for (int a = 0; a < AREAS; a++) for (int b = 0; b < AREAS; b++) A[a * AREAS + b] = roadKm[a][b] ? 1 : 0;
    tableHead();
    resetCounters(); matrixClassical(A, A, C1, AREAS); row("Classical", "10x10 table", steps, memPeak, 0);
    resetCounters(); matrixStrassen(A, A, C2, AREAS, 1); row("Strassen (padded to 16x16)", "10x10 table", steps, memPeak, 0);
    printf("Same answer: %s. Example: %d different 2-road routes from West Hub to Navrangpura.\n",
           memcmp(C1, C2, sizeof(C1)) == 0 ? "yes" : "NO", (int)C1[0 * AREAS + 2]);
    printf("Strassen does fewer multiplications only on big matrices (see the benchmark).\n");
}

static void moduleGreedy(void) {
    static Order a[MAX_ORDERS];
    static int chosen[MAX_ORDERS], groups[MAX_ORDERS];
    title("Activity selection: most slots ONE van can serve");
    resetCounters();
    int k = activitySelection(orders, orderCount, chosen);
    printf("One van can serve %d of %d slots without clashes:", k, orderCount);
    for (int i = 0; i < k; i++) printf(" #%d", chosen[i]);
    printf("  (%lld steps)\n", steps);

    title("Interval partitioning: fewest vans for ALL slots (one hub)");
    for (int i = 0; i < orderCount; i++) a[i] = orders[i];
    resetCounters();
    int vansAll = intervalPartitioning(a, orderCount, groups);
    printf("If one hub served everything: %d vans (that many slots overlap at once). %lld steps.\n", vansAll, steps);
    printf("Our two-hub plan (option 4) uses %d vans.\n", vanPlanCount);

    title("Fractional knapsack: load bulk goods into a 60 kg van");
    Goods g[GOODS];
    double taken[GOODS];
    for (int i = 0; i < GOODS; i++) g[i] = bulk[i];
    double total = fractionalKnapsack(g, GOODS, 60, taken);
    for (int i = 0; i < GOODS; i++)
        if (taken[i] > 0) printf("  %-12s %5.1f of %4.1f kg (Rs %.0f/kg)\n", g[i].name, taken[i], g[i].kg, g[i].rupees / g[i].kg);
    printf("Total Rs %.0f. Greedy is optimal because goods can be split.\n", total);

    title("Huffman coding: compress the order log");
    FILE *f = fopen("data/order_log.txt", "r");
    static char text[100000];
    int len = 0;
    if (f) { len = (int)fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[len] = '\0';
    static int freq[256];
    static char code[256][MAX_CODE];
    resetCounters();
    long long bits = huffmanBuild(text, freq, code);
    printf("Normal: %d bits, Huffman: %lld bits, saved %.1f%%. Decodes back exactly: %s\n",
           len * 8, bits, len ? 100.0 * (len * 8 - bits) / (len * 8) : 0.0, huffmanRoundTrip(text, code) ? "yes" : "NO");
    int most = 0, least = -1;
    for (int ch = 0; ch < 256; ch++) {
        if (freq[ch] > freq[most]) most = ch;
        if (freq[ch] > 0 && (least == -1 || freq[ch] < freq[least])) least = ch;
    }
    if (least >= 0)
        printf("Most common character '%c' (%d times) gets the short code %s; rarest '%c' (%d times) gets %s.\n",
               most == ' ' ? '_' : most, freq[most], code[most], least, freq[least], code[least]);
    printf("('_' means the space character.)\n");

    title("Dijkstra: shortest route from West Hub");
    int dist[MAX_V], prev[MAX_V];
    dijkstra(AREAS, roadKm, 0, dist, prev);
    for (int i = 1; i < AREAS; i++) { printf("  %-12s %3d km : ", areaName[i], dist[i]); printPath(0, i); printf("\n"); }

    title("Prim and Kruskal: cheapest road network (repair first after floods)");
    static Edge e[MAX_V * MAX_V], ch[MAX_V];
    int m = 0, cnt, parent[MAX_V], all = 0;
    for (int x = 0; x < AREAS; x++)
        for (int y = x + 1; y < AREAS; y++) if (roadKm[x][y]) { Edge ed = {x, y, roadKm[x][y]}; e[m++] = ed; all += roadKm[x][y]; }
    tableHead();
    char q[40];
    resetCounters(); int p = prim(AREAS, roadKm, parent);
    sprintf(q, "%d km of %d km", p, all); row("Prim", q, steps, memPeak, 0);
    resetCounters(); int kr = kruskal(AREAS, e, m, ch, &cnt);
    sprintf(q, "%d km of %d km", kr, all); row("Kruskal", q, steps, memPeak, 0);
    for (int i = 0; i < cnt; i++) printf("  keep %s - %s (%d km)\n", areaName[ch[i].from], areaName[ch[i].to], ch[i].weight);

}



/* ---------------- option 10: dynamic programming ---------------- */
static void moduleDP(void) {
    int wt[MAX_V], val[MAX_V], t1[MAX_V], t2[MAX_V];
    char q[60];
    int n = parcelsFromOrders(wt, val, 30);
    title("0/1 knapsack: which parcels go into a 35 kg van (cannot split)");
    tableHead();
    resetCounters(); double t = nowMs(); int vg = knapsackGreedy(wt, val, n, 35, t1); t = nowMs() - t;
    sprintf(q, "Rs %d", vg); row("Greedy (best Rs/kg first)", q, steps, memPeak, t);
    resetCounters(); t = nowMs(); int vd = knapsackDP(wt, val, n, 35, t2); t = nowMs() - t;
    sprintf(q, "Rs %d (optimal)", vd); row("Dynamic programming", q, steps, memPeak, t);
    printf("Greedy took:"); printIds(t1, n);
    printf("DP took:    "); printIds(t2, n);
    if (vd > vg) printf("Greedy lost Rs %d: it filled the van with small parcels, so the big valuable one no longer fit.\n", vd - vg);
    printf("DP table: best[i][w] = most rupees from the first i parcels with w kg of space.\n");
    printf("For each parcel there are two choices, take it or leave it; each answer is computed once.\n");

    title("Matrix chain: cheapest order to multiply the monthly cost tables");
    int dims[5] = {15, 8, 2, 5, 1};
    char order[200];
    resetCounters();
    long long best = matrixChain(dims, 4, order);
    long long leftToRight = 15 * 8 * 2 + 15 * 2 * 5 + 15 * 5 * 1;
    printf("Tables: orders(15x8) x areas(8x2) x hubs(2x5) x costs(5x1)\n");
    printf("Left to right: %lld multiplications. Best order %s: %lld multiplications (%lld DP steps).\n",
           leftToRight, order, best, steps);
    printf("The answer is the same either way; only the amount of work changes.\n");
}


/* =====================================================================
 *  SCENARIOS (options 11-13)
 * ===================================================================== */
/* Compare strategies on whatever data is loaded now */
static void compareStrategies(void) {
    static Order a[MAX_ORDERS], b[MAX_ORDERS];
    static Edge e[MAX_V * MAX_V], ch[MAX_V];
    char q[60];
    double t;
    printf("\nSame input, different strategies:\n");
    tableHead();
    for (int i = 0; i < orderCount; i++) { a[i] = orders[i]; b[i] = orders[i]; }
    resetCounters(); t = nowMs(); mergeSort(a, 0, orderCount - 1, BY_END); t = nowMs() - t;
    row("Sorting: merge sort", isSorted(a, orderCount, BY_END) ? "sorted" : "NOT sorted", steps, memPeak, t);
    resetCounters(); t = nowMs(); quickSort(b, 0, orderCount - 1, BY_END); t = nowMs() - t;
    row("Sorting: quick sort", isSorted(b, orderCount, BY_END) ? "sorted" : "NOT sorted", steps, memPeak, t);

    mergeSort(a, 0, orderCount - 1, BY_ID);          /* index for binary search */
    int found;
    resetCounters(); t = nowMs(); found = 0;
    for (int k = 0; k < 100; k++) if (linearSearch(orders, orderCount, orders[(k * 7) % orderCount].id) >= 0) found++;
    t = nowMs() - t;
    sprintf(q, "%d of 100 found", found); row("100 lookups: linear", q, steps, memPeak, t);
    resetCounters(); t = nowMs(); found = 0;
    for (int k = 0; k < 100; k++) if (binarySearchIterative(a, orderCount, orders[(k * 7) % orderCount].id) >= 0) found++;
    t = nowMs() - t;
    sprintf(q, "%d of 100 found", found); row("100 lookups: binary", q, steps, memPeak, t);

    int wt[MAX_V], val[MAX_V], take[MAX_V];
    int n = parcelsFromOrders(wt, val, 30);
    resetCounters(); t = nowMs(); int vg = knapsackGreedy(wt, val, n, 35, take); t = nowMs() - t;
    sprintf(q, "Rs %d", vg); row("Loading 35 kg: greedy", q, steps, memPeak, t);
    resetCounters(); t = nowMs(); int vd = knapsackDP(wt, val, n, 35, take); t = nowMs() - t;
    sprintf(q, "Rs %d (optimal)", vd); row("Loading 35 kg: DP", q, steps, memPeak, t);

    int m = 0, cnt, parent[MAX_V];
    for (int x = 0; x < AREAS; x++)
        for (int y = x + 1; y < AREAS; y++) if (roadKm[x][y]) { Edge ed = {x, y, roadKm[x][y]}; e[m++] = ed; }
    resetCounters(); t = nowMs(); int p = prim(AREAS, roadKm, parent); t = nowMs() - t;
    sprintf(q, "%d km network", p); row("Road network: Prim", q, steps, memPeak, t);
    resetCounters(); t = nowMs(); int k = kruskal(AREAS, e, m, ch, &cnt); t = nowMs() - t;
    sprintf(q, "%d km network", k); row("Road network: Kruskal", q, steps, memPeak, t);
}

static void scenarioNormal(void) {
    title("SCENARIO 1: a normal day");
    resetFleet(); reopenRoads();
    planDay();
    printPlan(1);
    compareStrategies();
}

static void scenarioShock(void) {
    title("SCENARIO 2: capacity and constraint shock");
    resetFleet(); reopenRoads();
    planDay();
    int s0 = servedCount(), v0 = vanPlanCount, h0 = hiredVans(), k0 = totalVanKm();
    printf("Shock: East Hub power cut (capacity 8 -> 2), Van 2 broken, Satellite-Navrangpura road closed.\n");
    hubs[1].capacity = 2;
    vans[1].working = 0;
    closeRoad(1, 2);
    planDay();
    printPlan(1);
    compareStrategies();
    printf("\n  %-24s %10s %10s\n", "", "Normal", "Shock");
    printf("  %-24s %10d %10d\n", "Orders served", s0, servedCount());
    printf("  %-24s %10d %10d\n", "Vans used", v0, vanPlanCount);
    printf("  %-24s %10d %10d\n", "Hired vans", h0, hiredVans());
    printf("  %-24s %10d %10d\n", "Total km", k0, totalVanKm());
    printf("Orders no hub can take are postponed; Dijkstra reroutes the vans around the closed road.\n");
    resetFleet(); reopenRoads();
    planDay();
    printf("(Normal settings restored.)\n");
}

static Order backup[MAX_ORDERS];

static void scenarioStress(void) {
    title("SCENARIO 3: large-data stress test (10,000 orders)");
    int saved = orderCount;
    for (int i = 0; i < saved; i++) backup[i] = orders[i];
    srand(7);
    int n = 10000;
    for (int i = 0; i < n; i++) {
        Order o;
        o.id = 100000 + i;
        sprintf(o.customer, "C%d", i);
        strcpy(o.area, areaName[1 + rand() % 8]);
        o.weight = 0.5 + rand() % 200 / 10.0;
        o.value = 99 + rand() % 15000;
        o.slotStart = 8 + rand() % 12;
        o.slotEnd = o.slotStart + 2 + rand() % 3;
        if (o.slotEnd > 24) o.slotEnd = 24;
        orders[i] = o;
    }
    orderCount = n;
    hubs[0].capacity = 6000;
    hubs[1].capacity = 4500;
    double t = nowMs();
    resetCounters();
    planDay();
    double planMs = nowMs() - t;
    printf("Planned %d orders in %.1f ms: served %d, on time %d, vans %d (hired %d), %d km.\n",
           n, planMs, servedCount(), onTimeCount(), vanPlanCount, hiredVans(), totalVanKm());
    compareStrategies();

    for (int i = 0; i < saved; i++) orders[i] = backup[i];
    orderCount = saved;
    resetFleet();
    planDay();
    printf("(Original orders restored.)\n");
}


/* =====================================================================
 *  BENCHMARK RUNNER (option 14): time, memory, quality -> results/benchmark.csv
 * ===================================================================== */
static FILE *bench;

static void benchRow(const char *problem, const char *dataset, int n, const char *strategy,
                     double ms, long long quality) {
    printf("  %-12s %-12s %6d %-22s %10.3f %11lld %12lld %12lld\n", problem, dataset, n, strategy, ms, memPeak, steps, quality);
    if (bench) fprintf(bench, "%s,%s,%d,%s,%.4f,%lld,%lld,%lld\n", problem, dataset, n, strategy, ms, memPeak, steps, quality);
}

static void runBenchmark(void) {
    static Order base[MAX_ORDERS], a[MAX_ORDERS];
    bench = fopen("results/benchmark.csv", "w");
    if (bench) fprintf(bench, "problem,dataset,n,strategy,time_ms,peak_memory_bytes,steps,quality\n");
    printf("\n  %-12s %-12s %6s %-22s %10s %11s %12s %12s\n", "problem", "dataset", "n", "strategy", "time_ms", "memory(B)", "steps", "quality");
    srand(42);
    double t;
    const char *sizeName[4] = {"small", "medium", "large", "adversarial"};

    /* 1. Sorting: quality = 1 if sorted. adversarial = already sorted (quick sort's worst case) */
    int sortN[4] = {100, 1000, 10000, 10000};
    for (int d = 0; d < 4; d++) {
        int n = sortN[d];
        for (int i = 0; i < n; i++) { base[i].id = i; base[i].slotStart = 0; base[i].slotEnd = (d == 3) ? i : rand() % 1000; }
        for (int s = 0; s < 2; s++) {
            for (int i = 0; i < n; i++) a[i] = base[i];
            resetCounters(); t = nowMs();
            if (s == 0) mergeSort(a, 0, n - 1, BY_END); else quickSort(a, 0, n - 1, BY_END);
            t = nowMs() - t;
            benchRow("sorting", sizeName[d], n, s == 0 ? "merge sort" : "quick sort", t, isSorted(a, n, BY_END));
        }
    }

    /* 2. Searching: 1000 lookups; adversarial = IDs that do not exist (worst case). quality = found */
    int searchN[4] = {100, 1000, 10000, 10000};
    for (int d = 0; d < 4; d++) {
        int n = searchN[d];
        for (int i = 0; i < n; i++) a[i].id = 2 * i;
        for (int s = 0; s < 3; s++) {
            long long foundCount = 0;
            resetCounters(); t = nowMs();
            for (int q = 0; q < 1000; q++) {
                int target = (d == 3) ? 2 * (q % n) + 1 : 2 * ((q * 7919) % n);
                int r = (s == 0) ? linearSearch(a, n, target) : (s == 1) ? binarySearchIterative(a, n, target)
                                                                         : binarySearchRecursive(a, 0, n - 1, target);
                if (r >= 0) foundCount++;
            }
            t = nowMs() - t;
            benchRow("searching", sizeName[d], n, s == 0 ? "linear" : s == 1 ? "binary loop" : "binary recursive", t, foundCount);
        }
    }

    /* 3. Maximum subarray: quality = best sum. adversarial = every day is a loss */
    static long long p[20000];
    int subN[4] = {100, 1000, 10000, 10000};
    for (int d = 0; d < 4; d++) {
        int n = subN[d];
        for (int i = 0; i < n; i++) p[i] = (d == 3) ? -(1 + rand() % 100) : rand() % 201 - 100;
        resetCounters(); t = nowMs(); Streak s1 = maxSubarraySimple(p, n); t = nowMs() - t;
        benchRow("max-subarray", sizeName[d], n, "simple", t, s1.sum);
        resetCounters(); t = nowMs(); Streak s2 = maxSubarrayDivide(p, 0, n - 1); t = nowMs() - t;
        benchRow("max-subarray", sizeName[d], n, "divide and conquer", t, s2.sum);
    }

    /* 4. Power: quality = 1 if both give the same answer */
    int powN[3] = {100, 10000, 1000000};
    for (int d = 0; d < 3; d++) {
        resetCounters(); t = nowMs(); double r1 = powerSimple(1.0000001, powN[d]); t = nowMs() - t;
        benchRow("power", sizeName[d], powN[d], "simple", t, 1);
        resetCounters(); t = nowMs(); double r2 = powerFast(1.0000001, powN[d]); t = nowMs() - t;
        double diff = r1 > r2 ? r1 - r2 : r2 - r1;
        benchRow("power", sizeName[d], powN[d], "fast", t, diff < 1e-6 * r1);
    }

    /* 5. 0/1 knapsack: quality = best value. adversarial = every item has the same Rs/kg */
    int knapN[4] = {10, 20, 30, 30};
    for (int d = 0; d < 4; d++) {
        int n = knapN[d], wt[MAX_V], val[MAX_V], take[MAX_V], total = 0;
        for (int i = 0; i < n; i++) {
            wt[i] = 1 + rand() % 30;
            val[i] = (d == 3) ? wt[i] * 10 : 10 + rand() % 500;
            total += wt[i];
        }
        int cap = total / 2 + 1;
        resetCounters(); t = nowMs(); int vg = knapsackGreedy(wt, val, n, cap, take); t = nowMs() - t;
        benchRow("knapsack", sizeName[d], n, "greedy", t, vg);
        resetCounters(); t = nowMs(); int vd = knapsackDP(wt, val, n, cap, take); t = nowMs() - t;
        benchRow("knapsack", sizeName[d], n, "dynamic programming", t, vd);
    }

    /* 6. Matrix multiplication: quality = 1 if both give the same answer */
    int matN[3] = {64, 128, 256};
    for (int d = 0; d < 3; d++) {
        int n = matN[d];
        long long *A = malloc(sizeof(long long) * n * n), *B = malloc(sizeof(long long) * n * n);
        long long *C1 = malloc(sizeof(long long) * n * n), *C2 = malloc(sizeof(long long) * n * n);
        if (!A || !B || !C1 || !C2) break;
        for (int i = 0; i < n * n; i++) { A[i] = rand() % 10; B[i] = rand() % 10; }
        resetCounters(); t = nowMs(); matrixClassical(A, B, C1, n); t = nowMs() - t;
        benchRow("matrix", sizeName[d], n, "classical", t, 1);
        resetCounters(); t = nowMs(); matrixStrassen(A, B, C2, n, 32); t = nowMs() - t;
        benchRow("matrix", sizeName[d], n, "Strassen (cutoff 32)", t, memcmp(C1, C2, sizeof(long long) * n * n) == 0);
        free(A); free(B); free(C1); free(C2);
    }
    if (bench) { fclose(bench); printf("\nSaved results/benchmark.csv\n"); }
}


/* =====================================================================
 *  MAIN MENU
 * ===================================================================== */
int main(void) {
    resetFleet();
    buildMap();
    if (loadOrders(ordersFile) <= 0) { printf("No orders loaded from %s\n", ordersFile); return 1; }
    planDay();
    printf("Loaded %d orders. Plan ready.\n", orderCount);

    int choice = -1;
    while (choice != 0) {
        printf("\n============ E-COMMERCE ORDER-TO-DELIVERY PLANNER ============\n");
        printf(" DATA AND DAILY PLAN\n");
        printf("   1. Load orders from a CSV file\n");
        printf("   2. Show orders\n");
        printf("   3. Choose fulfilment centres     (greedy: nearest hub with space)\n");
        printf("   4. Plan the vans                 (orders, routes, load, times)\n");
        printf("   5. Track an order                (binary search + status)\n");
        printf("   6. Export the plan               (CSV and JSON)\n");
        printf(" ALGORITHM MODULES (each compares strategies)\n");
        printf("   7. Sorting and searching\n");
        printf("   8. Divide and conquer\n");
        printf("   9. Greedy\n");
        printf("  10. Dynamic programming           (0/1 knapsack, matrix chain)\n");
        printf(" SCENARIOS AND BENCHMARK\n");
        printf("  11. Scenario 1: normal day\n");
        printf("  12. Scenario 2: shock (hub capacity cut, van broken, road closed)\n");
        printf("  13. Scenario 3: stress test (10,000 orders)\n");
        printf("  14. Benchmark runner (saves results/benchmark.csv)\n");
        printf("   0. Exit\n");
        if (!readInt("Choose: ", &choice)) {
            if (feof(stdin)) break;
            choice = -1;
            continue;
        }
        switch (choice) {
            case 1:  loadData(); break;
            case 2:  showOrders(); break;
            case 3:  showCentres(); break;
            case 4:  planDay(); printPlan(1); break;
            case 5:  trackOrder(); break;
            case 6:  exportNow(); break;
            case 7:  moduleSorting(); break;
            case 8:  moduleDivide(); break;
            case 9:  moduleGreedy(); break;
            case 10: moduleDP(); break;
            case 11: scenarioNormal(); break;
            case 12: scenarioShock(); break;
            case 13: scenarioStress(); break;
            case 14: runBenchmark(); break;
            case 0:  printf("Goodbye!\n"); break;
            default: printf("Please choose 0 to 14.\n");
        }
    }
    return 0;
}
