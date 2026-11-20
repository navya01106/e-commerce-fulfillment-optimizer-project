# Complexity report

## 1. Recurrences
| Algorithm | Recurrence | How it is solved | Result |
|---|---|---|---|
| Merge sort | T(n) = 2T(n/2) + n | Recursion tree: log₂n levels, each doing n work. Master Theorem case 2 | Θ(n log n) |
| Quick sort, average | T(n) = 2T(n/2) + n | Master Theorem case 2 | Θ(n log n) |
| Quick sort, worst (sorted input) | T(n) = T(n−1) + n | **Master Theorem does not apply** (the problem shrinks by 1, not by a factor). n + (n−1) + … + 1 = n(n+1)/2 | Θ(n²) |
| Binary search (recursive) | T(n) = T(n/2) + 1 | Iteration: T(n/4) + 2 = … = T(1) + log₂n | Θ(log n) |
| Max subarray (D&C) | T(n) = 2T(n/2) + n | Same as merge sort | Θ(n log n) |
| Fast power | T(n) = T(n/2) + 1 | Same as binary search | Θ(log n) |
| Celebrity (D&C) | T(n) = 2T(n/2) + 1 | Master Theorem case 1 | Θ(n) |
| Strassen | T(n) = 7T(n/2) + n² | Master Theorem case 1 | Θ(n^2.81) |
| 0/1 knapsack (DP) | best[i][w] = max(best[i−1][w], value + best[i−1][w − weight]) | Fill an (n+1) × (W+1) table once, row by row | Θ(n·W) |
| Matrix chain (DP) | cost[i][j] = min over k of cost[i][k] + cost[k+1][j] + d(i−1)·d(k)·d(j) | Fill the table by chain length; each cell tries every split k | Θ(n³) |

## 2. Summary
| Algorithm | Best | Average | Worst | Extra space |
|---|---|---|---|---|
| Merge sort | n log n | n log n | n log n | n |
| Quick sort | n log n | n log n | n² | log n (n in the worst case) |
| Linear search | 1 | n | n | 1 |
| Binary search | 1 | log n | log n | 1 (recursive: log n) |
| Max subarray: D&C / simple | n log n / n² | | | log n / 1 |
| Fast power / simple power | log n / n | | | log n / 1 |
| Celebrity: D&C / simple | n / up to n² | | | log n / 1 |
| Strassen / classical | n^2.81 / n³ | | | n² / 1 |
| Activity selection | n log n | | | n |
| Interval partitioning | n log n + n·k (k = vans) | | | n |
| Fractional knapsack | n log n | | | 1 |
| Huffman (simple scan) | c² for c distinct characters (n log n with a heap) | | | c |
| Dijkstra, Prim (table) | V² | | | V |
| Kruskal | E log E | | | V |
| 0/1 knapsack (DP) | n·W | | | n·W |
| Matrix chain | n³ | | | n² |

## 3. Measured results (menu 14, saved in `results/benchmark.csv`)
**Sorting, 10,000 orders:**

| Input | Merge sort | Quick sort |
|---|---|---|
| Random | 120,482 steps | 176,187 steps |
| **Already sorted** | 69,008 steps, 0.7 ms | **49,995,000 steps, about 250 ms** (= n(n−1)/2 exactly) |

**Searching, 1,000 lookups in 10,000 orders:**

| Method | Steps |
|---|---|
| Linear | 4,991,500 |
| Binary | 12,359 |

**Other divide-and-conquer results:**

| Problem | Simple method | Divide and conquer |
|---|---|---|
| Max subarray, 10,000 days | 50,005,000 steps, 62 ms | 133,616 steps, 0.4 ms |
| Power, x¹⁰⁰⁰⁰⁰⁰ | 1,000,000 multiplications | 27 multiplications |

**0/1 knapsack (parcels cannot be split):**

| Case | Greedy (best ₹/kg first) | Dynamic programming |
|---|---|---|
| Our 35 kg van | ₹25,096 | **₹25,747** (optimal, 540 table cells) |
| 20 random items (benchmark) | 4,094 | **4,147** |

**Matrix chain (report tables 15×8, 8×2, 2×5, 5×1):** left to right needs 465 multiplications; the best order (A1 × (A2 × (A3 × A4))) needs 146.

**Matrices, 256×256:**

| Method | Multiplications |
|---|---|
| Classical | 16.8 million |
| Strassen | 11.2 million (but slower on small sizes) |

**Daily plan:**

| Day | Result |
|---|---|
| Normal | 15 of 15 on time, 6 vans (2 hired), 237 km |
| Shock | 13 served, 2 postponed |
| 10,000 orders | Planned in well under a second |

## 4. Why the greedy choices are correct
- **Activity selection:** taking the slot that ends earliest never reduces how many slots fit afterwards (exchange argument).
- **Interval partitioning:** a new van opens only when every van is busy, so that many slots really overlap. No plan can use fewer vans.
- **Fractional knapsack:** any kg not used on the best ₹/kg item could be swapped for something better.
- **Dijkstra:** with non-negative distances, the closest unfinished place can't get any closer later.
- **Greedy hub choice is NOT always optimal:** on the shock day it postpones 2 orders that only East Hub can reach. This is a known limitation.
- **Greedy is NOT optimal for 0/1 knapsack:** it filled the van with small parcels and lost ₹651. That's why parcel loading uses dynamic programming.

## 5. Recommended choices
| Decision | Use | Why |
|---|---|---|
| Sort orders | Merge sort | Always n log n, stable; quick sort is n² on already-sorted data |
| Find an order | Sorted ID index + binary search | log n per lookup |
| Vans | Interval partitioning | Proven to use the fewest vans |
| Routes | Dijkstra, computed once from every place | Shortest roads, reused for every van |
| Bulk loading | Fractional knapsack | Optimal when goods can be split |
| Parcel loading | 0/1 knapsack DP | Parcels can't be split; greedy lost ₹651 |
| Multiplying report tables | Matrix chain DP | 146 multiplications instead of 465 |
