/*
 * planner.h  -  THE SHOP AND ITS DAILY PLAN (interface)
 * ---------------------------------------------------------------
 * The e-commerce "domain model": areas, roads, fulfilment centres (hubs),
 * vans and orders, plus the daily order-to-delivery plan.
 * It uses the algorithm library (daa.h) to make every decision.
 */
#ifndef PLANNER_H
#define PLANNER_H

#include "daa.h"

#define AREAS        10
#define HUBS         2
#define OWNED_VANS   4
#define MAX_VANS     5000
#define MIN_PER_KM   2     /* average speed 30 km/h = 2 minutes per km */
#define HANDOVER_MIN 10    /* minutes to hand over one parcel */
#define ZONE_KM      20    /* a hub only delivers to areas within 20 km by road */
#define HIRED_KG     30    /* capacity of any extra van we hire */

/* ---------------- the map ---------------- */
extern const char *areaName[AREAS];
extern int roadKm[MAX_V][MAX_V];    /* direct road length, 0 = no road */
extern int shortest[MAX_V][MAX_V];  /* shortest road distance between any two areas */
void buildMap(void);                /* fills roadKm and shortest (Dijkstra from every area) */
void closeRoad(int a, int b);       /* shock: a road is closed */
void reopenRoads(void);
int  findArea(const char *name);
void printPath(int from, int to);   /* prints e.g. West Hub -> Satellite -> Vastrapur */
void printTime(int minutes);        /* prints e.g. 09:30 */

/* ---------------- hubs and vans ---------------- */
typedef struct { char name[20]; int area; int capacity; } Hub;   /* capacity = orders per day */
typedef struct { int id; int hub; double capacity; int working; } Van;
extern Hub hubs[HUBS];
extern Van vans[OWNED_VANS];
void resetFleet(void);              /* hubs and vans back to normal */

/* ---------------- orders ---------------- */
extern Order orders[MAX_ORDERS];
extern int orderCount;
int loadOrders(const char *fileName);     /* returns number loaded, or -1 if the file is missing */

/* ---------------- step 1: choose a fulfilment centre for each order ---------------- */
#define POSTPONED -1
int assignCentresGreedy(int hubOf[]);     /* greedy: nearest hub with space; returns orders served */
int totalHubKm(int hubOf[]);              /* sum of hub-to-customer distances */

/* ---------------- step 2: plan every van ---------------- */
typedef struct {
    int hub;          /* which hub it leaves from */
    int vanId;        /* own van number, or 0 = hired */
    double capacity;  /* kg */
    double load;      /* kg carried */
    int leaves;       /* minutes after midnight */
    int km;           /* total km including the drive back */
    int late;         /* number of late deliveries */
} VanPlan;

extern int hubOf[MAX_ORDERS];       /* per order: hub index or POSTPONED */
extern int vanOf[MAX_ORDERS];       /* per order: index into vanPlan, or -1 */
extern int arriveAt[MAX_ORDERS];    /* per order: delivery time in minutes */
extern VanPlan vanPlan[MAX_VANS];
extern int vanPlanCount;

void planDay(void);                 /* centres + vans + routes + times */
void printPlan(int showRoutes);
int  hiredVans(void);
int  onTimeCount(void);
int  servedCount(void);
int  totalVanKm(void);
const char *orderStatus(int orderIndex, int hourNow, char text[]);   /* tracking */
int  exportPlan(const char *csvFile, const char *jsonFile);           /* 1 = saved */

#endif
