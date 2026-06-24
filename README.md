# Operating Systems Project: Graph Traffic Simulation

## Team Members
* **michaelNz** - Responsibilities: Core Raylib GUI rendering, discrete jump animation math, and build environment (CMake) setup.
* **mohanadYa** - Responsibilities: File I/O parsing, memory management, and Dijkstra's shortest-path algorithm implementation.
* **yazeedTa** - Responsibilities: Animation state-machine timing, Makefile configuration, and GitHub repository management/documentation.

## Project Theme
Our graph represents an Inter-city Delivery Network. Nodes represent regional distribution centers, and edges represent highways. Weights represent the travel time between centers.

## Compilation and Execution
This project includes a uniform `Makefile` to compile specific milestones.

### To compile:
* `make milestone1`
* `make milestone2`
* `make milestone3`
* `make milestone4`
* `make milestone5`
* `make milestone6`
* `make milestone7`
* `make clean` (Cleans all compiled files)

### To run:
* **Milestone 1:** `./sim tests/test1.txt`
* **Milestones 2 :** `./sim tests/test2_no_path.txt`
* **Milestones 3 :** `./sim tests/test3_same_src_dst.txt`
* **Milestones 4 :** `./sim tests/test_multi.txt`
* **Milestones 5 :** `./sim tests/test_m5.txt`
* **Milestones 6 :** `./sim tests/test_m6.txt`
* **Milestone 7 (FCFS):** `./sim -schd fcfs tests/test_m7.txt`
* **Milestone 7 (SJF):** `./sim -schd sjf tests/test_m7.txt`

---

## Milestone Implementations

### Milestone 1: Dijkstra's Algorithm
Reads a directed graph from a text file and builds an adjacency list. Implements Dijkstra's algorithm to calculate the shortest path, handling disconnected graphs and non-negative weights.

### Milestone 2: Static Graph Display
Uses `raylib` to render a graphical representation. Nodes are circles with IDs; edges are directed arrows with weights, using a circular layout algorithm.

### Milestone 3: Animation
An interactive state-machine animation where an entity traverses the shortest path. Movement is discrete: edge weight `W` is divided into `W` jumps (300ms each). Includes a mandatory 1-second pause at nodes.

### Milestone 4: Multiple Travelers (Processes)
Simulates multiple delivery trucks simultaneously. The parent process computes all paths and runs the GUI, while each traveler is a child process (`fork()`). The parent manages child lifecycles, sending `SIGTERM` upon arrival.

### Milestone 5: Inter-Process Communication (IPC)

* **Mechanism:** Pipes (one per traveler for child-to-parent status streaming).
* **Why:** Pipes provide a clean, unidirectional producer-consumer architecture, allowing isolated child processes to safely stream real-time updates to the GUI controller.

### Milestone 6: Node Synchronization

* **Mechanism:** Binary Semaphores (`sem_t`) stored in shared memory (`mmap` + `pshared=1`).
* **Logic:** Before entering a node, a traveler executes `sem_wait()`. It occupies the node for 1 second, then executes `sem_post()` to free it. This prevents collision and ensures safe intersection management.

### Milestone 7: Scheduling Algorithms

Replaces the unordered semaphore wait queue from milestone 6 with two selectable scheduling algorithms that decide, in a controlled order, which waiting traveler enters a free node next.

* **FCFS (First-Come-First-Served):** the traveler that requested the node earliest (lowest arrival sequence number) is admitted first. This is the simplest, fairest-by-arrival-time policy.
* **SJF (Shortest-Job-First):** the traveler whose *next edge* (the edge it will travel immediately after leaving this node) has the smallest weight is admitted first. Ties are broken by arrival order. This minimizes the time other travelers stay blocked behind a traveler with a long upcoming trip.

**Mechanism:** a shared-memory `SchedulerState` (one entry queue per node) holds, for every waiting traveler: its PID, arrival sequence number, a private `sem_t` ("turn" semaphore) and its "burst" (the weight of its next edge). A traveler calls `scheduler_request_enter()`, which registers it in the node's queue and blocks on its own `turn_sem`. Once per GUI frame, the parent calls `scheduler_tick()`, which — for every node that is currently free — picks the winning waiter according to the active algorithm and posts that waiter's semaphore, admitting it. When the traveler leaves the node it calls `scheduler_leave()`, freeing the node for the next tick.

**Usage:**
```bash
./sim -schd fcfs tests/test_m7.txt
./sim -schd sjf  tests/test_m7.txt
```

The active algorithm is shown clearly in the top-right corner of the GUI window ("Scheduler: FCFS" / "Scheduler: SJF").

**Comparison (FCFS vs SJF) on `tests/test_m7.txt`:**
With three travelers all heading toward node 5 through different entry points, FCFS admits travelers strictly in the order they first request the shared node, regardless of how long their remaining trip is — a traveler with a long upcoming edge can make others wait behind it only as long as it itself waited, which is fair but not optimal. SJF instead prioritizes the traveler with the *shortest* upcoming edge, so travelers who have less travel time left clear the bottleneck node faster, slightly reducing the **average wait time** across all travelers at the shared node, at the cost of occasionally delaying a traveler with a longer remaining route even though it arrived earlier. The total wall-clock time to finish all travelers was modestly lower under SJF for this graph because the shorter "next hops" got priority and did not block behind a traveler that was about to take the weight-10 edge.