# Operating Systems Project: Graph Traffic Simulation

## Team Members
* **michaelNz** - [ID] - Responsibilities: Core Raylib GUI rendering, discrete jump animation math, and build environment (CMake) setup.
* **mohanadYa** - [ID] - Responsibilities: File I/O parsing (`loadGraphFromFile`), memory management, and Dijkstra's shortest-path algorithm implementation.
* **yazeedTa** - [ID] - Responsibilities: Animation state-machine timing (1-second node delays, 300ms edge jumps), `Makefile` configuration, and GitHub repository management/documentation.

## Project Theme / Background Story
Our graph represents an Inter-city Delivery Network. The nodes represent regional distribution centers, and the edges represent the highways connecting them. The weights on the edges represent the travel time required for a delivery truck to travel between the centers.

## Compilation and Execution
This project includes a uniform `Makefile` to compile specific milestones.

### To compile:
* `make milestone1`
* `make milestone2`
* `make milestone3`
* `make clean` (Cleans all compiled files)

### To run:
* **Milestone 1:** `./dijkstra <file_name>`
* **Milestone 2 & 3:** `./sim <file_name>`

## Milestone Implementations
### Milestone 1: Dijkstra's Algorithm
* **Description:** Reads a directed graph from a text file and builds an adjacency matrix. It implements Dijkstra's algorithm to calculate the shortest path from a source node to a destination node, printing the exact path and total weight.

### Milestone 2: Static Graph Display
* **Description:** Uses the `raylib` library to render a graphical representation of the parsed text file. Nodes are displayed as colored circles with IDs, and edges are drawn as directed arrows with their respective weights, ensuring clear directionality.

### Milestone 3: Animation on the Graph
* **Description:** Introduces an interactive state-machine animation loop. A distinct entity travels along the pre-calculated shortest path. It utilizes a discrete jumping mechanism, where the entity divides its movement across an edge into $W$ (weight) equal jumps, with each jump taking exactly 300ms. It also implements a mandatory 1-second pause at all intermediate nodes and includes interactive Play/Pause and Restart controls.