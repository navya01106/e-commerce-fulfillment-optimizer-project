/*
 * planner.c  -  THE SHOP AND ITS DAILY PLAN (code)
 * ---------------------------------------------------------------
 * Step 1  Choose a fulfilment centre (hub) for each order   -> greedy (nearest hub with space)
 * Step 2  Split each hub's orders into vans                 -> interval partitioning
 * Step 3  Give each group the right van                     -> greedy (heaviest load, biggest van)
 * Step 4  Drive each van's route and time every stop        -> shortest paths (Dijkstra)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planner.h"

/* =====================================================================
 *  THE MAP: 10 areas of Ahmedabad, two of them are our hubs
 * ===================================================================== */
const char *areaName[AREAS] = {
    "West Hub", "Satellite", "Navrangpura", "Maninagar", "Bopal",
    "Chandkheda", "Vastrapur", "Paldi", "Gota", "East Hub"
};

#define ROADS 16
static Edge roads[ROADS] = {          /* from, to, km (roads go both ways) */
    {0, 1, 8}, {0, 4, 6}, {0, 7, 9}, {1, 6, 3}, {1, 2, 6}, {1, 4, 5},
    {2, 6, 4}, {2, 7, 5}, {2, 5, 10}, {2, 9, 14}, {3, 7, 4}, {3, 9, 11},
    {5, 8, 7}, {5, 9, 12}, {6, 8, 9}, {4, 8, 13}
};
static int closed[MAX_V][MAX_V];      /* 1 = road closed (shock scenario) */

int roadKm[MAX_V][MAX_V];
int shortest[MAX_V][MAX_V];
static int prevOn[MAX_V][MAX_V];      /* prevOn[a][b] = area just before b on the best route from a */

void buildMap(void) {
    for (int a = 0; a < MAX_V; a++)
        for (int b = 0; b < MAX_V; b++) roadKm[a][b] = 0;
    for (int i = 0; i < ROADS; i++) {
        if (closed[roads[i].from][roads[i].to]) continue;
        roadKm[roads[i].from][roads[i].to] = roads[i].weight;
        roadKm[roads[i].to][roads[i].from] = roads[i].weight;
    }
    int dist[MAX_V], prev[MAX_V];
    for (int a = 0; a < AREAS; a++) {                 /* Dijkstra from every area */
        dijkstra(AREAS, roadKm, a, dist, prev);
        for (int b = 0; b < AREAS; b++) { shortest[a][b] = dist[b]; prevOn[a][b] = prev[b]; }
    }
}

void closeRoad(int a, int b) { closed[a][b] = closed[b][a] = 1; buildMap(); }

void reopenRoads(void) {
    for (int a = 0; a < MAX_V; a++) for (int b = 0; b < MAX_V; b++) closed[a][b] = 0;
    buildMap();
}

int findArea(const char *name) {
    for (int i = 0; i < AREAS; i++) if (strcmp(areaName[i], name) == 0) return i;
    return -1;
}

void printPath(int from, int to) {
    if (to == from) { printf("%s", areaName[from]); return; }
    if (prevOn[from][to] == -1) { printf("(no route)"); return; }
    printPath(from, prevOn[from][to]);
    printf(" -> %s", areaName[to]);
}

void printTime(int minutes) { printf("%02d:%02d", minutes / 60, minutes % 60); }


/* =====================================================================
 *  HUBS AND VANS
 * ===================================================================== */
Hub hubs[HUBS];
Van vans[OWNED_VANS];

void resetFleet(void) {
    Hub h[HUBS] = { {"West Hub", 0, 12}, {"East Hub", 9, 8} };
    Van v[OWNED_VANS] = { {1, 0, 60, 1}, {2, 0, 45, 1}, {3, 1, 30, 1}, {4, 1, 30, 1} };
    for (int i = 0; i < HUBS; i++) hubs[i] = h[i];
    for (int i = 0; i < OWNED_VANS; i++) vans[i] = v[i];
}


/* =====================================================================
 *  READING FILES (every row is checked; bad rows are skipped with a reason)
 * ===================================================================== */
Order orders[MAX_ORDERS];
int orderCount = 0;

