#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "raylib.h"
#include <unistd.h>
#include "gui_multi.h"
#include "graph.h"
#include "traveler.h"

#define WINDOW_W    900
#define WINDOW_H    700
#define NODE_RADIUS 24
#define MAX_VERTS   15
#define FPS         60
#define STEP_MS     300.0f
#define WAIT_MS     1000.0f

/* 8 distinct traveler colors */
static Color TRAV_COLORS[] = {
    {255, 200,   0, 255},   /* yellow  */
    {60,  200,  80, 255},   /* green   */
    {80,  160, 255, 255},   /* blue    */
    {255,  80,  80, 255},   /* red     */
    {200,  80, 255, 255},   /* purple  */
    {255, 160,  40, 255},   /* orange  */
    {0,   210, 210, 255},   /* cyan    */
    {255, 100, 180, 255},   /* pink    */
};
#define N_COLORS 8

#define COL_BG        RAYWHITE
#define COL_NODE      (Color){70,  130, 180, 255}
#define COL_EDGE      (Color){160, 160, 160, 255}
#define COL_WEIGHT    (Color){60,   60,  60, 255}
#define COL_BTN       (Color){60,   60,  60, 255}
#define COL_BTN_HOV   (Color){100, 100, 100, 255}
#define COL_BTN_GRN   (Color){40,  150,  60, 255}
#define COL_BTN_GRN_H (Color){60,  190,  80, 255}
#define COL_BTN_RED   (Color){180,  40,  40, 255}
#define COL_BTN_RED_H (Color){220,  60,  60, 255}

static void compute_positions(int n, Vector2 *pos)
{
    float cx=WINDOW_W/2.0f, cy=WINDOW_H/2.0f;

    /* For small graphs use a hand-tuned layout that avoids edge crossings.
     * Nodes are placed on a large circle with extra spacing. */
    if(n==1){pos[0]=(Vector2){cx,cy};return;}

    /* All nodes on one circle, enough radius so arrows don't overlap */
    float r = (n <= 6) ? 260.0f : (n <= 10) ? 240.0f : 200.0f;
    for(int i=0;i<n;i++){
        float a = (2.0f*PI*i/n) - PI/2.0f;
        pos[i] = (Vector2){cx + r*cosf(a), cy + r*sinf(a)};
    }
}

static int get_edge_weight(const Graph *g, int u, int v)
{
    for(Edge *e=g->adj[u];e;e=e->next) if(e->dst==v) return e->weight;
    return 1;
}

static void draw_edge(Vector2 from, Vector2 to, int weight)
{
    float dx=to.x-from.x,dy=to.y-from.y,len=sqrtf(dx*dx+dy*dy);
    if(len<1.0f)return;
    float ux=dx/len,uy=dy/len;
    Vector2 s={from.x+ux*NODE_RADIUS,from.y+uy*NODE_RADIUS};
    Vector2 e={to.x-ux*NODE_RADIUS,to.y-uy*NODE_RADIUS};
    DrawLineEx(s,e,1.5f,COL_EDGE);
    float angle=atan2f(uy,ux),al=14.0f,aa=0.42f;
    Vector2 tip=e;
    Vector2 left={e.x-al*cosf(angle-aa),e.y-al*sinf(angle-aa)};
    Vector2 right={e.x-al*cosf(angle+aa),e.y-al*sinf(angle+aa)};
    DrawTriangle(tip,right,left,COL_EDGE);
    float mx=(s.x+e.x)/2.0f-uy*14.0f,my=(s.y+e.y)/2.0f+ux*14.0f;
    char buf[16];snprintf(buf,sizeof(buf),"%d",weight);
    int fs=20,tw=MeasureText(buf,fs);
    DrawText(buf,(int)(mx-tw/2),(int)(my-fs/2),fs,COL_WEIGHT);
}

static void draw_node(Vector2 p, int id)
{
    DrawCircleV(p,NODE_RADIUS,COL_NODE);
    DrawCircleLines((int)p.x,(int)p.y,NODE_RADIUS,BLACK);
    char buf[8];snprintf(buf,sizeof(buf),"%d",id);
    int fs=24,tw=MeasureText(buf,fs);
    DrawText(buf,(int)(p.x-tw/2),(int)(p.y-fs/2),fs,WHITE);
}

static int draw_btn(Rectangle r, const char *label, Color base, Color hov)
{
    bool over=CheckCollisionPointRec(GetMousePosition(),r);
    DrawRectangleRec(r,over?hov:base);
    DrawRectangleLinesEx(r,1.5f,DARKGRAY);
    int fs=22,tw=MeasureText(label,fs);
    DrawText(label,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-fs/2),fs,WHITE);
    return over&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

/* initialize / reset one traveler's animation state */
static void trav_start(Traveler *t, Vector2 *pos, const Graph *g)
{
    t->path_idx   = 0;
    t->jump       = 0;
    t->done       = 0;
    t->wait_ms    = 0;
    t->timer_ms   = STEP_MS;
    /* guard: src must be valid */
    if(t->src < 0 || t->src >= MAX_VERTS) { t->done = 1; return; }
    t->x = pos[t->src].x;
    t->y = pos[t->src].y;
    if(!t->path.found || t->path.path_len < 2){
        t->done = 1;
        return;
    }
    t->total_jumps = get_edge_weight(g, t->path.path[0], t->path.path[1]);
}

