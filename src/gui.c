#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "gui.h"
#include "graph.h"
#include "dijkstra.h"

#define WINDOW_W      900
#define WINDOW_H      700
#define NODE_RADIUS   24
#define MAX_VERTICES  15
#define FPS           60
#define STEP_MS       300.0f
#define WAIT_MS       1000.0f

#define COL_BG         RAYWHITE
#define COL_NODE       (Color){70,  130, 180, 255}
#define COL_NODE_PATH  (Color){220,  60,  60, 255}
#define COL_NODE_SRC   (Color){50,  180,  80, 255}
#define COL_NODE_DST   (Color){230, 140,  20, 255}
#define COL_EDGE       (Color){140, 140, 140, 255}
#define COL_EDGE_PATH  (Color){220,  60,  60, 255}
#define COL_AGENT      (Color){255, 200,   0, 255}
#define COL_TEXT       WHITE
#define COL_WEIGHT     (Color){50,  50,  50, 255}
#define COL_WEIGHT_P   (Color){180,  30,  30, 255}
#define COL_BTN        (Color){60,  60,  60, 255}
#define COL_BTN_HOV    (Color){100, 100, 100, 255}
#define COL_BTN_RED    (Color){180,  40,  40, 255}
#define COL_BTN_RED_H  (Color){220,  60,  60, 255}
#define COL_BTN_GRN    (Color){40,  150,  60, 255}
#define COL_BTN_GRN_H  (Color){60,  190,  80, 255}

typedef enum { STATE_IDLE, STATE_MOVING, STATE_WAITING, STATE_DONE } AgentState;

typedef struct {
    AgentState state;
    int        path_idx;    /* current edge index in path */
    int        jump;        /* current jump (0..total_jumps-1) */
    int        total_jumps; /* weight of current edge */
    float      timer_ms;
    Vector2    pos;
    int        playing;
} Agent;

/* ── Layout ─────────────────────────────────────────────────────────────── */
static void compute_positions(int n, Vector2 *pos)
{
    float cx = WINDOW_W/2.0f, cy = WINDOW_H/2.0f;
    if (n == 1) { pos[0] = (Vector2){cx,cy}; return; }
    float r = (n <= 6) ? 260.0f : (n <= 10) ? 240.0f : 200.0f;
    for (int i=0;i<n;i++){
        float a=(2.0f*PI*i/n)-PI/2.0f;
        pos[i]=(Vector2){cx+r*cosf(a), cy+r*sinf(a)};
    }
}

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static int edge_on_path(const DijkstraResult *r, int u, int v)
{
    if (!r||!r->found) return 0;
    for(int i=0;i+1<r->path_len;i++)
        if(r->path[i]==u && r->path[i+1]==v) return 1;
    return 0;
}
static int vertex_on_path(const DijkstraResult *r, int v)
{
    if (!r||!r->found) return 0;
    for(int i=0;i<r->path_len;i++) if(r->path[i]==v) return 1;
    return 0;
}
static int get_edge_weight(const Graph *g, int u, int v)
{
    for(Edge *e=g->adj[u];e;e=e->next) if(e->dst==v) return e->weight;
    return 1;
}

/* ── Draw helpers ────────────────────────────────────────────────────────── */
static void draw_edge(Vector2 from, Vector2 to, int weight,
                      Color ec, Color wc, int is_path)
{
    float dx=to.x-from.x, dy=to.y-from.y;
    float len=sqrtf(dx*dx+dy*dy);
    if(len<1.0f) return;
    float ux=dx/len, uy=dy/len;
    Vector2 s={from.x+ux*NODE_RADIUS, from.y+uy*NODE_RADIUS};
    Vector2 e={to.x  -ux*NODE_RADIUS, to.y  -uy*NODE_RADIUS};
    DrawLineEx(s, e, is_path?3.0f:1.5f, ec);

    /* arrowhead — counter-clockwise winding for raylib */
    float angle=atan2f(uy,ux), al=14.0f, aa=0.42f;
    Vector2 tip  = e;
    Vector2 left = {e.x-al*cosf(angle-aa), e.y-al*sinf(angle-aa)};
    Vector2 right= {e.x-al*cosf(angle+aa), e.y-al*sinf(angle+aa)};
    DrawTriangle(tip, right, left, ec);

    /* weight label */
    float mx=(s.x+e.x)/2.0f-uy*14.0f, my=(s.y+e.y)/2.0f+ux*14.0f;
    char buf[16]; snprintf(buf,sizeof(buf),"%d",weight);
    int fs=20, tw=MeasureText(buf,fs);
    DrawText(buf,(int)(mx-tw/2),(int)(my-fs/2),fs,wc);
}

