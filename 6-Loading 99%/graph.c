/* ============================================================
   graph.c  --  Graph แบบ Adjacency List + Dijkstra's Algorithm
   ท่าเรือ = Vertex (เก็บใน array ports[])
   เส้นทางเดินเรือ = Edge (เก็บเป็น linked list ต่อจากแต่ละ port)
   ============================================================ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "graph.h"
#include "pqueue.h"

/* O(1) */
Graph *graph_create(void) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    if (!g) return NULL;
    g->port_count = 0;
    { int i; for (i = 0; i < MAX_PORTS; i++) {
        g->ports[i].id = i;
        g->ports[i].name[0] = '\0';
        g->ports[i].head = NULL;
    } }
    return g;
}

/* O(V+E) -- ต้อง free ทุก Edge (linked list) ของทุก port ก่อน แล้วค่อย free ตัว Graph */
void graph_destroy(Graph *g) {
    if (!g) return;
    { int i; for (i = 0; i < g->port_count; i++) {
        Edge *e = g->ports[i].head;
        while (e != NULL) {
            Edge *tmp = e;
            e = e->next;
            free(tmp);
        }
    } }
    free(g);
}

/* O(1) -- เพิ่มท่าเรือใหม่ คืน port id (index) หรือ -1 ถ้าเต็ม */
int graph_add_port(Graph *g, const char *name) {
    if (!g || g->port_count >= MAX_PORTS) return -1;
    int id = g->port_count;
    g->ports[id].id = id;
    snprintf(g->ports[id].name, MAX_NAME_LEN, "%s", name);
    g->ports[id].head = NULL;
    g->port_count++;
    return id;
}

/* O(1) -- เพิ่มเส้นทางเดินเรือ (แทรกที่หัว adjacency list ของ 'from')
   หมายเหตุ: เป็น directed edge ทางเดียว ถ้าเรือวิ่งสองทาง ผู้เรียกต้อง
   เรียก graph_add_route สองครั้งสลับ from/to (booking.c จัดการส่วนนี้) */
int graph_add_route(Graph *g, int from, int to, int route_id, BoatType boat,
                     int travel_time, int fare, int capacity) {
    if (!g || from < 0 || from >= g->port_count || to < 0 || to >= g->port_count)
        return 0;

    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) return 0;

    e->to_port     = to;
    e->route_id    = route_id;
    e->boat        = boat;
    e->travel_time = travel_time;
    e->fare        = fare;
    e->capacity    = capacity;
    { int d; for (d = 0; d < DAY_COUNT; d++)
        { int s; for (s = 0; s < MAX_DEPART_TIMES; s++) {
            e->seats_taken[d][s] = 0;
            { int b; for (b = 0; b < SEAT_BITMAP_BYTES; b++) e->seat_bits[d][s][b] = 0; }
        } } }
    e->next        = g->ports[from].head;
    g->ports[from].head = e;

    return 1;
}

/* O(V) -- ค้นหาชื่อท่าเรือแบบไล่ทีละตัว (จำนวนท่าเรือน้อย ไม่คุ้มทำ hash) */
int graph_find_port(const Graph *g, const char *name) {
    if (!g) return -1;
    { int i; for (i = 0; i < g->port_count; i++) {
        if (strcmp(g->ports[i].name, name) == 0) return i;
    } }
    return -1;
}

/* O(1) */
const char *graph_port_name(const Graph *g, int port_id) {
    if (!g || port_id < 0 || port_id >= g->port_count) return "?";
    return g->ports[port_id].name;
}

/* O(deg) -- ไล่ดู edge ของ 'from' หา edge ที่ไปยัง 'to' */
/* O(deg) -- หา edge ที่ตรงทั้งปลายทางและชนิดเรือ
   จำเป็นเพราะท่าคู่เดียวกันอาจมีทั้งเรือข้ามฟากและสปีดโบ๊ท (คนละ Edge) */
/* O(1) -- ที่นั่งที่ยังว่างของรอบ [day][slot] */
int graph_seats_left(const Edge *e, int day, int slot) {
    if (!e || day < 0 || day >= DAY_COUNT || slot < 0 || slot >= MAX_DEPART_TIMES) return 0;
    return e->capacity - e->seats_taken[day][slot];
}

