#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"

#define MAX_NODES 15
#define INF INT_MAX

// --- GLOBAL VARIABLES ---
int graph[MAX_NODES][MAX_NODES];
int num_nodes = 0;
int num_edges = 0;

int global_path[MAX_NODES];
int global_path_length = 0;

void initGraph() {
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            graph[i][j] = INF;
        }
    }
}

void loadGraphFromFile(const char* filename, int* start_node, int* end_node) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s.\n", filename);
        exit(1);
    }

    fscanf(file, "%d %d", &num_nodes, &num_edges);

    for (int i = 0; i < num_edges; i++) {
        int u, v, weight;
        fscanf(file, "%d %d %d", &u, &v, &weight);
        if (weight < 0) {
            printf("Error: Negative weights are not allowed.\n");
            exit(1);
        }
        graph[u][v] = weight;
    }

    fscanf(file, "%d %d", start_node, end_node);
    fclose(file);
}

void dijkstra(int start, int end) {
    int dist[MAX_NODES];
    int prev[MAX_NODES];
    bool visited[MAX_NODES];

    for (int i = 0; i < num_nodes; i++) {
        dist[i] = INF;
        prev[i] = -1;
        visited[i] = false;
    }
    dist[start] = 0;

    if (start == end) {
        printf("0\n0\n");
        return;
    }

    for (int count = 0; count < num_nodes - 1; count++) {
        int min_dist = INF, u = -1;
        for (int v = 0; v < num_nodes; v++) {
            if (!visited[v] && dist[v] <= min_dist) {
                min_dist = dist[v];
                u = v;
            }
        }

        if (u == -1 || dist[u] == INF) break;
        visited[u] = true;

        for (int v = 0; v < num_nodes; v++) {
            if (!visited[v] && graph[u][v] != INF && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                prev[v] = u;
            }
        }
    }

    if (dist[end] == INF) {
        printf("No path found\n");
    } else {
        int path[MAX_NODES];
        int path_length = 0;
        int curr = end;

        while (curr != -1) {
            path[path_length++] = curr;
            curr = prev[curr];
        }

        global_path_length = path_length;
        for (int i = 0; i < path_length; i++) {
            global_path[i] = path[path_length - 1 - i];
        }

        for (int i = path_length - 1; i >= 0; i--) {
            printf("%d", path[i]);
            if (i > 0) printf(" -> ");
        }
        printf("\n%d\n", dist[end]);
    }
}

