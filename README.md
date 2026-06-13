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
* `make clean` (Cleans all compiled files)

### To run:
* **Milestone 1:** `./sim tests/test1.txt`
* **Milestones 2 :** `./sim tests/test2_no_path.txt`
* **Milestones 3 :** `./sim tests/test3_same_src_dst.txt`
* **Milestones 4 :** `./sim tests/test_multi.txt`
* **Milestones 5 :** `./sim tests/test_m5.txt`
* **Milestones 6 :** `./sim tests/test_m6.txt`

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