/* O(1) -- จองที่นั่งว่างหมายเลขต่ำสุดของรอบ [day][slot]
   สแกน bitmap ทีละไบต์ (อย่างมาก 32 ไบต์) แทนการวน booking ทุกใบ
   คืนหมายเลขที่นั่ง (เริ่มที่ 1) หรือ -1 ถ้าเต็ม */
int graph_seat_take(Edge *e, int day, int slot) {
    if (!e || day < 0 || day >= DAY_COUNT || slot < 0 || slot >= MAX_DEPART_TIMES)
        return -1;
    if (e->seats_taken[day][slot] >= e->capacity) return -1;

    { int byte; for (byte = 0; byte < SEAT_BITMAP_BYTES; byte++) {
        if (e->seat_bits[day][slot][byte] == 0xFF) continue;   /* ไบต์นี้เต็มแล้ว ข้าม */
        { int bit; for (bit = 0; bit < 8; bit++) {
            int seat = byte * 8 + bit + 1;                     /* ที่นั่งเริ่มนับที่ 1 */
            if (seat > e->capacity) return -1;
            if (!(e->seat_bits[day][slot][byte] & (1u << bit))) {
                e->seat_bits[day][slot][byte] |= (unsigned char)(1u << bit);
                e->seats_taken[day][slot]++;
                return seat;
            }
        } }
    } }
    return -1;
}

/* O(1) -- จองที่นั่ง "หมายเลขที่ระบุ" (ใช้ตอน Undo การยกเลิก)
   ต่างจาก graph_seat_take ที่เลือกเลขว่างต่ำสุดให้เอง
   จำเป็นเพราะการคืนการจองต้องได้ "ที่นั่งเดิม" กลับมา
   ถ้าใช้ graph_seat_take อาจได้เลขอื่น แต่ Booking ยังจำเลขเดิม
   ทำให้ bitmap กับ Booking ไม่ตรงกัน แล้วคนถัดไปได้ที่นั่งซ้ำ
   คืนหมายเลขที่นั่ง, หรือ -1 ถ้าที่นั่งนั้นมีคนอยู่แล้ว/ค่าไม่ถูกต้อง */
int graph_seat_take_exact(Edge *e, int day, int slot, int seat_no) {
    int idx, byte, bit;
    if (!e || day < 0 || day >= DAY_COUNT || slot < 0 || slot >= MAX_DEPART_TIMES) return -1;
    if (seat_no < 1 || seat_no > e->capacity || seat_no > MAX_SEATS_PER_TRIP) return -1;

    idx  = seat_no - 1;
    byte = idx / 8;
    bit  = idx % 8;
    if (e->seat_bits[day][slot][byte] & (1u << bit)) return -1;   /* มีคนนั่งอยู่แล้ว */
    e->seat_bits[day][slot][byte] |= (unsigned char)(1u << bit);
    e->seats_taken[day][slot]++;
    return seat_no;
}

/* O(1) -- คืนที่นั่งกลับเข้าระบบตอนยกเลิกหรือ undo */
void graph_seat_release(Edge *e, int day, int slot, int seat_no) {
    int idx, byte, bit;
    if (!e || day < 0 || day >= DAY_COUNT || slot < 0 || slot >= MAX_DEPART_TIMES) return;
    if (seat_no < 1 || seat_no > MAX_SEATS_PER_TRIP) return;

    idx  = seat_no - 1;
    byte = idx / 8;
    bit  = idx % 8;
    if (!(e->seat_bits[day][slot][byte] & (1u << bit))) return;  /* ไม่ได้ถูกจองอยู่ ไม่ต้องทำอะไร */
    e->seat_bits[day][slot][byte] &= (unsigned char)~(1u << bit);
    if (e->seats_taken[day][slot] > 0) e->seats_taken[day][slot]--;
}

/* O(1) -- ที่นั่งที่ถูกจองรวมทุกวันทุกรอบ (ใช้แสดงผลเฉยๆ) */
int graph_seats_total_booked(const Edge *e) {
    int sum = 0;
    if (!e) return 0;
    { int d; for (d = 0; d < DAY_COUNT; d++)
        { int s; for (s = 0; s < MAX_DEPART_TIMES; s++)
            sum += e->seats_taken[d][s]; } }
    return sum;
}

