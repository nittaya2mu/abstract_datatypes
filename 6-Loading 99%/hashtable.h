/* ============================================================
   hashtable.h  --  Hash Table (separate chaining)
   ค้นรายการจองจาก Booking ID ให้ได้ O(1) โดยเฉลี่ย
   ============================================================ */
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "types.h"

/* จำนวนบักเก็ต: ใช้จำนวนเฉพาะ 100003 ซึ่งมากกว่าข้อมูลทดสอบสูงสุด (100,000 ตัว)
   เพื่อให้ load factor = n / HASH_SIZE ไม่เกิน 1 โซ่ในแต่ละช่องจึงสั้น
   ค้นหาได้ O(1) โดยเฉลี่ยจริงตามที่โจทย์กำหนด
   (เดิมใช้ 211 ช่อง ที่ n = 100,000 โซ่ยาวเฉลี่ย 474 ตัว ค้นหาเสื่อมเป็น O(n))
   แลกกับหน่วยความจำของตารางประมาณ 800 KB ซึ่งจองบน heap ผ่าน ht_create() */
#define HASH_SIZE 100003

typedef struct HashEntry {
    char              key[MAX_ID_LEN];
    Booking          *value;
    struct HashEntry *next;
} HashEntry;

typedef struct {
    HashEntry *buckets[HASH_SIZE];
    int        count;
} HashTable;

HashTable   *ht_create(void);                                       /* O(1)          */
void         ht_destroy(HashTable *ht);                             /* O(n)          */
unsigned int ht_hash(const char *key);                              /* O(len)        */
int          ht_insert(HashTable *ht, const char *key, Booking *v); /* O(1) เฉลี่ย    */
Booking     *ht_search(const HashTable *ht, const char *key);       /* O(1) เฉลี่ย    */
int          ht_delete(HashTable *ht, const char *key);             /* O(1) เฉลี่ย    */
void         ht_print_stats(const HashTable *ht);                   /* ใช้โชว์อาจารย์ */

#endif /* HASHTABLE_H */
