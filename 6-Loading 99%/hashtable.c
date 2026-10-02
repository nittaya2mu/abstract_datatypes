/* ============================================================
   hashtable.c  --  Hash Table + separate chaining
   ค้นรายการจองจาก Booking ID ได้ O(1) โดยเฉลี่ย
   ใช้ djb2 hash ซึ่งกระจาย key ที่เป็นสตริงได้ดี
   ============================================================ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hashtable.h"

/* สร้างตารางเปล่า ทุกบักเก็ตเป็น NULL -- O(HASH_SIZE) */
HashTable *ht_create(void) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (!ht) return NULL;
    { int i; for (i = 0; i < HASH_SIZE; i++) ht->buckets[i] = NULL; }
    ht->count = 0;
    return ht;
}

/* คืนหน่วยความจำทุก entry -- O(n)
   ไม่ free ตัว Booking เพราะเจ้าของคือ booking.c */
void ht_destroy(HashTable *ht) {
    if (!ht) return;
    { int i; for (i = 0; i < HASH_SIZE; i++) {
        HashEntry *e = ht->buckets[i];
        while (e) {
            HashEntry *next = e->next;
            free(e);
            e = next;
        }
    } }
    free(ht);
}

/* djb2 hash -- O(len) ของความยาว key
   สูตร: h = h * 33 + c  เริ่มที่ 5381 */
unsigned int ht_hash(const char *key) {
    unsigned long h = 5381;
    if (!key) return 0u;
    while (*key) {
        h = ((h << 5) + h) + (unsigned char)(*key);
        key++;
    }
    return (unsigned int)(h % HASH_SIZE);
}

/* เพิ่มหรืออัปเดตข้อมูล -- O(1) เฉลี่ย, O(n) กรณีแย่สุดที่ชนกันหมด */
int ht_insert(HashTable *ht, const char *key, Booking *value) {
    unsigned int idx;
    HashEntry *e;
    if (!ht || !key) return 0;

    idx = ht_hash(key);
    for (e = ht->buckets[idx]; e; e = e->next)
        if (strcmp(e->key, key) == 0) { e->value = value; return 1; }  /* มีอยู่แล้ว ทับค่าเดิม */

    e = (HashEntry *)malloc(sizeof(HashEntry));
    if (!e) return 0;
    strncpy(e->key, key, MAX_ID_LEN - 1);
    e->key[MAX_ID_LEN - 1] = '\0';
    e->value = value;
    e->next  = ht->buckets[idx];    /* แทรกหัวบักเก็ต O(1) */
    ht->buckets[idx] = e;
    ht->count++;
    return 1;
}

/* ค้นหาจาก key -- O(1) เฉลี่ย */
Booking *ht_search(const HashTable *ht, const char *key) {
    unsigned int idx;
    if (!ht || !key) return NULL;
    idx = ht_hash(key);
    { HashEntry *e; for (e = ht->buckets[idx]; e; e = e->next)
        if (strcmp(e->key, key) == 0) return e->value; }
    return NULL;
}

/* ลบออกจากตาราง -- O(1) เฉลี่ย */
int ht_delete(HashTable *ht, const char *key) {
    unsigned int idx;
    HashEntry *e, *prev = NULL;
    if (!ht || !key) return 0;

    idx = ht_hash(key);
    for (e = ht->buckets[idx]; e; prev = e, e = e->next) {
        if (strcmp(e->key, key) == 0) {
            if (prev) prev->next        = e->next;
            else      ht->buckets[idx]  = e->next;
            free(e);
            ht->count--;
            return 1;
        }
    }
    return 0;
}

/* สถิติการกระจายตัว -- O(HASH_SIZE + n) ใช้ตอนนำเสนอให้อาจารย์ดู */
void ht_print_stats(const HashTable *ht) {
    int used = 0, longest = 0;
    if (!ht) { printf(">> ตารางยังไม่ถูกสร้าง\n"); return; }

    { int i; for (i = 0; i < HASH_SIZE; i++) {
        int len = 0;
        { HashEntry *e; for (e = ht->buckets[i]; e; e = e->next) len++; }
        if (len > 0) used++;
        if (len > longest) longest = len;
    } }
    printf("\n--- สถิติ Hash Table ---\n");
    printf("  ขนาดตาราง        : %d บักเก็ต\n", HASH_SIZE);
    printf("  จำนวนข้อมูล       : %d รายการ\n", ht->count);
    printf("  บักเก็ตที่ถูกใช้    : %d (%.1f%%)\n",
           used, HASH_SIZE ? (100.0 * used / HASH_SIZE) : 0.0);
    printf("  โซ่ที่ยาวที่สุด     : %d  (ยิ่งสั้นยิ่งค้นเร็ว)\n", longest);
    printf("  Load factor      : %.3f\n",
           HASH_SIZE ? ((double)ht->count / HASH_SIZE) : 0.0);
}
