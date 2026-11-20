# Demonstration script (8 minutes + 4 minutes viva)

Before the jury arrives, run `make` and `make test`, then `./optimizer`.

| Time | Menu | Show | Say |
|---|---|---|---|
| 0:00 | 11 | **Normal day.** Hubs, each van's route, load and times | "Every order gets a hub, a van, a route and an arrival time. 15 of 15 on time, 6 vans, 237 km." |
| 1:30 | 12 | **Shock.** East Hub capacity 8 → 2, Van 2 broken, a road closed | "The plan adapts: 13 orders served, 2 postponed because no hub has space, and Dijkstra reroutes around the closed road." |
| 2:45 | 5 | Track order **110** at hour **12**, then hour **18** | "Binary search finds the order in a few steps; the status comes from the plan." |
| 3:30 | 10 | **Dynamic programming:** 0/1 knapsack and matrix chain | "Greedy loads ₹25,096; DP loads ₹25,747. Greedy filled the van with small parcels, so the big valuable one didn't fit. DP checks take-or-leave for every parcel." |
| 4:45 | 8 | Divide and conquer: profit streak, fast power | "54 steps instead of 105 for the streak, and 9 multiplications instead of 30 for the power." |
| 5:45 | 9 | Greedy: fractional knapsack, Dijkstra, Prim = Kruskal | "Greedy picks the best choice at each step. Prim and Kruskal both find the 54 km network." |
| 6:45 | 14 | Benchmark, the "adversarial" sorting row | "On already-sorted orders, quick sort does 50 million steps against merge sort's 69 thousand. That's why I chose merge sort." |

## The three required scenarios
1. Normal operation: menu 11
2. Capacity or constraint shock: menu 12
3. Large-data stress case: menu 13
