
# E-Commerce Order Fulfilment and Delivery Optimizer

**DAA Project 02** · Navya Agarwal · SAP ID 590024728 · B.Tech CSE (AI & ML)
Language: C (C17) · Build: Make

An order-to-delivery planning console for an online shop in Ahmedabad. It:
- picks a **fulfilment centre** (hub) for every order
- plans how many **vans** are needed
- plans each van's **route and timing**
- lets a customer **track** an order

It uses **sorting and searching, divide and conquer, greedy**, and two **dynamic programming** problems (**0/1 knapsack** and **matrix chain**). For each topic it compares at least two methods on steps, memory and time.

---

## User guide

### Install and run (Mac or Linux)
```bash
make            # builds the program "optimizer"
./optimizer     # opens the menu (run it from this folder so it finds data/)
make test       # runs the test suite (should end with "0 failed")
make bench      # runs the benchmark and saves results/benchmark.csv
```
On a Mac, install the compiler once with `xcode-select --install`.

### The menu
| Option | What it does | Algorithms |
|---|---|---|
| 1 | Load orders from any CSV file | input validation |
| 2 | Show orders | |
| 3 | Choose a hub for each order | **greedy** (nearest hub with space), merge sort |
| 4 | Plan the vans: orders, roads, load, times | **interval partitioning**, **Dijkstra** |
| 5 | Track an order (packed / out for delivery / delivered) | **binary search** |
| 6 | Export the plan to `results/plan.csv` and `results/plan.json` | |
| 7 | Sorting and searching | merge vs quick sort; linear vs binary search (loop and recursive) |
| 8 | Divide and conquer | max subarray, fast power, celebrity, classical vs Strassen |
| 9 | Greedy | activity selection, interval partitioning, fractional knapsack, Huffman, Dijkstra, Prim vs Kruskal |
| 10 | Dynamic programming | 0/1 knapsack (greedy vs DP), matrix chain |
| 11 | Scenario 1: a normal day | |
| 12 | Scenario 2: shock (hub capacity cut, van broken, road closed) | |
| 13 | Scenario 3: stress test with 10,000 orders | |
| 14 | Benchmark runner: small, medium, large and adversarial data | |

### Input files (`data/`)
- `orders.csv` has the columns `id,customer,area,weight_kg,value_rs,slot_start,slot_end`. Area must be one of: Satellite, Navrangpura, Maninagar, Bopal, Chandkheda, Vastrapur, Paldi, Gota. Bad rows are skipped with a reason. Line 17 is a −5 kg row kept on purpose to show this.
- `order_log.txt` holds the delivery event log, used by Huffman compression.

---

## Project files
| File | Role |
|---|---|
| `daa.h`, `daa.c` | **Algorithm library**: sorting, searching, divide and conquer, greedy, 0/1 knapsack, matrix chain |
| `planner.h`, `planner.c` | **The shop and daily plan**: map, hubs, vans, orders, tracking, export |
| `main.c` | **Console**: menu, module comparisons, 3 scenarios, benchmark |
| `tests/tests.c` | **Test suite**: oracles, edge cases, invalid input, integration, regression |
| `docs/` | Design guide, complexity report, demo script |

## Topics covered
| Area | Implemented | Used for |
|---|---|---|
| Sorting / searching | merge sort, quick sort, linear search, binary search (iterative + recursive) | urgency order, order index, tracking |
| Divide & conquer | maximum subarray, fast power, celebrity, classical vs Strassen | profit streak, growth forecast, key supplier, 2-road route counts |
| Greedy | activity selection, interval partitioning, fractional knapsack, Huffman, Dijkstra, Prim, Kruskal | hubs, vans needed, bulk loading, log compression, routes, road repair network |
| Dynamic programming | 0/1 knapsack, matrix chain | which parcels go into a van, cheapest way to multiply report tables |

## Limitations
- The greedy hub choice is fast but not always the best. On the shock day it postpones 2 orders; a smarter method (network flow) could serve one more.
- The map is small (10 places). Distances are whole km with no live traffic.
- Each van makes one trip, and a delivery holds its van for the whole slot. That's why the 10,000-order stress test needs many vans.

## Git repository
Source: https://github.com/navya01106/E-commerce-fulfillment-optimizer
```bash
git clone https://github.com/navya01106/E-commerce-fulfillment-optimizer.git
cd E-commerce-fulfillment-optimizer
make && ./optimizer
```