// Milestone 3: Animated GUI
void drawGraphGUI() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "OS Project - Milestone 3");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES] = {
        {150, 300}, {350, 150}, {350, 450},
        {550, 150}, {700, 300}, {550, 450}
    };

    // --- ANIMATION VARIABLES ---
    bool isAnimating = false;
    int pathIndex = 0;
    float timer = 0.0f;
    int currentJump = 0; // Tracks which jump we are currently on for the edge

    // States: 0 = Waiting at Node, 1 = Moving on Edge, 2 = Finished
    int animState = (global_path_length > 1) ? 0 : 2;

    Vector2 entityPos = positions[global_path[0]];

    // Buttons definition
    Rectangle playBtn = { 10, 40, 100, 40 };
    Rectangle restartBtn = { 120, 40, 120, 40 };

    while (!WindowShouldClose()) {
        // --- 1. UPDATE LOGIC ---
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();

            // Play/Stop Button Click
            if (CheckCollisionPointRec(mousePos, playBtn)) {
                if (animState != 2) isAnimating = !isAnimating;
            }

            // Restart Button Click
            if (CheckCollisionPointRec(mousePos, restartBtn)) {
                pathIndex = 0;
                animState = (global_path_length > 1) ? 0 : 2;
                timer = 0.0f;
                currentJump = 0;
                isAnimating = false;
                if (global_path_length > 0) entityPos = positions[global_path[0]];
            }
        }

        if (isAnimating) {
            timer += GetFrameTime();

            if (animState == 0) { // WAITING AT NODE
                // Wait exactly 1 second at intermediate nodes[cite: 3]
                if (pathIndex == 0 || timer >= 1.0f) {
                    timer = 0.0f;
                    animState = 1;
                    currentJump = 0;
                }
            }
            else if (animState == 1) { // MOVING ON EDGE IN DISCRETE JUMPS[cite: 3]
                int u = global_path[pathIndex];
                int v = global_path[pathIndex + 1];
                int weight = graph[u][v];

                // Jump exactly every 300ms[cite: 3]
                if (timer >= 0.3f) {
                    timer = 0.0f;
                    currentJump++;

                    // Calculate discrete position fractional jump (e.g., 1/4, 2/4, 3/4)[cite: 3]
                    float progress = (float)currentJump / (float)weight;
                    entityPos.x = positions[u].x + (positions[v].x - positions[u].x) * progress;
                    entityPos.y = positions[u].y + (positions[v].y - positions[u].y) * progress;

                    // If we completed all W jumps, we reached the destination node[cite: 3]
                    if (currentJump >= weight) {
                        pathIndex++;
                        if (pathIndex >= global_path_length - 1) {
                            animState = 2; // Reached the final destination
                            isAnimating = false;
                        } else {
                            animState = 0; // Wait at this intermediate node
                        }
                    }
                }
            }
        }

        // --- 2. DRAWING LOGIC ---
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Milestone 3: Animation (Discrete Jumps)", 10, 10, 20, DARKGRAY);

        // Draw Play/Stop Button
        DrawRectangleRec(playBtn, isAnimating ? LIGHTGRAY : GRAY);
        DrawRectangleLinesEx(playBtn, 2, DARKGRAY);
        DrawText(isAnimating ? "STOP" : "PLAY", playBtn.x + 25, playBtn.y + 10, 20, isAnimating ? RED : GREEN);

        // Draw Restart Button
        DrawRectangleRec(restartBtn, GRAY);
        DrawRectangleLinesEx(restartBtn, 2, DARKGRAY);
        DrawText("RESTART", restartBtn.x + 15, restartBtn.y + 10, 20, BLUE);

        // Draw Edges
        for (int i = 0; i < num_nodes; i++) {
            for (int j = 0; j < num_nodes; j++) {
                if (graph[i][j] != INF && i != j) {
                    Vector2 start = positions[i];
                    Vector2 end = positions[j];
                    float dx = end.x - start.x;
                    float dy = end.y - start.y;
                    float angle = atan2(dy, dx);

                    float nodeRadius = 25.0f;
                    Vector2 arrowEnd;
                    arrowEnd.x = end.x - cos(angle) * nodeRadius;
                    arrowEnd.y = end.y - sin(angle) * nodeRadius;
                    DrawLineEx(start, arrowEnd, 2.0f, DARKGRAY);

                    float arrowLen = 15.0f, arrowAng = PI / 6.0f;
                    Vector2 p1 = { arrowEnd.x - cos(angle - arrowAng) * arrowLen, arrowEnd.y - sin(angle - arrowAng) * arrowLen };
                    Vector2 p2 = { arrowEnd.x - cos(angle + arrowAng) * arrowLen, arrowEnd.y - sin(angle + arrowAng) * arrowLen };
                    DrawLineEx(arrowEnd, p1, 3.0f, DARKGRAY);
                    DrawLineEx(arrowEnd, p2, 3.0f, DARKGRAY);

                    DrawText(TextFormat("%d", graph[i][j]), (start.x + end.x) / 2, ((start.y + end.y) / 2) - 20, 20, RED);
                }
            }
        }

        // Draw Nodes
        for (int i = 0; i < num_nodes; i++) {
            DrawCircleV(positions[i], 25, DARKBLUE);
            DrawText(TextFormat("%d", i), positions[i].x - 5, positions[i].y - 10, 20, WHITE);
        }

        // Draw the Animated Entity (Orange Circle)
        if (global_path_length > 0) {
            DrawCircleV(entityPos, 15, ORANGE);
        }

        // Draw Finished Message[cite: 3]
        if (animState == 2) {
            DrawText("DESTINATION REACHED!", 250, 50, 30, DARKGREEN);
        }

        EndDrawing();
    }
    CloseWindow();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    initGraph();
    int start_node, end_node;

    loadGraphFromFile(argv[1], &start_node, &end_node);
    dijkstra(start_node, end_node);
    drawGraphGUI();

    return 0;
}