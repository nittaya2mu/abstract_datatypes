#include "adt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static uint64_t hash_key(const char *s) {
    uint64_t x=UINT64_C(14695981039346656037);
    while (*s) { x^=(unsigned char)*s++; x*=UINT64_C(1099511628211); }
    return x;
}
int hash_init(Hash *h,int capacity) {
    memset(h,0,sizeof(*h)); if(capacity<1 || capacity>100000) return 0;
    h->capacity=capacity; h->bucket_count=2*capacity+1;
    h->buckets=malloc((size_t)h->bucket_count*sizeof(int));
    h->entries=calloc((size_t)capacity,sizeof(HashEntry));
    if(!h->buckets || !h->entries) { hash_free(h); return 0; }
    for(int i=0;i<h->bucket_count;i++) h->buckets[i]=-1;
    for(int i=0;i<capacity;i++) h->entries[i].next=i+1<capacity?i+1:-1;
    h->free_head=0; return 1;
}
void hash_free(Hash *h) { free(h->buckets);free(h->entries);memset(h,0,sizeof(*h)); }
int hash_get(const Hash *h,const char *key) {
    int b=(int)(hash_key(key)%(uint64_t)h->bucket_count);
    for(int i=h->buckets[b];i!=-1;i=h->entries[i].next)
        if(strcmp(h->entries[i].key,key)==0) return h->entries[i].value;
    return -1;
}
int hash_put(Hash *h,const char *key,int value) {
    if(strlen(key)>=KEY_BYTES || value<0) return 0;
    int b=(int)(hash_key(key)%(uint64_t)h->bucket_count);
    for(int i=h->buckets[b];i!=-1;i=h->entries[i].next)
        if(strcmp(h->entries[i].key,key)==0) { h->entries[i].value=value; return 1; }
    if(h->free_head==-1) return 0;
    int i=h->free_head;h->free_head=h->entries[i].next;
    strcpy(h->entries[i].key,key);h->entries[i].value=value;
    h->entries[i].next=h->buckets[b];h->buckets[b]=i;h->size++;return 1;
}
int hash_remove(Hash *h,const char *key) {
    int b=(int)(hash_key(key)%(uint64_t)h->bucket_count),*link=&h->buckets[b];
    while(*link!=-1) {
        int i=*link;
        if(strcmp(h->entries[i].key,key)==0) {
            *link=h->entries[i].next;h->entries[i].next=h->free_head;
            h->free_head=i;h->size--;return 1;
        }
        link=&h->entries[i].next;
    } return 0;
}
static int earlier(Event a,Event b) { return a.at<b.at || (a.at==b.at && a.serial<b.serial); }
static void swap(Heap *h,int a,int b) {
    Event e=h->items[a];h->items[a]=h->items[b];h->items[b]=e;
    h->positions[h->items[a].id]=a;h->positions[h->items[b].id]=b;
}
static void up(Heap *h,int i) { while(i>0 && earlier(h->items[i],h->items[(i-1)/2])) { int p=(i-1)/2;swap(h,i,p);i=p; } }
static void down(Heap *h,int i) {
    for(;;) { int a=2*i+1,b=a+1,j=i;
        if(a<h->size && earlier(h->items[a],h->items[j])) j=a;
        if(b<h->size && earlier(h->items[b],h->items[j])) j=b;
        if(j==i) break;swap(h,i,j);i=j;
    }
}
void heap_init(Heap *h) { memset(h,0,sizeof(*h));for(int i=0;i<MAX_TICKETS;i++)h->positions[i]=-1; }
int heap_push(Heap *h,Event e) {
    if(e.id<0||e.id>=MAX_TICKETS||h->size==MAX_TICKETS||h->positions[e.id]!=-1)return 0;
    int i=h->size++;h->items[i]=e;h->positions[e.id]=i;up(h,i);return 1;
}
int heap_remove(Heap *h,int id) {
    if(id<0||id>=MAX_TICKETS||h->positions[id]<0)return 0;
    int i=h->positions[id];h->positions[id]=-1;h->size--;
    if(i<h->size) { h->items[i]=h->items[h->size];h->positions[h->items[i].id]=i;
        if(i>0 && earlier(h->items[i],h->items[(i-1)/2]))up(h,i);else down(h,i); }
    return 1;
}
Event heap_pop(Heap *h) { Event e={0,0,-1};if(h->size){e=h->items[0];heap_remove(h,e.id);}return e; }
int queue_push(Queue *q,int id) { if(q->size==MAX_TICKETS)return 0;q->items[(q->head+q->size++)%MAX_TICKETS]=id;return 1; }
int queue_pop(Queue *q) { if(!q->size)return -1;int id=q->items[q->head];q->head=(q->head+1)%MAX_TICKETS;q->size--;return id; }
int queue_remove(Queue *q,int id) {
    for(int i=0;i<q->size;i++)if(q->items[(q->head+i)%MAX_TICKETS]==id){
        for(int j=i;j<q->size-1;j++)q->items[(q->head+j)%MAX_TICKETS]=q->items[(q->head+j+1)%MAX_TICKETS];q->size--;return 1;
    }
    return 0;
}
int stack_push(Stack *s,int id) { if(s->size==MAX_TICKETS)return 0;s->items[s->size++]=id;return 1; }
int stack_remove(Stack *s,int id) {
    for(int i=0;i<s->size;i++)if(s->items[i]==id){memmove(&s->items[i],&s->items[i+1],(size_t)(s->size-i-1)*sizeof(int));s->size--;return 1;}return 0;
}
void log_append(Log *log,double now,const char *action,const char *code,const char *user,int machine) {
    int i=(log->head+log->size)%30;
    if(log->size==30){i=log->head;log->head=(log->head+1)%30;}else log->size++;
    snprintf(log->lines[i],180,"%.0f,%s,%s,%s,%d",now,action,code,user,machine+1);
}
void log_print(const Log *log) { puts("time,action,code,user,machine");for(int i=0;i<log->size;i++)puts(log->lines[(log->head+i)%30]); }
