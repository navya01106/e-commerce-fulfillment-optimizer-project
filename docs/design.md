# Design guide

## 1. Layers
```
main.c          console: menu, module demos, scenarios, benchmark
   |
planner.c/.h    the shop and its daily plan: map, hubs, vans, orders, tracking, CSV/JSON export
   |
daa.c/.h        algorithm library: sorting, searching, divide and conquer, greedy, 0/1 knapsack, matrix chain
```
`daa.c` knows nothing about the shop. It works on plain arrays, so each algorithm can be tested on its own. `planner.c` decides which algorithm makes each business decision. `main.c` only calls the other two and prints the results.

## 2. Interface rules (daa.h)
- Inputs are arrays plus their size. The answer is returned, or written into an output array.
- Graphs are a table `w[a][b]`, where 0 means no road. Kruskal takes a list of roads instead, because it sorts them.
- Every algorithm adds to `steps` (basic operations). It also records extra memory with `memUse`/`memFree`: temporary arrays, and 48 bytes per waiting recursive call. `memPeak` is the most memory used at one time.
- No library sort or search (`qsort`, `bsearch`) is used.

## 3. Data records
| Record | Fields |
|---|---|
| `Order` | id, customer, area, weight, value, slot start/end |
| `Hub` | name, area, daily capacity |
| `Van` | id, home hub, capacity kg, working? |
| `Edge` | from, to, km |
| `VanPlan` | hub, van, capacity, load, leave time, km, late count |

The map is a weighted graph of 10 places (West Hub = 0, East Hub = 9) and 16 roads.

## 4. The daily plan (planDay)
1. **Hub (greedy):** sort orders by urgency (merge sort). Send each to the nearest hub that still has space and covers that area (within 20 km). If no hub can take it, it's postponed.
2. **Vans (greedy):** interval partitioning splits each hub's orders into the fewest groups with no clashing slots. One group is one van.
3. **Which van:** the heaviest group gets the biggest working van. Extra groups get hired 30 kg vans.
4. **Route and times:** each van visits its stops in slot order along the shortest roads (Dijkstra, run once from every place). The van drives at 30 km/h, takes 10 minutes per handover, and waits if early.

## 5. Validation and testing
- **Input checks:** wrong format, weight ≤ 0, negative value, a bad slot, an unknown area, a duplicate ID.
- **Tests:** about 1,950 checks (the exact number can differ slightly between computers, because some tests use random inputs).
  - **Oracles:** slow brute-force methods on random small inputs (activity selection, interval partitioning, Dijkstra, 0/1 knapsack).
  - **Textbook answers:** max subarray 43, Huffman 33 bits, matrix chain 15,125.
  - **Cross-checks:** Prim = Kruskal.
  - **Integration:** the full daily plan.
  - **Regression values:** 6 vans, 237 km, 13 served on the shock day, 54 km network, 35 kg van ₹25,747 (DP) vs ₹25,096 (greedy).