Edge *graph_find_edge_boat(Graph *g, int from, int to, BoatType boat) {
    if (!g || from < 0 || from >= g->port_count) return NULL;
    Edge *e = g->ports[from].head;
    while (e != NULL) {
        if (e->to_port == to && e->boat == boat) return e;
        e = e->next;
    }
    return NULL;
}

/* O(deg) -- หา edge แรกที่ไปถึง to (ไม่สนชนิดเรือ) */
Edge *graph_find_edge(Graph *g, int from, int to) {
    if (!g || from < 0 || from >= g->port_count) return NULL;
    Edge *e = g->ports[from].head;
    while (e != NULL) {
        if (e->to_port == to) return e;
        e = e->next;
    }
    return NULL;
}

/* O(V+E) -- ไล่ทุก port ทุก edge หา route_id ที่ตรงกัน */
Edge *graph_find_route(Graph *g, int route_id) {
    if (!g) return NULL;
    { int i; for (i = 0; i < g->port_count; i++) {
        Edge *e = g->ports[i].head;
        while (e != NULL) {
            if (e->route_id == route_id) return e;
            e = e->next;
        }
    } }
    return NULL;
}

/* Dijkstra's Algorithm ด้วย Priority Queue (Min-Heap) -- O((V+E) log V)
   หลักการ: เริ่มจาก src ระยะทาง 0, ที่เหลือเป็นอนันต์
   ดึงตัวที่ระยะทางน้อยที่สุดออกจาก PQ มา "relax" edge ของมันเรื่อยๆ
   จนกว่า PQ จะว่าง หรือเจอ dst แล้ว (ในเวอร์ชันนี้ไล่จนครบเพื่อความง่าย) */
PathResult graph_shortest_path_filtered(const Graph *g, int src, int dst, int boat_filter) {
    PathResult result;
    memset(&result, 0, sizeof(result));
    result.found = 0;

    if (!g || src < 0 || src >= g->port_count || dst < 0 || dst >= g->port_count)
        return result;

    int dist[MAX_PORTS];       /* ระยะเวลาที่น้อยที่สุดจาก src ไปแต่ละ port */
    int fare[MAX_PORTS];       /* ค่าโดยสารสะสมตามเส้นทางที่สั้นที่สุด */
    int prev[MAX_PORTS];       /* port ก่อนหน้า ใช้ย้อนรอยเส้นทาง */
    int visited[MAX_PORTS];    /* 1 = สรุประยะทางที่สั้นสุดแล้ว (finalize) */
    BoatType used[MAX_PORTS];  /* เรือที่ใช้เดินทางเข้ามาถึง port นั้น */

    { int i; for (i = 0; i < g->port_count; i++) {
        dist[i] = INT_MAX;
        fare[i] = 0;
        prev[i] = -1;
        visited[i] = 0;
        used[i] = BOAT_FERRY;
    } }
    dist[src] = 0;

    /* ใช้ Priority Queue เก็บ (ระยะทาง, port_id) โดย priority = ระยะทางปัจจุบัน */
    PriorityQueue *pq = pq_create(g->port_count + 1);
    if (!pq) return result;

    /* เก็บ port_id ผ่าน pointer โดยแปลง int เป็น pointer ชั่วคราว (ใช้ static array กันปัญหา) */
    static int port_ids[MAX_PORTS];
    { int i; for (i = 0; i < g->port_count; i++) port_ids[i] = i; }

    pq_push(pq, 0, &port_ids[src]);

    while (!pq_is_empty(pq)) {
        long d;
        int *pu = (int *)pq_pop(pq, &d);
        int u = *pu;

        if (visited[u]) continue;   /* อาจมี entry ซ้ำใน PQ จากการ relax หลายรอบ ข้ามถ้า finalize แล้ว */
        visited[u] = 1;

        if (u == dst) break;   /* เจอปลายทางแล้ว หยุดได้เลย (ประหยัดเวลา) */

        /* ไล่ดู edge ทั้งหมดของ u แล้ว relax */
        Edge *e = g->ports[u].head;
        while (e != NULL) {
            int v = e->to_port;
            /* ข้าม edge ที่ไม่ใช่เรือชนิดที่ผู้ใช้เลือก (ANY_BOAT = ไม่กรอง) */
            if (boat_filter != ANY_BOAT && (int)e->boat != boat_filter) { e = e->next; continue; }
            if (!visited[v] && dist[u] != INT_MAX && dist[u] + e->travel_time < dist[v]) {
                dist[v] = dist[u] + e->travel_time;
                fare[v] = fare[u] + e->fare;
                prev[v] = u;
                used[v] = e->boat;
                pq_push(pq, dist[v], &port_ids[v]);
            }
            e = e->next;
        }
    }

    pq_destroy(pq);

    if (dist[dst] == INT_MAX) {
        result.found = 0;
        return result;
    }

    /* ย้อนรอยเส้นทางจาก dst กลับไป src โดยใช้ prev[] แล้วกลับลำดับ */
    int temp_path[MAX_PORTS];
    int len = 0;
    { int at; for (at = dst; at != -1; at = prev[at]) {
        temp_path[len++] = at;
    } }
    { int i; for (i = 0; i < len; i++) {
        result.path[i] = temp_path[len - 1 - i];
    } }
    /* boats[i] = เรือที่ใช้จาก path[i] ไป path[i+1]
       used[] เก็บ "เรือที่ใช้เข้ามาถึง port" จึงเลื่อน index ลง 1 ช่อง */
    { int i; for (i = 0; i + 1 < len; i++) {
        result.boats[i] = used[result.path[i + 1]];
    } }
    result.length = len;
    result.total_time = dist[dst];
    result.total_fare = fare[dst];
    result.found = 1;

    return result;
}

