/* ============================================================
   graph.h  --  Graph แบบ Adjacency List + Dijkstra
   ท่าเรือ = Vertex,  เส้นทางเดินเรือ = Edge (น้ำหนัก = เวลาเดินทาง)
   ============================================================ */
#ifndef GRAPH_H
#define GRAPH_H

#include "types.h"
#include "data.h"   /* BoatType, boat_name() */

typedef struct Edge {
    int          to_port;      /* ปลายทาง */
    int          route_id;     /* รหัสเที่ยวเรือ */
    BoatType     boat;         /* เส้นนี้วิ่งด้วยเรือชนิดไหน (ferry/speedboat) */
    int          travel_time;  /* นาที -- ใช้เป็น weight ของ Dijkstra */
    int          fare;         /* ค่าโดยสาร (บาท) */
    int          capacity;     /* ที่นั่งทั้งหมด */
    /* ที่นั่งที่ถูกจองแล้ว แยกตาม [วัน][รอบเวลา]
       จำเป็นเพราะเรือลำเดียวกันวิ่งหลายรอบต่อวัน แต่ละรอบมีที่นั่งของตัวเอง */
    int          seats_taken[DAY_COUNT][MAX_DEPART_TIMES];
    /* bitmap บอกว่าที่นั่งหมายเลขใดถูกจองแล้ว (1 บิต = 1 ที่นั่ง)
       ใช้แทนการวนดู booking ทุกใบ ทำให้หาที่นั่งว่างเร็วขึ้นจาก O(n x capacity) เหลือ O(1) */
    unsigned char seat_bits[DAY_COUNT][MAX_DEPART_TIMES][SEAT_BITMAP_BYTES];
    struct Edge *next;
} Edge;

typedef struct {
    int   id;
    char  name[MAX_NAME_LEN];
    Edge *head;                /* หัวของ adjacency list */
} Port;

typedef struct {
    Port ports[MAX_PORTS];
    int  port_count;
} Graph;

/* ผลลัพธ์การหาเส้นทางสั้นที่สุด */
typedef struct {
    int      path[MAX_PORTS];   /* ลำดับ port id จากต้นทางถึงปลายทาง */
    BoatType boats[MAX_PORTS];  /* boats[i] = เรือที่ใช้เดินทางจาก path[i] -> path[i+1] */
    int length;            /* จำนวนท่าในเส้นทาง */
    int total_time;        /* เวลารวม (นาที) */
    int total_fare;        /* ค่าโดยสารรวม */
    int found;             /* 1 = เจอเส้นทาง, 0 = ไปไม่ถึง */
} PathResult;

Graph      *graph_create(void);                                  /* O(1)     */
void        graph_destroy(Graph *g);                             /* O(V+E)   */
int         graph_add_port(Graph *g, const char *name);          /* O(1) คืน port id หรือ -1 */
int         graph_add_route(Graph *g, int from, int to, int route_id, BoatType boat,
                            int travel_time, int fare, int capacity); /* O(1) */
int         graph_find_port(const Graph *g, const char *name);   /* O(V)     */
const char *graph_port_name(const Graph *g, int port_id);        /* O(1)     */
Edge       *graph_find_edge(Graph *g, int from, int to);         /* O(deg)   */
Edge       *graph_find_edge_boat(Graph *g, int from, int to, BoatType boat); /* O(deg) */
Edge       *graph_find_route(Graph *g, int route_id);            /* O(V+E)   */
int         graph_seats_left(const Edge *e, int day, int slot);  /* O(1) ที่นั่งเหลือของรอบนั้น */
/* จองที่นั่งว่างหมายเลขต่ำสุดของรอบนั้น คืนหมายเลขที่นั่ง หรือ -1 ถ้าเต็ม -- O(1) */
int         graph_seat_take(Edge *e, int day, int slot);
/* จองที่นั่งหมายเลขที่ระบุ (ใช้ตอน Undo การยกเลิก) -- O(1) คืน -1 ถ้ามีคนนั่งอยู่แล้ว */
int         graph_seat_take_exact(Edge *e, int day, int slot, int seat_no);
/* คืนที่นั่งกลับเข้าระบบ (ตอนยกเลิก/undo) -- O(1) */
void        graph_seat_release(Edge *e, int day, int slot, int seat_no);
int         graph_seats_total_booked(const Edge *e);             /* O(1) รวมทุกวันทุกรอบ */

/* Dijkstra ด้วย Priority Queue -- O((V+E) log V)
   boat_filter: ใส่ BOAT_FERRY / BOAT_SPEEDBOAT เพื่อจำกัดให้ใช้เรือชนิดเดียว
                ใส่ -1 (ANY_BOAT) = ใช้เรือชนิดไหนก็ได้ */
#define ANY_BOAT (-1)
PathResult  graph_shortest_path_filtered(const Graph *g, int src, int dst, int boat_filter);
PathResult  graph_shortest_path(const Graph *g, int src, int dst);  /* = filtered ด้วย ANY_BOAT */

void        graph_print(const Graph *g);                         /* แสดงผังท่าเรือทั้งหมด */
void        path_print(const Graph *g, const PathResult *r);     /* แสดงเส้นทางที่หาได้ */

#endif /* GRAPH_H */