static void draw_node(Vector2 p, int id, Color fill)
{
    DrawCircleV(p,NODE_RADIUS,fill);
    DrawCircleLines((int)p.x,(int)p.y,NODE_RADIUS,BLACK);
    char buf[8]; snprintf(buf,sizeof(buf),"%d",id);
    int fs=24,tw=MeasureText(buf,fs);
    DrawText(buf,(int)(p.x-tw/2),(int)(p.y-fs/2),fs,COL_TEXT);
}

static int draw_btn(Rectangle r, const char *label, Color base, Color hov)
{
    bool over=CheckCollisionPointRec(GetMousePosition(),r);
    DrawRectangleRec(r, over?hov:base);
    DrawRectangleLinesEx(r,1.5f,DARKGRAY);
    int fs=22,tw=MeasureText(label,fs);
    DrawText(label,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-fs/2),fs,WHITE);
    return over&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void draw_legend(const DijkstraResult *r, int src, int dst)
{
    int x=16,y=16,sz=18,gap=28;
    DrawCircle(x+10,y+10,10,COL_NODE_SRC);
    DrawText(TextFormat("Source (%d)",src),x+26,y+3,sz,DARKGRAY); y+=gap;
    DrawCircle(x+10,y+10,10,COL_NODE_DST);
    DrawText(TextFormat("Dest (%d)",dst),  x+26,y+3,sz,DARKGRAY); y+=gap;
    DrawCircle(x+10,y+10,10,COL_AGENT);
    DrawText("Agent",                       x+26,y+3,sz,DARKGRAY); y+=gap;
    if(r&&r->found)
        DrawText(TextFormat("Total weight: %d",r->total_weight),x,y+3,sz,COL_WEIGHT_P);
    else
        DrawText("No path found",x,y+3,sz,RED);
}

/* ── Agent ───────────────────────────────────────────────────────────────── */
static void agent_reset(Agent *a, const DijkstraResult *r, Vector2 *pos)
{
    a->state=STATE_IDLE; a->playing=0;
    a->path_idx=0; a->jump=0; a->total_jumps=0; a->timer_ms=0;
    a->pos=(r&&r->found)?pos[r->path[0]]:(Vector2){0,0};
}

static void agent_start(Agent *a, const DijkstraResult *r,
                         const Graph *g, Vector2 *pos)
{
    agent_reset(a,r,pos);
    if(!r||!r->found||r->path_len<2) return;
    a->state       = STATE_MOVING;
    a->playing     = 1;
    a->jump        = 0;
    a->total_jumps = get_edge_weight(g,r->path[0],r->path[1]);
    a->timer_ms    = STEP_MS;
}

