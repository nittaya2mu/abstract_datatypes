#ifndef SMART_ADT_H
#define SMART_ADT_H
#include <stddef.h>
#include <stdint.h>
#define KEY_BYTES 80
typedef struct { char key[KEY_BYTES]; int value, next; } HashEntry;
typedef struct { int *buckets, capacity, bucket_count, free_head, size; HashEntry *entries; } Hash;
int hash_init(Hash *h, int capacity);
void hash_free(Hash *h);
int hash_get(const Hash *h, const char *key);
int hash_put(Hash *h, const char *key, int value);
int hash_remove(Hash *h, const char *key);
#define MAX_TICKETS 50
typedef struct { double at; unsigned serial; int id; } Event;
typedef struct { Event items[MAX_TICKETS]; int positions[MAX_TICKETS], size; } Heap;
void heap_init(Heap *h);
int heap_push(Heap *h, Event e);
int heap_remove(Heap *h, int id);
Event heap_pop(Heap *h);
typedef struct { int items[MAX_TICKETS], head, size; } Queue;
int queue_push(Queue *q, int id);
int queue_pop(Queue *q);
int queue_remove(Queue *q, int id);
typedef struct { int items[MAX_TICKETS], size; } Stack;
int stack_push(Stack *s, int id);
int stack_remove(Stack *s, int id);
typedef struct { char lines[30][180]; int head, size; } Log;
void log_append(Log *log, double now, const char *action, const char *code, const char *user, int machine);
void log_print(const Log *log);
int benchmark_main(int argc, char **argv);
#endif