static void trav_update(Traveler *t, Vector2 *pos, const Graph *g, float dt)
{
    if(t->done || !t->path.found) return;
    if(t->path_idx + 1 >= t->path.path_len) { t->done = 1; return; }

    /* waiting at intermediate node */
    if(t->wait_ms > 0){
        t->wait_ms -= dt;
        return;
    }

    int u = t->path.path[t->path_idx];
    int v = t->path.path[t->path_idx+1];
    if(t->total_jumps <= 0) { t->done = 1; return; }

    /* snap to current jump position */
    float frac = (float)t->jump / (float)t->total_jumps;
    t->x = pos[u].x + frac*(pos[v].x-pos[u].x);
    t->y = pos[u].y + frac*(pos[v].y-pos[u].y);

    t->timer_ms -= dt;
    if(t->timer_ms <= 0.0f){
        t->jump++;
        t->timer_ms = STEP_MS;
        if(t->jump >= t->total_jumps){
            /* arrived at next node */
            t->x = pos[v].x; t->y = pos[v].y;
            if(t->path_idx+1 >= t->path.path_len-1){
                t->done = 1;
            } else {
                t->wait_ms  = WAIT_MS;
                t->path_idx++;
                t->jump     = 0;
                if(t->path_idx+1 < t->path.path_len)
                    t->total_jumps = get_edge_weight(g,
                                        t->path.path[t->path_idx],
                                        t->path.path[t->path_idx+1]);
            }
        }
    }
}

void gui_multi_run(const Graph *g, Traveler *travelers, int T)
{
    Vector2 pos[MAX_VERTS];
    compute_positions(g->num_vertices, pos);

    int playing = 0;
    int started = 0;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(WINDOW_W, WINDOW_H, "Traffic Simulation – Multi");
    SetTargetFPS(FPS);

    Rectangle btn_play    = {WINDOW_W-280, WINDOW_H-54, 120, 40};
    Rectangle btn_restart = {WINDOW_W-150, WINDOW_H-54, 130, 40};

    while(!WindowShouldClose()){
        float dt = GetFrameTime()*1000.0f;

        /* check all done */
        int all_done = started;
        for(int i=0;i<T;i++) if(!travelers[i].done) all_done=0;

        if(playing){
            for(int i=0;i<T;i++)
                trav_update(&travelers[i], pos, g, dt);
            /* send SIGTERM to finished travelers */
            for(int i=0;i<T;i++)
                if(travelers[i].done && travelers[i].pid>0){
                    kill(travelers[i].pid, SIGTERM);
                    travelers[i].pid = -1;
                }
        }

        BeginDrawing();
        ClearBackground(COL_BG);

        /* edges */
        for(int u=0;u<g->num_vertices;u++)
            for(Edge *e=g->adj[u];e;e=e->next)
                draw_edge(pos[u],pos[e->dst],e->weight);

        /* nodes */
        for(int v=0;v<g->num_vertices;v++)
            draw_node(pos[v],v);

        /* travelers */
        for(int i=0;i<T;i++){
            if(!started) break;
            Color c = TRAV_COLORS[i % N_COLORS];
            DrawCircle((int)travelers[i].x,(int)travelers[i].y,14,c);
            /* label with traveler index */
            char buf[16]; snprintf(buf,sizeof(buf),"%d",i);
            int tw=MeasureText(buf,14);
            DrawText(buf,(int)travelers[i].x-tw/2,(int)travelers[i].y-7,14,BLACK);
        }

        /* legend */
        int lx=16,ly=16;
        DrawText("Travelers:",lx,ly,20,DARKGRAY); ly+=22;
        for(int i=0;i<T;i++){
            Color c=TRAV_COLORS[i%N_COLORS];
            DrawCircle(lx+10,ly+10,10,c);
            char buf[64];
            if(!travelers[i].path.found)
                snprintf(buf,sizeof(buf),"#%d: %d->%d (no path)",i,travelers[i].src,travelers[i].dst);
            else
                snprintf(buf,sizeof(buf),"#%d: %d->%d (w=%d)",i,travelers[i].src,travelers[i].dst,travelers[i].path.total_weight);
            DrawText(buf,lx+26,ly+3,18,DARKGRAY);
            ly+=22;
        }

        /* all done message */
        if(all_done && started){
            const char *msg="All travelers arrived!";
            int fs=28,tw=MeasureText(msg,fs);
            DrawRectangle(WINDOW_W/2-tw/2-16,WINDOW_H/2-28,tw+32,56,(Color){0,0,0,180});
            DrawText(msg,WINDOW_W/2-tw/2,WINDOW_H/2-fs/2,fs,YELLOW);
        }

        /* buttons */
        if(draw_btn(btn_play,
                    playing?"STOP":"PLAY",
                    playing?COL_BTN_RED:COL_BTN_GRN,
                    playing?COL_BTN_RED_H:COL_BTN_GRN_H)){
            if(!started){
                started=1;
                for(int i=0;i<T;i++) trav_start(&travelers[i],pos,g);
            }
            playing=!playing;
        }
        if(draw_btn(btn_restart,"RESTART",COL_BTN,COL_BTN_HOV)){
            started=1; playing=1;
            for(int i=0;i<T;i++) trav_start(&travelers[i],pos,g);
        }

        DrawText("ESC to close",WINDOW_W-160,WINDOW_H-28,16,LIGHTGRAY);
        EndDrawing();
    }
    CloseWindow();
}