static void agent_update(Agent *a, const DijkstraResult *r,
                          const Graph *g, Vector2 *pos, float dt)
{
    if(!a->playing||a->state==STATE_DONE||a->state==STATE_IDLE) return;

    if(a->state==STATE_WAITING){
        a->timer_ms-=dt;
        if(a->timer_ms<=0.0f){
            a->path_idx++;
            if(a->path_idx+1>=r->path_len){ a->state=STATE_DONE; return; }
            a->jump        = 0;
            a->total_jumps = get_edge_weight(g,r->path[a->path_idx],
                                               r->path[a->path_idx+1]);
            a->timer_ms    = STEP_MS;
            a->state       = STATE_MOVING;
        }
        return;
    }

    if(a->state==STATE_MOVING){
        int u=r->path[a->path_idx];
        int v=r->path[a->path_idx+1];

        float t = (float)a->jump / (float)a->total_jumps;
        a->pos.x = pos[u].x + t*(pos[v].x-pos[u].x);
        a->pos.y = pos[u].y + t*(pos[v].y-pos[u].y);

        a->timer_ms -= dt;
        if(a->timer_ms<=0.0f){
            a->jump++;
            a->timer_ms = STEP_MS;
            if(a->jump>=a->total_jumps){
                a->pos = pos[v];
                if(a->path_idx+1==r->path_len-1){
                    a->state=STATE_DONE;
                } else {
                    a->state    = STATE_WAITING;
                    a->timer_ms = WAIT_MS;
                }
            }
        }
    }
}

/* ── Main GUI loop ───────────────────────────────────────────────────────── */
void gui_run(const Graph *g, const DijkstraResult *result)
{
    int src=(result&&result->found&&result->path_len>0)?result->path[0]:-1;
    int dst=(result&&result->found&&result->path_len>0)?result->path[result->path_len-1]:-1;

    Vector2 pos[MAX_VERTICES];
    compute_positions(g->num_vertices,pos);

    Agent agent;
    agent_reset(&agent,result,pos);

    SetTraceLogLevel(LOG_NONE);
    InitWindow(WINDOW_W,WINDOW_H,"Traffic Simulation");
    SetTargetFPS(FPS);

    Rectangle btn_play    = {WINDOW_W-280, WINDOW_H-54, 120, 40};
    Rectangle btn_restart = {WINDOW_W-150, WINDOW_H-54, 130, 40};

    while(!WindowShouldClose()){
        float dt=GetFrameTime()*1000.0f;

        agent_update(&agent,result,g,pos,dt);

        BeginDrawing();
        ClearBackground(COL_BG);

        /* edges */
        for(int u=0;u<g->num_vertices;u++)
            for(Edge *e=g->adj[u];e;e=e->next){
                int on=edge_on_path(result,u,e->dst);
                draw_edge(pos[u],pos[e->dst],e->weight,
                          on?COL_EDGE_PATH:COL_EDGE,
                          on?COL_WEIGHT_P :COL_WEIGHT, on);
            }

        /* nodes */
        for(int v=0;v<g->num_vertices;v++){
            Color fill=COL_NODE;
            if(v==src)                   fill=COL_NODE_SRC;
            else if(v==dst)              fill=COL_NODE_DST;
            else if(vertex_on_path(result,v)) fill=COL_NODE_PATH;
            draw_node(pos[v],v,fill);
        }



        /* agent */
        if(result&&result->found&&agent.state!=STATE_IDLE)
            DrawCircleV(agent.pos,14,COL_AGENT);

        /* arrived message */
        if(agent.state==STATE_DONE){
            const char *msg="Arrived at destination!";
            int fs=28,tw=MeasureText(msg,fs);
            DrawRectangle(WINDOW_W/2-tw/2-16,WINDOW_H/2-28,tw+32,56,(Color){0,0,0,180});
            DrawText(msg,WINDOW_W/2-tw/2,WINDOW_H/2-fs/2,fs,YELLOW);
        }

        draw_legend(result,src,dst);

        /* buttons */
        if(result&&result->found){
            if(draw_btn(btn_play,
                        agent.playing?"STOP":"PLAY",
                        agent.playing?COL_BTN_RED:COL_BTN_GRN,
                        agent.playing?COL_BTN_RED_H:COL_BTN_GRN_H)){
                if(agent.state==STATE_IDLE||agent.state==STATE_DONE)
                    agent_start(&agent,result,g,pos);
                else
                    agent.playing=!agent.playing;
            }
            if(draw_btn(btn_restart,"RESTART",COL_BTN,COL_BTN_HOV))
                agent_start(&agent,result,g,pos);
        }

        DrawText("ESC to close",WINDOW_W-160,WINDOW_H-28,16,LIGHTGRAY);
        EndDrawing();
    }
    CloseWindow();
}