int loadOrders(const char *fileName) {
    FILE *f = fopen(fileName, "r");
    if (f == NULL) { printf("Cannot open %s\n", fileName); return -1; }
    char line[256];
    if (fgets(line, sizeof(line), f) == NULL) { fclose(f); orderCount = 0; return 0; }   /* heading */
    int n = 0, lineNumber = 1;
    while (fgets(line, sizeof(line), f) != NULL && n < MAX_ORDERS) {
        lineNumber++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        Order o;
        int got = sscanf(line, "%d,%29[^,],%29[^,],%lf,%lf,%d,%d", &o.id, o.customer, o.area,
                         &o.weight, &o.value, &o.slotStart, &o.slotEnd);
        const char *problem = NULL;
        if (got != 7) problem = "wrong format";
        else if (o.weight <= 0) problem = "weight must be more than 0";
        else if (o.value < 0) problem = "value cannot be negative";
        else if (o.slotStart < 0 || o.slotEnd > 24 || o.slotStart >= o.slotEnd) problem = "bad time slot";
        else if (findArea(o.area) <= 0 || findArea(o.area) == 9) problem = "unknown delivery area";
        else {
            for (int i = 0; i < n; i++) if (orders[i].id == o.id) { problem = "duplicate order ID"; break; }
        }
        if (problem) { printf("  Line %d skipped: %s\n", lineNumber, problem); continue; }
        orders[n++] = o;
    }
    fclose(f);
    orderCount = n;
    return n;
}

/* =====================================================================
 *  STEP 1: CHOOSE A FULFILMENT CENTRE (HUB) FOR EACH ORDER
 * ===================================================================== */
static int inZone(int hub, int area) {
    return shortest[hubs[hub].area][area] <= ZONE_KM;
}

/* Greedy: most urgent order first (merge sort by slot end); send it to the NEAREST hub
 * that still has space and covers that area. If no hub can take it, it is postponed.
 * Limitation: an early choice can block a later order that only one hub can reach. */
int assignCentresGreedy(int hubOfOut[]) {
    static Order sorted[MAX_ORDERS];
    int used[HUBS] = {0}, served = 0;
    for (int i = 0; i < orderCount; i++) sorted[i] = orders[i];
    mergeSort(sorted, 0, orderCount - 1, BY_END);
    for (int k = 0; k < orderCount; k++) {
        int i = 0;
        while (orders[i].id != sorted[k].id) i++;    /* position in the original list */
        int area = findArea(orders[i].area), best = -1;
        for (int h = 0; h < HUBS; h++) {
            steps++;
            if (!inZone(h, area) || used[h] >= hubs[h].capacity) continue;
            if (best == -1 || shortest[hubs[h].area][area] < shortest[hubs[best].area][area]) best = h;
        }
        hubOfOut[i] = best;
        if (best != POSTPONED) { used[best]++; served++; }
    }
    return served;
}

int totalHubKm(int hubOfOut[]) {
    int km = 0;
    for (int i = 0; i < orderCount; i++)
        if (hubOfOut[i] != POSTPONED) km += shortest[hubs[hubOfOut[i]].area][findArea(orders[i].area)];
    return km;
}


/* =====================================================================
 *  STEPS 2-4: PLAN EVERY VAN
 * ===================================================================== */
int hubOf[MAX_ORDERS], vanOf[MAX_ORDERS], arriveAt[MAX_ORDERS];
VanPlan vanPlan[MAX_VANS];
int vanPlanCount = 0;

static Order hubList[MAX_ORDERS];     /* one hub's orders, sorted by slot start */
static int hubIndex[MAX_ORDERS];      /* position of hubList[k] in orders[] */
static int groupOf[MAX_ORDERS];