/* O(V+E) -- แสดงผังท่าเรือทั้งหมดและเส้นทางที่ออกจากแต่ละท่า */
/* เวอร์ชันเดิม -- ไม่กรองชนิดเรือ (เรียกต่อไปได้เหมือนเดิม) */
PathResult graph_shortest_path(const Graph *g, int src, int dst) {
    return graph_shortest_path_filtered(g, src, dst, ANY_BOAT);
}

void graph_print(const Graph *g) {
    if (!g) return;
    { int i; for (i = 0; i < g->port_count; i++) {
        printf("[%d] %s\n", i, g->ports[i].name);
        Edge *e = g->ports[i].head;
        while (e != NULL) {
            printf("      -> ");
            print_padded(g->ports[e->to_port].name, 22);
            print_padded(boat_name(e->boat), 14);
            printf("route#%-2d %3d นาที %4d บาท  ที่นั่ง %d/%d\n",
                   e->route_id, e->travel_time, e->fare,
                   graph_seats_total_booked(e), e->capacity);
            e = e->next;
        }
    } }
}

/* O(length) -- แสดงเส้นทางที่ได้จาก graph_shortest_path */
void path_print(const Graph *g, const PathResult *r) {
    if (!g || !r || !r->found) {
        printf(">> ไม่พบเส้นทาง\n");
        return;
    }
    printf(">> เส้นทางที่เร็วที่สุด (%d ช่วง)\n", r->length - 1);
    { int i; for (i = 0; i + 1 < r->length; i++) {
        printf("   %d) %s  ==[%s]==>  %s\n", i + 1,
               graph_port_name(g, r->path[i]),
               boat_name(r->boats[i]),
               graph_port_name(g, r->path[i + 1]));
    } }
    printf(">> เวลารวม: %d นาที, ค่าโดยสารรวม: %d บาท\n", r->total_time, r->total_fare);

    /* เตือนถ้าต้องเปลี่ยนชนิดเรือระหว่างทาง (ตารางเวลาคนละรอบ ต้องรอต่อเรือ) */
    { int i; for (i = 0; i + 2 < r->length; i++) {
        if (r->boats[i] != r->boats[i + 1]) {
            printf(">> หมายเหตุ: เส้นทางนี้ต้องเปลี่ยนชนิดเรือระหว่างทาง โปรดเผื่อเวลารอต่อเรือ\n");
            break;
        }
    } }
}
