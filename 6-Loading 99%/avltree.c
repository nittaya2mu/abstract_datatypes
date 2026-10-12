/* ============================================================
   avltree.c  --  AVL Tree (Binary Search Tree ที่สมดุลตัวเอง)
   ใช้ booking_id (string) เป็น key เพื่อแสดงรายการจองแบบเรียงลำดับ
   และค้นหาได้เร็ว O(log n) แม้มีรายการจองเป็นพันรายการ

   หลักการ AVL: ทุก node เก็บ "height" ของตัวเอง
   balance factor = height(left) - height(right)
   ถ้า |balance factor| > 1 หลัง insert/delete ต้องหมุนต้นไม้ (rotate)
   เพื่อให้กลับมาสมดุล มี 4 กรณี: LL, RR, LR, RL
   ============================================================ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "avltree.h"
#include "data.h"   /* print_padded: จัดคอลัมน์ภาษาไทยให้ตรง */

/* O(1) */
AVLTree *avl_create(void) {
    AVLTree *t = (AVLTree *)malloc(sizeof(AVLTree));
    if (!t) return NULL;
    t->root = NULL;
    t->count = 0;
    return t;
}

/* O(1) -- คืนค่า height ของ node รองรับ NULL (ถือว่า height = 0) */
int avl_height(const AVLNode *n) {
    return n ? n->height : 0;
}

/* O(1) -- ใช้ภายในไฟล์นี้เท่านั้น หา max ของสอง int */
static int max_int(int a, int b) {
    return (a > b) ? a : b;
}

/* O(1) -- อัปเดต height ของ node จาก height ของลูกทั้งสอง (เรียกหลัง rotate/insert เสมอ) */
static void update_height(AVLNode *n) {
    if (n) n->height = 1 + max_int(avl_height(n->left), avl_height(n->right));
}

/* O(1) -- balance factor: บวก = เอียงซ้าย, ลบ = เอียงขวา */
static int balance_factor(const AVLNode *n) {
    return n ? avl_height(n->left) - avl_height(n->right) : 0;
}

/* O(1) -- หมุนขวา (แก้กรณี Left-Left) 
        y                x
       / \              / \
      x   T3    -->    T1  y
     / \                  / \
    T1  T2               T2 T3 */
static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *t2 = x->right;

    x->right = y;
    y->left = t2;

    update_height(y);   /* ต้องอัปเดต y ก่อน เพราะ y อยู่ล่างกว่าหลังหมุน */
    update_height(x);

    return x;   /* x กลายเป็น root ใหม่ของ subtree นี้ */
}

/* O(1) -- หมุนซ้าย (แก้กรณี Right-Right) 
      x                    y
     / \                  / \
    T1  y      -->        x  T3
       / \               / \
      T2 T3             T1 T2 */
static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *t2 = y->left;

    y->left = x;
    x->right = t2;

    update_height(x);
    update_height(y);

    return y;
}