void planDay(void) {
    static double groupLoad[MAX_VANS];
    static int groupOrder[MAX_VANS], groupVan[MAX_VANS];
    assignCentresGreedy(hubOf);                      /* Step 1: a hub for every order */
    for (int i = 0; i < orderCount; i++) { vanOf[i] = -1; arriveAt[i] = -1; }
    vanPlanCount = 0;

    for (int h = 0; h < HUBS; h++) {
        int m = 0;                                     /* this hub's orders */
        for (int i = 0; i < orderCount; i++) if (hubOf[i] == h) hubList[m++] = orders[i];
        if (m == 0) continue;

        /* Step 2: interval partitioning -> fewest vans with no clashing slots */
        int groups = intervalPartitioning(hubList, m, groupOf);
        if (vanPlanCount + groups > MAX_VANS) groups = MAX_VANS - vanPlanCount;
        for (int k = 0; k < m; k++) {                  /* where is each order in orders[]? */
            int i = 0;
            while (orders[i].id != hubList[k].id) i++;
            hubIndex[k] = i;
        }

        /* Step 3: heaviest group gets the biggest working van of this hub; extra groups get hired vans */
        for (int g = 0; g < groups; g++) { groupLoad[g] = 0; groupOrder[g] = g; }
        for (int k = 0; k < m; k++) if (groupOf[k] < groups) groupLoad[groupOf[k]] += hubList[k].weight;
        for (int a = 0; a < groups - 1; a++)
            for (int b = a + 1; b < groups; b++)
                if (groupLoad[groupOrder[b]] > groupLoad[groupOrder[a]]) { int x = groupOrder[a]; groupOrder[a] = groupOrder[b]; groupOrder[b] = x; }
        int ownVans[OWNED_VANS], own = 0;
        for (int v = 0; v < OWNED_VANS; v++) if (vans[v].hub == h && vans[v].working) ownVans[own++] = v;
        for (int a = 0; a < own - 1; a++)
            for (int b = a + 1; b < own; b++)
                if (vans[ownVans[b]].capacity > vans[ownVans[a]].capacity) { int x = ownVans[a]; ownVans[a] = ownVans[b]; ownVans[b] = x; }
        for (int r = 0; r < groups; r++) {
            int g = groupOrder[r];
            VanPlan p = {h, 0, HIRED_KG, groupLoad[g], 0, 0, 0};
            if (r < own) { p.vanId = vans[ownVans[r]].id; p.capacity = vans[ownVans[r]].capacity; }
            groupVan[g] = vanPlanCount;
            vanPlan[vanPlanCount++] = p;
        }

        /* Step 4: each van visits its stops in slot order along the shortest roads */
        for (int g = 0; g < groups; g++) {
            VanPlan *p = &vanPlan[groupVan[g]];
            int here = hubs[h].area, time = -1;
            for (int k = 0; k < m; k++) {
                if (groupOf[k] != g) continue;
                int stop = findArea(hubList[k].area), i = hubIndex[k];
                if (time == -1) {                      /* leave so we arrive as the first slot opens */
                    time = hubList[k].slotStart * 60 - shortest[here][stop] * MIN_PER_KM;
                    if (time < 8 * 60) time = 8 * 60;  /* hubs open at 08:00 */
                    p->leaves = time;
                }
                time += shortest[here][stop] * MIN_PER_KM;
                if (time < hubList[k].slotStart * 60) time = hubList[k].slotStart * 60;   /* early: wait */
                arriveAt[i] = time;
                vanOf[i] = groupVan[g];
                if (time > hubList[k].slotEnd * 60) p->late++;
                p->km += shortest[here][stop];
                time += HANDOVER_MIN;
                here = stop;
            }
            p->km += shortest[here][hubs[h].area];     /* drive back to the hub */
        }
    }
}

int hiredVans(void) { int c = 0; for (int v = 0; v < vanPlanCount; v++) if (vanPlan[v].vanId == 0) c++; return c; }
int servedCount(void) { int c = 0; for (int i = 0; i < orderCount; i++) if (vanOf[i] != -1) c++; return c; }
int totalVanKm(void) { int k = 0; for (int v = 0; v < vanPlanCount; v++) k += vanPlan[v].km; return k; }
int onTimeCount(void) {
    int c = 0;
    for (int i = 0; i < orderCount; i++) if (vanOf[i] != -1 && arriveAt[i] <= orders[i].slotEnd * 60) c++;
    return c;
}

void printPlan(int showRoutes) {
    int perHub[HUBS] = {0}, postponed = 0;
    for (int i = 0; i < orderCount; i++) { if (hubOf[i] == POSTPONED) postponed++; else perHub[hubOf[i]]++; }
    printf("\nFulfilment centres: %s %d orders (limit %d) | %s %d orders (limit %d) | postponed %d\n",
           hubs[0].name, perHub[0], hubs[0].capacity, hubs[1].name, perHub[1], hubs[1].capacity, postponed);
    printf("Vans used: %d (own %d, hired %d) | on time: %d of %d served | total driving: %d km\n",
           vanPlanCount, vanPlanCount - hiredVans(), hiredVans(), onTimeCount(), servedCount(), totalVanKm());

    for (int v = 0; v < vanPlanCount; v++) {
        VanPlan *p = &vanPlan[v];
        if (p->vanId) printf("\n=== Van %d (own, %s, %.0f kg) leaves at ", p->vanId, hubs[p->hub].name, p->capacity);
        else printf("\n=== HIRED van (%s, %.0f kg) leaves at ", hubs[p->hub].name, p->capacity);
        printTime(p->leaves);
        printf(" ===\n");
        int here = hubs[p->hub].area;
        /* print stops sorted by arrival time (simple selection) */
        int printed[64], count = 0;
        while (1) {
            int pick = -1;
            for (int j = 0; j < orderCount; j++) {
                if (vanOf[j] != v) continue;
                int done = 0;
                for (int q = 0; q < count; q++) if (printed[q] == j) done = 1;
                if (!done && (pick == -1 || arriveAt[j] < arriveAt[pick])) pick = j;
            }
            if (pick == -1 || count >= 64) break;
            printed[count++] = pick;
            int stop = findArea(orders[pick].area);
            printf("  ");
            printTime(arriveAt[pick]);
            printf("  #%d %-10s to %-11s slot %2d-%2d  %s\n", orders[pick].id, orders[pick].customer,
                   orders[pick].area, orders[pick].slotStart, orders[pick].slotEnd,
                   arriveAt[pick] <= orders[pick].slotEnd * 60 ? "on time" : "LATE");
            if (showRoutes) {
                printf("         route: ");
                if (stop == here) printf("same area, no driving\n");
                else { printPath(here, stop); printf(" (%d km)\n", shortest[here][stop]); }
            }
            here = stop;
        }
        printf("  Total %d km | load %.1f of %.0f kg %s\n", p->km, p->load, p->capacity,
               p->load <= p->capacity ? "(OK)" : "(OVERLOADED - split the load)");
    }
    if (postponed) {
        printf("\nPostponed (no hub could take them today):");
        for (int i = 0; i < orderCount; i++) if (hubOf[i] == POSTPONED) printf(" #%d", orders[i].id);
        printf("\n");
    }
}

