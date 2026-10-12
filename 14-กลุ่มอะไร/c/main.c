/* C11 command-line implementation of the project ADTs.
   Build with -DLAUNDRY=1 (group 10) or -DLAUNDRY=0 (group 14).
   Native C GUI and console share these domain transitions and ADTs. */
#include "adt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
/* Invariants and self-tests are deliberately active in optimized builds too. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#ifndef LAUNDRY
#define LAUNDRY 1
#endif
#if LAUNDRY
#define NAME "SmartLaundry C | ADT Group 10"
#define PREFIX 'Q'
#define LIMIT 50
#else
#define NAME "SmartParking C | ADT Group 14"
#define PREFIX 'R'
#define LIMIT 10
#endif
enum { UNUSED, WAITING, READY, RUNNING, DONE };
static const char *state[]={"FREE","WAITING","READY","RUNNING","DONE"};
typedef struct { char code[KEY_BYTES],user[KEY_BYTES];int status,machine,duration;double deadline; } Ticket;
typedef struct {
    Ticket tickets[MAX_TICKETS];int owner[10],online[10],count;unsigned sequence,event_sequence;
    Hash codes,users;Heap heap;Queue queue;Stack stack;Log log;
    double now,offset;time_t started;int distance[16],previous[16];
} System;
static const int graph_edges[15][3]={
    {0,1,7},{1,2,12},{2,3,12},{3,4,12},{4,5,12},
    {1,6,3},{1,7,9},{2,8,3},{2,9,9},{3,10,3},{3,11,9},{4,12,3},{4,13,9},{5,14,3},{5,15,9}
};
/* Dense adjacency matrix implementation; fixed 16 vertices, O(V^2) preparation. */
static void dijkstra(System *s) {
    int adjacency[16][16],visited[16]={0};
    for(int i=0;i<16;i++){s->distance[i]=1000000;s->previous[i]=-1;for(int j=0;j<16;j++)adjacency[i][j]=-1;}
    for(int e=0;e<15;e++){int a=graph_edges[e][0],b=graph_edges[e][1],w=graph_edges[e][2];adjacency[a][b]=adjacency[b][a]=w;}
    s->distance[0]=0;
    for(int step=0;step<16;step++){
        int u=-1;for(int i=0;i<16;i++)if(!visited[i]&&(u<0||s->distance[i]<s->distance[u]))u=i;
        if(u<0||s->distance[u]==1000000)break;visited[u]=1;
        for(int v=0;v<16;v++)if(adjacency[u][v]>=0&&s->distance[u]+adjacency[u][v]<s->distance[v]){
            s->distance[v]=s->distance[u]+adjacency[u][v];s->previous[v]=u;
        }
    }
}
static int init(System *s) {
    memset(s,0,sizeof(*s));for(int i=0;i<10;i++){s->owner[i]=-1;s->online[i]=1;}
    heap_init(&s->heap);s->started=time(NULL);dijkstra(s);
    if(!hash_init(&s->codes,LIMIT)||!hash_init(&s->users,LIMIT)){hash_free(&s->codes);hash_free(&s->users);return 0;}return 1;
}
static void destroy(System *s){hash_free(&s->codes);hash_free(&s->users);}
static int normalize(const char *input,char *out) {
    int len=0;for(;*input;input++)if(!isspace((unsigned char)*input)){
        if(len>=KEY_BYTES-1)return 0;out[len++]=(char)toupper((unsigned char)*input);
    }
    out[len]=0;return len>0;
}
static int reserved_format(const char *key) {
    if(key[0]!=PREFIX||!key[1])return 0;for(int i=1;key[i];i++)if(!isdigit((unsigned char)key[i]))return 0;return 1;
}
static int lookup(System *s,const char *key) {
    char canonical[KEY_BYTES];if(!normalize(key,canonical))return -1;
    int id=hash_get(&s->codes,canonical);return id>=0?id:hash_get(&s->users,canonical);
}
static void record(System *s,int id,const char *action) {Ticket *t=&s->tickets[id];log_append(&s->log,s->now,action,t->code,t->user,t->machine);}
static void dispatch(System *s) {
    if(!LAUNDRY)return;
    for(int m=0;m<10&&s->queue.size;m++)if(s->online[m]&&s->owner[m]<0){
        int id=queue_pop(&s->queue);Ticket *t=&s->tickets[id];t->machine=m;t->status=READY;t->deadline=s->now+300;s->owner[m]=id;
        assert(heap_push(&s->heap,(Event){t->deadline,++s->event_sequence,id}));record(s,id,"ASSIGN");
    }
}
static void release(System *s,int id,const char *action) {
    Ticket *t=&s->tickets[id];record(s,id,action);
    heap_remove(&s->heap,id);queue_remove(&s->queue,id);stack_remove(&s->stack,id);
    hash_remove(&s->codes,t->code);hash_remove(&s->users,t->user);
    if(t->machine>=0)s->owner[t->machine]=-1;t->status=UNUSED;s->count--;dispatch(s);
}
static void tick_to(System *s,double now) {
    while(s->heap.size&&s->heap.items[0].at<=now){
        Event e=heap_pop(&s->heap);s->now=e.at;Ticket *t=&s->tickets[e.id];
        if(t->status==READY)release(s,e.id,"EXPIRED");
        else if(LAUNDRY&&t->status==RUNNING){t->status=DONE;record(s,e.id,"FINISHED");}
        else assert(0);
    }s->now=now;
}
static int join(System *s,const char *user,int duration) {
    char key[KEY_BYTES];if(!normalize(user,key)||reserved_format(key)||hash_get(&s->users,key)>=0||s->count==LIMIT)return -1;
    if(LAUNDRY&&duration!=1200&&duration!=1800&&duration!=2700)return -1;
    int m=-1;
    if(!LAUNDRY){for(int i=0;i<10;i++)if(s->owner[i]<0&&(m<0||s->distance[6+i]<s->distance[6+m]))m=i;if(m<0)return -1;}
    int id=0;while(s->tickets[id].status!=UNUSED)id++;
    Ticket *t=&s->tickets[id];memset(t,0,sizeof(*t));
    snprintf(t->code,sizeof(t->code),"%c%04u",PREFIX,++s->sequence);strcpy(t->user,key);t->duration=duration;t->machine=m;
    assert(hash_put(&s->codes,t->code,id));assert(hash_put(&s->users,t->user,id));assert(stack_push(&s->stack,id));s->count++;
    if(LAUNDRY){t->status=WAITING;assert(queue_push(&s->queue,id));record(s,id,"JOIN");dispatch(s);}
    else {t->status=READY;t->deadline=s->now+300;s->owner[m]=id;assert(heap_push(&s->heap,(Event){t->deadline,++s->event_sequence,id}));record(s,id,"RESERVE");}
    return id;
}
static int start(System *s,const char *key) {
    int id=lookup(s,key);if(id<0||s->tickets[id].status!=READY)return 0;
    Ticket *t=&s->tickets[id];heap_remove(&s->heap,id);stack_remove(&s->stack,id);t->status=RUNNING;
    if(LAUNDRY){t->deadline=s->now+t->duration;assert(heap_push(&s->heap,(Event){t->deadline,++s->event_sequence,id}));}
    record(s,id,LAUNDRY?"START":"ENTER");return 1;
}
static int pickup(System *s,const char *key) {
    int id=lookup(s,key);if(id<0||s->tickets[id].status!=(LAUNDRY?DONE:RUNNING))return 0;
    release(s,id,LAUNDRY?"PICKUP":"EXIT");return 1;
}
static int cancel(System *s,const char *key) {
    int id=lookup(s,key);if(id<0||(s->tickets[id].status!=WAITING&&s->tickets[id].status!=READY))return 0;
    release(s,id,"CANCEL");return 1;
}
static int undo(System *s){if(!s->stack.size)return 0;release(s,s->stack.items[s->stack.size-1],"UNDO");return 1;}
static int maintenance(System *s,int m){if(!LAUNDRY||m<0||m>=10||s->owner[m]>=0)return 0;s->online[m]=!s->online[m];dispatch(s);return 1;}
static void invariant(System *s) {
    int count=0,waiting=0,pending=0,events=0;
    for(int i=0;i<MAX_TICKETS;i++)if(s->tickets[i].status){
        Ticket *t=&s->tickets[i];count++;assert(hash_get(&s->codes,t->code)==i&&hash_get(&s->users,t->user)==i);
        if(t->status==WAITING)waiting++;else assert(t->machine>=0&&s->owner[t->machine]==i&&s->online[t->machine]);
        if(t->status==READY||t->status==WAITING)pending++;
        int needs_event=t->status==READY||(LAUNDRY&&t->status==RUNNING);
        if(needs_event){events++;assert(s->heap.positions[i]>=0&&s->heap.items[s->heap.positions[i]].at==t->deadline);}else assert(s->heap.positions[i]==-1);
    }
    assert(count==s->count&&count==s->codes.size&&count==s->users.size&&waiting==s->queue.size&&pending==s->stack.size&&events==s->heap.size);
    for(int i=0;i<s->queue.size;i++)assert(s->tickets[s->queue.items[(s->queue.head+i)%MAX_TICKETS]].status==WAITING);
    for(int i=0;i<s->stack.size;i++)assert(s->tickets[s->stack.items[i]].status==WAITING||s->tickets[s->stack.items[i]].status==READY);
    for(int m=0;m<10;m++)if(s->owner[m]>=0)assert(s->tickets[s->owner[m]].machine==m&&s->tickets[s->owner[m]].status!=UNUSED);
    if(waiting)for(int m=0;m<10;m++)assert(!s->online[m]||s->owner[m]>=0);
    assert(s->log.size<=30);
}
static void status(System *s) {
    printf("\n%s | time=%.0f s | active=%d | waiting=%d | log=%d/30\n",NAME,s->now,s->count,s->queue.size,s->log.size);
    for(int m=0;m<10;m++){
        printf("%c%d ",LAUNDRY?'W':'P',m+1);int id=s->owner[m];
        if(id<0)printf("%s",s->online[m]?"FREE":"OFFLINE");
        else {Ticket *t=&s->tickets[id];printf("%s %s %s",t->code,t->user,LAUNDRY?state[t->status]:(t->status==READY?"RESERVED":"OCCUPIED"));if(t->status==READY||(LAUNDRY&&t->status==RUNNING))printf(" deadline=%.0f",t->deadline);}
        if(!LAUNDRY)printf(" distance=%dm",s->distance[6+m]);puts("");
    }
    if(s->queue.size){printf("FIFO: ");for(int i=0;i<s->queue.size;i++)printf("%s ",s->tickets[s->queue.items[(s->queue.head+i)%MAX_TICKETS]].code);puts("");}
}
static void demo(System *s) {
    if(LAUNDRY){for(int i=0;i<10;i++){char room[20];snprintf(room,sizeof(room),"A%d",101+i);int id=join(s,room,1200);assert(id>=0);assert(start(s,s->tickets[id].code));}join(s,"B201",1800);join(s,"B202",2700);join(s,"B203",1200);}
    else {int a=join(s,"DEMO-001",0);assert(start(s,s->tickets[a].code));int b=join(s,"DEMO-002",0);assert(start(s,s->tickets[b].code));join(s,"DEMO-003",0);}
    invariant(s);status(s);
}
static void selftest(void) {
    Hash h;assert(hash_init(&h,1000));char key[80];
    for(int i=0;i<1000;i++){snprintf(key,sizeof(key),"key%d",i);assert(hash_put(&h,key,i));}
    for(int i=0;i<1000;i++){snprintf(key,sizeof(key),"key%d",i);assert(hash_get(&h,key)==i);assert(hash_remove(&h,key));assert(hash_get(&h,key)==-1);}
    for(int i=0;i<1000;i++){snprintf(key,sizeof(key),"again%d",i);assert(hash_put(&h,key,i));}hash_free(&h);
    Heap heap;heap_init(&heap);for(int i=0;i<50;i++)assert(heap_push(&heap,(Event){(double)(50-i),(unsigned)i,i}));
    for(int i=0;i<50;i+=3)assert(heap_remove(&heap,i));double last=-1;while(heap.size){Event e=heap_pop(&heap);assert(e.at>=last);last=e.at;}
    System s;assert(init(&s));demo(&s);
    if(LAUNDRY){assert(s.queue.size==3);tick_to(&s,1200);assert(s.tickets[0].status==DONE&&s.queue.size==3);assert(pickup(&s,"Q0001"));assert(lookup(&s,"Q0011")>=0);tick_to(&s,1500);assert(lookup(&s,"Q0011")<0&&s.owner[0]==11);assert(start(&s,"Q0012"));assert(undo(&s));assert(lookup(&s,"Q0013")<0&&lookup(&s,"Q0012")>=0);}
    else {assert(s.tickets[2].machine==2&&s.distance[8]==22);tick_to(&s,300);assert(lookup(&s,"R0003")<0);assert(s.count==2);assert(pickup(&s,"R0001"));int id=join(&s,"NEW",0);assert(s.tickets[id].machine==0);assert(undo(&s));}
    invariant(&s);destroy(&s);assert(init(&s));
    if(LAUNDRY){for(int m=0;m<10;m++)assert(maintenance(&s,m));for(int i=0;i<50;i++){snprintf(key,sizeof(key),"Z%d",i);assert(join(&s,key,1200)>=0);}assert(join(&s,"FULL",1200)<0);assert(!cancel(&s,"UNKNOWN"));assert(cancel(&s,"Z20"));assert(undo(&s));invariant(&s);destroy(&s);assert(init(&s));for(int m=1;m<10;m++)assert(maintenance(&s,m));join(&s,"X1",1200);join(&s,"X2",1200);join(&s,"X3",1200);tick_to(&s,900);assert(s.count==0);invariant(&s);}
    else {for(int i=0;i<10;i++){snprintf(key,sizeof(key),"Z%d",i);assert(join(&s,key,0)>=0);}assert(join(&s,"FULL",0)<0);invariant(&s);destroy(&s);assert(init(&s));}
    destroy(&s);assert(init(&s));for(int r=0;r<1000;r++){
        int id=join(&s,"CYCLE",1200);assert(id>=0);char code[80];strcpy(code,s.tickets[id].code);assert(join(&s,"CYCLE",1200)<0);assert(start(&s,code));if(LAUNDRY)tick_to(&s,s.now+1200);assert(pickup(&s,code));invariant(&s);
    }assert(s.log.size==30&&s.count==0);destroy(&s);
    assert(init(&s));unsigned rng=10;
    for(int r=0;r<1500;r++){
        rng=rng*1664525u+1013904223u;unsigned op=rng%6;snprintf(key,sizeof(key),"ROOM%u",(rng/6)%70);
        if(op==0)join(&s,key,1200);else if(op==1)start(&s,key);else if(op==2)pickup(&s,key);else if(op==3)cancel(&s,key);else if(op==4)undo(&s);else tick_to(&s,s.now+300);invariant(&s);
    }destroy(&s);puts("SELFTEST PASS: hash reuse, indexed heap deletion, domain demo, boundaries/capacity, 1000 cycles, 1500 randomized commands.");
}
int main(int argc,char **argv) {
    if(argc>1&&strcmp(argv[1],"--benchmark")==0)return benchmark_main(argc,argv);
    if(argc>1&&strcmp(argv[1],"--self-test")==0){selftest();return 0;}
    System s;if(!init(&s)){fputs("Out of memory\n",stderr);return 1;}
    puts(NAME);puts("Commands: join USER [20|30|45], start CODE/USER, pickup CODE/USER, cancel CODE/USER,");
    puts("undo, advance SECONDS, maintenance MACHINE, status, history, demo, quit.");
    puts("Parking: join=reserve, start=enter, pickup=exit. C checks elapsed time before each command.");
    if(argc>1&&strcmp(argv[1],"--demo")==0)demo(&s);
    char line[256],cmd[40],arg[80],value[40];
    for(;;){printf("> ");fflush(stdout);if(!fgets(line,sizeof(line),stdin))break;
        if(!strchr(line,'\n')&&!feof(stdin)){int ch;while((ch=getchar())!='\n'&&ch!=EOF){}puts("Command too long");continue;}
        cmd[0]=arg[0]=value[0]=0;char extra[2];int parts=sscanf(line,"%39s %79s %39s %1s",cmd,arg,value,extra);
        if(parts<1)continue;if(parts>3){puts("Too many arguments");continue;}
        double wall=difftime(time(NULL),s.started)+s.offset;tick_to(&s,fmax(s.now,wall));int ok=0;
        if(strcmp(cmd,"quit")==0)break;
        else if(strcmp(cmd,"status")==0){status(&s);continue;}
        else if(strcmp(cmd,"history")==0){log_print(&s.log);continue;}
        else if(strcmp(cmd,"demo")==0){if(s.count){puts("Demo requires empty system");continue;}demo(&s);continue;}
        else if(strcmp(cmd,"join")==0&&parts>=2){int minutes=20;if(parts==3){char *end;long number=strtol(value,&end,10);if(*end||(number!=20&&number!=30&&number!=45)){puts("Program must be 20, 30 or 45");continue;}minutes=(int)number;}int id=join(&s,arg,minutes*60);ok=id>=0;if(ok)printf("CREATED %s %s\n",s.tickets[id].code,s.tickets[id].user);}
        else if(strcmp(cmd,"start")==0&&parts==2)ok=start(&s,arg);
        else if(strcmp(cmd,"pickup")==0&&parts==2)ok=pickup(&s,arg);
        else if(strcmp(cmd,"cancel")==0&&parts==2)ok=cancel(&s,arg);
        else if(strcmp(cmd,"undo")==0&&parts==1)ok=undo(&s);
        else if(strcmp(cmd,"maintenance")==0&&parts==2){char *end;long m=strtol(arg,&end,10);if(!*end&&m>=1&&m<=10)ok=maintenance(&s,(int)m-1);}
        else if(strcmp(cmd,"advance")==0&&parts==2){char *end;double seconds=strtod(arg,&end);if(!*end&&isfinite(seconds)&&seconds>=0&&isfinite(s.now+seconds)){s.offset+=seconds;tick_to(&s,s.now+seconds);ok=1;}}
        else {puts("Unknown command or wrong arguments");continue;}
        puts(ok?"OK":"REJECTED: invalid/duplicate/full/expired state");invariant(&s);status(&s);
    }destroy(&s);return 0;
}