/* O(log n) -- ฟังก์ชันภายใน ใช้ recursion แทรก key/value แล้ว rebalance กลับขึ้นมา */
static AVLNode *insert_node(AVLNode *node, const char *key, Booking *value, int *inserted) {
    /* กรณีฐาน: ถึงตำแหน่งว่าง สร้าง node ใหม่ตรงนี้ */
    if (node == NULL) {
        AVLNode *new_node = (AVLNode *)malloc(sizeof(AVLNode));
        if (!new_node) { *inserted = 0; return NULL; }   /* หน่วยความจำเต็ม: ไม่ล้ม แค่ไม่เพิ่ม
                                                            ปลอดภัยเพราะกรณีฐานนี้ node เดิมเป็น NULL อยู่แล้ว */
        snprintf(new_node->key, MAX_ID_LEN, "%s", key);
        new_node->value = value;
        new_node->left = NULL;
        new_node->right = NULL;
        new_node->height = 1;
        *inserted = 1;
        return new_node;
    }

    int cmp = strcmp(key, node->key);
    if (cmp < 0) {
        node->left = insert_node(node->left, key, value, inserted);
    } else if (cmp > 0) {
        node->right = insert_node(node->right, key, value, inserted);
    } else {
        /* key ซ้ำ: อัปเดต value แทน ไม่เพิ่ม node ใหม่ */
        node->value = value;
        *inserted = 0;
        return node;
    }

    update_height(node);
    int bf = balance_factor(node);

    /* Left-Left: เอียงซ้ายมาก และ key ใหม่อยู่ลูกซ้ายของลูกซ้าย */
    if (bf > 1 && strcmp(key, node->left->key) < 0) {
        return rotate_right(node);
    }
    /* Right-Right: เอียงขวามาก และ key ใหม่อยู่ลูกขวาของลูกขวา */
    if (bf < -1 && strcmp(key, node->right->key) > 0) {
        return rotate_left(node);
    }
    /* Left-Right: เอียงซ้าย แต่ key ใหม่อยู่ลูกขวาของลูกซ้าย -> หมุนซ้ายที่ลูกก่อน แล้วหมุนขวาที่ node */
    if (bf > 1 && strcmp(key, node->left->key) > 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    /* Right-Left: เอียงขวา แต่ key ใหม่อยู่ลูกซ้ายของลูกขวา -> หมุนขวาที่ลูกก่อน แล้วหมุนซ้ายที่ node */
    if (bf < -1 && strcmp(key, node->right->key) < 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;   /* สมดุลอยู่แล้ว ไม่ต้องหมุน */
}

/* O(log n) */
int avl_insert(AVLTree *t, const char *key, Booking *value) {
    if (!t || !key) return 0;
    int inserted = 0;
    t->root = insert_node(t->root, key, value, &inserted);
    if (inserted) t->count++;
    return 1;
}

/* O(log n) -- ไล่ตาม BST property ธรรมดา (ซ้ายเล็กกว่า ขวามากกว่า) */
Booking *avl_search(const AVLTree *t, const char *key) {
    if (!t || !key) return NULL;
    AVLNode *cur = t->root;
    while (cur != NULL) {
        int cmp = strcmp(key, cur->key);
        if (cmp == 0) return cur->value;
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

/* O(log n) -- หา node ที่ key น้อยที่สุดใน subtree (ใช้หา successor ตอนลบ) */
static AVLNode *find_min_node(AVLNode *node) {
    AVLNode *cur = node;
    while (cur->left != NULL) cur = cur->left;
    return cur;
}

/* O(log n) -- ฟังก์ชันภายใน ใช้ recursion ลบ node แล้ว rebalance กลับขึ้นมา */
static AVLNode *delete_node(AVLNode *node, const char *key, int *deleted) {
    if (node == NULL) return NULL;

    int cmp = strcmp(key, node->key);
    if (cmp < 0) {
        node->left = delete_node(node->left, key, deleted);
    } else if (cmp > 0) {
        node->right = delete_node(node->right, key, deleted);
    } else {
        /* เจอ node ที่จะลบแล้ว มี 3 กรณี */
        *deleted = 1;
        if (node->left == NULL || node->right == NULL) {
            /* กรณีมีลูกไม่เกิน 1 ตัว: แทนที่ด้วยลูกที่มี (หรือ NULL ถ้าไม่มีเลย) */
            AVLNode *child = node->left ? node->left : node->right;
            free(node);
            return child;
        } else {
            /* กรณีมีลูก 2 ตัว: หา successor (ค่าน้อยสุดใน subtree ขวา) มาแทน
               แล้วลบ successor ตัวเดิมออกจาก subtree ขวาแทน */
            AVLNode *successor = find_min_node(node->right);
            snprintf(node->key, MAX_ID_LEN, "%s", successor->key);
            node->value = successor->value;
            int dummy = 0;
            node->right = delete_node(node->right, successor->key, &dummy);
        }
    }

    update_height(node);
    int bf = balance_factor(node);

    /* Rebalance 4 กรณีเหมือนตอน insert แต่เช็คจาก balance factor ของลูกแทน */
    if (bf > 1 && balance_factor(node->left) >= 0) {
        return rotate_right(node);
    }
    if (bf > 1 && balance_factor(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (bf < -1 && balance_factor(node->right) <= 0) {
        return rotate_left(node);
    }
    if (bf < -1 && balance_factor(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

/* O(log n) */
int avl_delete(AVLTree *t, const char *key) {
    if (!t || !key) return 0;
    int deleted = 0;
    t->root = delete_node(t->root, key, &deleted);
    if (deleted) t->count--;
    return deleted;
}

/* O(n) -- Inorder traversal ของ BST จะได้ key เรียงจากน้อยไปมากเสมอ
   (คุณสมบัตินี้เองที่ทำให้เมนู "แสดงรายการจองเรียงตามรหัส" ใช้ AVL ได้พอดี) */
static void inorder_recursive(const AVLNode *node) {
    if (node == NULL) return;
    inorder_recursive(node->left);

    const Booking *b = node->value;
    const char *status_str =
        (b->status == STATUS_CONFIRMED)  ? "ยืนยันแล้ว" :
        (b->status == STATUS_WAITLISTED) ? "รอคิว"      : "ยกเลิกแล้ว";
    printf("  ");
    print_padded(node->key, 10);
    print_padded(b->passenger_name, 20);
    print_padded(status_str, 12);
    printf("\n");

    inorder_recursive(node->right);
}

/* O(n) */
void avl_inorder_print(const AVLTree *t) {
    if (!t || t->root == NULL) {
        printf("  (ยังไม่มีการจอง)\n");
        return;
    }
    printf("--- รายการจองทั้งหมด เรียงตามรหัส (AVL Tree, %d รายการ) ---\n", t->count);
    printf("  ");
    print_padded("รหัส", 10);
    print_padded("ผู้โดยสาร", 20);
    print_padded("สถานะ", 12);
    printf("\n");
    inorder_recursive(t->root);
}

/* O(n) -- ฟังก์ชันภายใน ใช้ postorder เพื่อ free ลูกก่อนแล้วค่อย free ตัวเอง */
static void destroy_recursive(AVLNode *node) {
    if (node == NULL) return;
    destroy_recursive(node->left);
    destroy_recursive(node->right);
    free(node);
}

/* O(n) */
void avl_destroy(AVLTree *t) {
    if (!t) return;
    destroy_recursive(t->root);
    free(t);
}