const char *orderStatus(int i, int hourNow, char text[]) {
    int now = hourNow * 60;
    if (vanOf[i] == -1) { sprintf(text, "POSTPONED to tomorrow (no hub had space)"); return text; }
    VanPlan *p = &vanPlan[vanOf[i]];
    char van[40];
    if (p->vanId) sprintf(van, "Van %d", p->vanId); else sprintf(van, "a hired van");
    if (now < p->leaves)
        sprintf(text, "PACKED at %s, %s leaves at %02d:%02d", hubs[p->hub].name, van, p->leaves / 60, p->leaves % 60);
    else if (now < arriveAt[i])
        sprintf(text, "OUT FOR DELIVERY in %s, expected at %02d:%02d", van, arriveAt[i] / 60, arriveAt[i] % 60);
    else
        sprintf(text, "DELIVERED at %02d:%02d by %s", arriveAt[i] / 60, arriveAt[i] % 60, van);
    return text;
}

int exportPlan(const char *csvFile, const char *jsonFile) {
    FILE *c = fopen(csvFile, "w"), *j = fopen(jsonFile, "w");
    if (c == NULL || j == NULL) { if (c) fclose(c); if (j) fclose(j); return 0; }
    fprintf(c, "order_id,customer,area,weight_kg,hub,van,arrival,slot_start,slot_end,on_time\n");
    fprintf(j, "{\n  \"orders\": [\n");
    for (int i = 0; i < orderCount; i++) {
        Order o = orders[i];
        char hub[20] = "postponed", van[20] = "none", arr[16] = "-";
        int onTime = 0;
        if (vanOf[i] != -1) {
            VanPlan *p = &vanPlan[vanOf[i]];
            strcpy(hub, hubs[p->hub].name);
            if (p->vanId) sprintf(van, "Van %d", p->vanId); else strcpy(van, "Hired");
            sprintf(arr, "%02d:%02d", arriveAt[i] / 60, arriveAt[i] % 60);
            onTime = arriveAt[i] <= o.slotEnd * 60;
        }
        fprintf(c, "%d,%s,%s,%.1f,%s,%s,%s,%d,%d,%s\n", o.id, o.customer, o.area, o.weight, hub, van, arr,
                o.slotStart, o.slotEnd, onTime ? "yes" : "no");
        fprintf(j, "    {\"order_id\": %d, \"customer\": \"%s\", \"area\": \"%s\", \"weight_kg\": %.1f, "
                   "\"hub\": \"%s\", \"van\": \"%s\", \"arrival\": \"%s\", \"slot\": \"%d-%d\", \"on_time\": %s}%s\n",
                o.id, o.customer, o.area, o.weight, hub, van, arr, o.slotStart, o.slotEnd,
                onTime ? "true" : "false", i < orderCount - 1 ? "," : "");
    }
    fprintf(j, "  ],\n  \"summary\": {\"orders\": %d, \"served\": %d, \"on_time\": %d, \"vans\": %d, \"hired_vans\": %d, \"total_km\": %d}\n}\n",
            orderCount, servedCount(), onTimeCount(), vanPlanCount, hiredVans(), totalVanKm());
    fclose(c);
    fclose(j);
    return 1;
}
