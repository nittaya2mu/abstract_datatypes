/* ============================================================
   avltree.h  --  AVL Tree (BST ที่สมดุลตัวเอง)
   ใช้แสดงรายการจองแบบ "เรียงตามรหัส" และค้นแบบ O(log n)
   ถ้าเวลาไม่พอ ตัดโมดูลนี้ออกได้ โดยไม่กระทบส่วนอื่น
   ============================================================ */
#ifndef AVLTREE_H
#define AVLTREE_H

#include "types.h"

typedef struct AVLNode {
    char            key[MAX_ID_LEN];
    Booking        *value;
    struct AVLNode *left;
    struct AVLNode *right;
    int             height;
} AVLNode;

typedef struct {
    AVLNode *root;
    int      count;
} AVLTree;

AVLTree *avl_create(void);                                        /* O(1)      */
void     avl_destroy(AVLTree *t);                                 /* O(n)      */
int      avl_insert(AVLTree *t, const char *key, Booking *value); /* O(log n)  */
Booking *avl_search(const AVLTree *t, const char *key);           /* O(log n)  */
int      avl_delete(AVLTree *t, const char *key);                 /* O(log n)  */
void     avl_inorder_print(const AVLTree *t);                     /* O(n)      */
int      avl_height(const AVLNode *n);                            /* O(1)      */

#endif /* AVLTREE_H */
