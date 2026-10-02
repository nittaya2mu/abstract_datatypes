/* ชุดทดสอบ ADT -- ตรวจความถูกต้องเชิงพฤติกรรม ไม่ใช่แค่ว่าคอมไพล์ผ่าน */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* บน Windows: บังคับ console ให้แสดงผลเป็น UTF-8 (เหมือน main.c) */
#ifdef _WIN32
#include <windows.h>
#endif

#include "queue.h"
#include "stack.h"
#include "pqueue.h"
#include "hashtable.h"
#include "avltree.h"
#include "graph.h"
#include "data.h"

static int pass=0, fail=0;
#define CHECK(cond,msg) do{ if(cond){pass++;} else {fail++; printf("  [FAIL] %s\n",msg);} }while(0)

/* ---------- AVL ---------- */
static int tree_height(const AVLNode*n){return n?n->height:0;}
static int sorted_ok=1; static char last_key[32];
static void inorder_check(const AVLNode*n,int*cnt){
    if(!n)return;
    inorder_check(n->left,cnt);
    if(*cnt>0 && strcmp(last_key,n->key)>=0) sorted_ok=0;
    snprintf(last_key,sizeof(last_key),"%s",n->key); (*cnt)++;
    inorder_check(n->right,cnt);
}
static void test_avl(void){
    printf("\n[1] AVL Tree\n");
    AVLTree*t=avl_create(); char k[16]; Booking dummy;
    int N=2000;
    { int i; for(i=1;i<=N;i++){ snprintf(k,sizeof k,"BK%06d",i); avl_insert(t,k,&dummy); } }
    CHECK(t->count==N,"count หลัง insert ไม่ตรง");
    int cnt=0; sorted_ok=1; inorder_check(t->root,&cnt);
    CHECK(cnt==N,"inorder เดินไม่ครบทุก node");
    CHECK(sorted_ok,"inorder ไม่ได้เรียงจากน้อยไปมาก");
    int h=tree_height(t->root), lim=(int)(1.4405*(log(N+2)/log(2)));
    printf("  insert เรียงขึ้น %d รายการ -> height=%d (เพดาน AVL=%d, BST ธรรมดาจะเป็น %d)\n",N,h,lim,N);
    CHECK(h<=lim,"height เกินเพดานของ AVL แปลว่าไม่ได้ rebalance");
    snprintf(k,sizeof k,"BK%06d",1000);
    CHECK(avl_search(t,k)!=NULL,"search หา key ที่มีอยู่ไม่เจอ");
    CHECK(avl_search(t,"BK999999")==NULL,"search คืนค่าให้ key ที่ไม่มีอยู่");
    CHECK(avl_delete(t,k)==1,"delete key ที่มีอยู่ล้มเหลว");
    CHECK(avl_search(t,k)==NULL,"ลบแล้วยังหาเจอ");
    CHECK(t->count==N-1,"count หลัง delete ไม่ตรง");
    cnt=0; sorted_ok=1; inorder_check(t->root,&cnt);
    CHECK(sorted_ok&&cnt==N-1,"หลัง delete โครงสร้างเสียหาย");
    CHECK(tree_height(t->root)<=lim,"หลัง delete height เกินเพดาน");
    avl_destroy(t);
}
/* ---------- Hash ---------- */
static void test_hash(void){
    printf("\n[2] Hash Table\n");
    HashTable*ht=ht_create(); char k[16]; Booking d[3000]; int N=3000;
    { int i; for(i=0;i<N;i++){ snprintf(k,sizeof k,"BK%06d",i); ht_insert(ht,k,&d[i]); } }
    CHECK(ht->count==N,"count ไม่ตรง");
    int allfound=1;
    { int i; for(i=0;i<N;i++){ snprintf(k,sizeof k,"BK%06d",i); if(ht_search(ht,k)!=&d[i]) allfound=0; } }
    CHECK(allfound,"ค้นเจอไม่ครบ/คืนค่าผิดตัว (chaining พัง)");
    CHECK(ht_search(ht,"NOPE")==NULL,"คืนค่าให้ key ที่ไม่มี");
    snprintf(k,sizeof k,"BK%06d",500);
    CHECK(ht_delete(ht,k)==1,"delete ล้มเหลว");
    CHECK(ht_search(ht,k)==NULL,"ลบแล้วยังเจอ");
    CHECK(ht_delete(ht,k)==0,"ลบซ้ำแล้วคืนค่าสำเร็จ");
    CHECK(ht->count==N-1,"count หลังลบไม่ตรง");
    printf("  ใส่ %d รายการในตาราง %d ช่อง -> เฉลี่ยโซ่ยาว %.2f (ค้นถูกทุกตัว)\n",N,HASH_SIZE,(double)N/HASH_SIZE);
    ht_destroy(ht);
}
/* ---------- PQ ---------- */
static void test_pq(void){
    printf("\n[3] Priority Queue (Min-Heap)\n");
    PriorityQueue*pq=pq_create(4);   /* จงใจให้เล็กเพื่อบังคับ realloc */
    int N=5000; long p; int ok=1; static int v[5000];
    srand(42);
    { int i; for(i=0;i<N;i++){ v[i]=i; pq_push(pq,rand()%100000,&v[i]); } }
    CHECK(pq_size(pq)==N,"size ไม่ตรงหลัง push (realloc อาจพัง)");
    long prev=-1;
    { int i; for(i=0;i<N;i++){ pq_pop(pq,&p); if(p<prev) ok=0; prev=p; } }
    CHECK(ok,"pop ออกมาไม่เรียงจากน้อยไปมาก");
    CHECK(pq_is_empty(pq),"pop หมดแล้ว size ไม่เป็น 0");
    CHECK(pq_pop(pq,&p)==NULL,"pop จาก heap ว่างไม่คืน NULL");
    /* ทดสอบสูตรลำดับสิทธิ์จริงของโปรเจกต์ */
    pq_destroy(pq); pq=pq_create(8);
    long pr_express_late = 0L*1000000L+9999;   /* ด่วน จองทีหลังสุด */
    long pr_normal_early = 2L*1000000L+1;      /* ทั่วไป จองก่อนสุด */
    pq_push(pq,pr_normal_early,(void*)1); pq_push(pq,pr_express_late,(void*)2);
    pq_pop(pq,&p);
    CHECK(p==pr_express_late,"ตั๋วด่วนไม่ได้สิทธิ์ก่อนตั๋วทั่วไป");
    printf("  ด่วน(จองที่ 9999)=%ld ชนะ ทั่วไป(จองที่ 1)=%ld -> ประเภทสำคัญกว่าเวลาจอง\n",pr_express_late,pr_normal_early);
    pq_destroy(pq);
}
/* ---------- Queue / Stack ---------- */
static void test_queue_stack(void){
    printf("\n[4] Queue (FIFO) + Stack (LIFO)\n");
    Queue*q=queue_create(); static int v[100]; int ok=1;
    { int i; for(i=0;i<100;i++){ v[i]=i; queue_enqueue(q,&v[i]); } }
    CHECK(queue_size(q)==100,"queue size ไม่ตรง");
    { int i; for(i=0;i<100;i++){ int*x=queue_dequeue(q); if(*x!=i) ok=0; } }
    CHECK(ok,"queue ไม่ได้ออกตามลำดับเข้าก่อน-ออกก่อน");
    CHECK(queue_is_empty(q),"queue ว่างแล้วแต่ size ไม่เป็น 0");
    CHECK(queue_dequeue(q)==NULL,"dequeue จากคิวว่างไม่คืน NULL");
    { int i; for(i=0;i<50;i++){ v[i]=i; queue_enqueue(q,&v[i]); } }   /* ใช้ซ้ำหลังว่าง */
    CHECK(queue_size(q)==50,"ใช้คิวซ้ำหลังว่างแล้วพัง (rear ไม่ถูก reset)");
    queue_destroy(q);

    Stack*s=stack_create(); Action a; ok=1;
    { int i; for(i=0;i<100;i++){ a.type=ACT_BOOK; a.route_id=i; stack_push(s,a); } }
    { int i; for(i=99;i>=0;i--){ stack_pop(s,&a); if(a.route_id!=i) ok=0; } }
    CHECK(ok,"stack ไม่ได้ออกตามลำดับเข้าทีหลัง-ออกก่อน");
    CHECK(stack_is_empty(s),"stack ว่างแล้วแต่ size ไม่เป็น 0");
    CHECK(stack_pop(s,&a)==0,"pop จาก stack ว่างไม่คืน 0");
    stack_destroy(s);
}
/* ---------- Graph / Dijkstra ---------- */
static void test_graph(void){
    printf("\n[5] Graph + Dijkstra\n");
    Graph*g=graph_create();
    { int i; for(i=0;i<SAMPLE_PORT_COUNT;i++) graph_add_port(g,SAMPLE_PORTS[i]); }
    { int i; for(i=0;i<SAMPLE_ROUTE_COUNT;i++){
        int f=graph_find_port(g,SAMPLE_ROUTES[i].from), t=graph_find_port(g,SAMPLE_ROUTES[i].to);
        graph_add_route(g,f,t,i,SAMPLE_ROUTES[i].boat,SAMPLE_ROUTES[i].travel_time,SAMPLE_ROUTES[i].fare,SAMPLE_ROUTES[i].capacity);
        graph_add_route(g,t,f,i,SAMPLE_ROUTES[i].boat,SAMPLE_ROUTES[i].travel_time,SAMPLE_ROUTES[i].fare,SAMPLE_ROUTES[i].capacity);
    } }
    /* ตรวจด้วย Bellman-Ford (อัลกอริทึมคนละตัว) ว่าได้คำตอบเดียวกัน */
    int n=g->port_count, bad=0;
    { int s; for(s=0;s<n;s++){
        int dist[64]; { int i; for(i=0;i<n;i++) dist[i]=1000000000; } dist[s]=0;
        { int it; for(it=0;it<n-1;it++)
            { int u; for(u=0;u<n;u++)
                { Edge *e; for(e=g->ports[u].head;e;e=e->next)
                    if(dist[u]<1000000000 && dist[u]+e->travel_time<dist[e->to_port])
                        dist[e->to_port]=dist[u]+e->travel_time; } } }
        { int d; for(d=0;d<n;d++){
            PathResult r=graph_shortest_path(g,s,d);
            if(dist[d]>=1000000000){ if(r.found&&s!=d) bad++; }
            else if(!r.found||r.total_time!=dist[d]) bad++;
        } }
    } }
    CHECK(bad==0,"Dijkstra ให้คำตอบไม่ตรงกับ Bellman-Ford");
    printf("  เทียบทุกคู่ต้นทาง-ปลายทาง %dx%d=%d คู่ กับ Bellman-Ford -> ต่างกัน %d คู่\n",n,n,n*n,bad);
    PathResult r=graph_shortest_path(g,graph_find_port(g,"ท่าเรือรัษฎา"),graph_find_port(g,"เกาะราชา"));
    CHECK(r.found&&r.total_time==90,"เส้นทางตัวอย่างให้เวลาไม่ถูก");
    /* ปลายทางที่ไปไม่ถึง */
    graph_add_port(g,"เกาะลอยเดี่ยว");
    r=graph_shortest_path(g,0,graph_find_port(g,"เกาะลอยเดี่ยว"));
    CHECK(!r.found,"ท่าที่ไม่มีเส้นทางเชื่อม แต่ Dijkstra บอกว่าไปถึงได้");
    graph_destroy(g);
}
int main(void){
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    printf("========== ชุดทดสอบ ADT ==========");
    test_avl(); test_hash(); test_pq(); test_queue_stack(); test_graph();
    printf("\n==================================\nผ่าน %d / ล้มเหลว %d\n",pass,fail);
    return fail?1:0;
}
