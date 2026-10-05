/*
 * timing_avl.c - AVL Search timing program for 01204212
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -Werror timing_avl.c -o timing_avl
 *
 * Run:
 *   ./timing_avl data_1000.txt targets_1000.txt
 *
 * NOTE: The instructor's timing activity requires 5 runs per data size,
 * discarding the minimum and maximum, then averaging the remaining 3 runs.
 * This program performs that measurement directly.
 *
 * The data loading and AVL construction are NOT included in the timed section.
 * Only AVL search is repeated REPEAT times.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAXN 100000
#define MAX_TARGETS 100
#define REPEAT 1000
#define MEASURE_RUNS 5

typedef struct AVLNode {
    int key;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

static int max_int(int a, int b) { return a > b ? a : b; }
static int height(AVLNode *n) { return n ? n->height : 0; }
static void update_height(AVLNode *n) {
    n->height = 1 + max_int(height(n->left), height(n->right));
}

static AVLNode *new_node(int key) {
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));
    if (!n) { perror("malloc"); exit(1); }
    n->key = key; n->height = 1; n->left = n->right = NULL;
    return n;
}

static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *t2 = x->right;
    x->right = y; y->left = t2;
    update_height(y); update_height(x);
    return x;
}

static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *t2 = y->left;
    y->left = x; x->right = t2;
    update_height(x); update_height(y);
    return y;
}

static int balance_factor(AVLNode *n) {
    return n ? height(n->left) - height(n->right) : 0;
}

static AVLNode *avl_insert(AVLNode *node, int key) {
    if (!node) return new_node(key);
    if (key < node->key) node->left = avl_insert(node->left, key);
    else if (key > node->key) node->right = avl_insert(node->right, key);
    else return node;

    update_height(node);
    int b = balance_factor(node);
    if (b > 1 && key < node->left->key) return rotate_right(node);
    if (b < -1 && key > node->right->key) return rotate_left(node);
    if (b > 1 && key > node->left->key) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (b < -1 && key < node->right->key) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    return node;
}

/* O(log n) average/worst-case for an AVL tree. */
static int MySearch(AVLNode *root, int target) {
    AVLNode *cur = root;
    while (cur) {
        if (target == cur->key) return 1;
        cur = (target < cur->key) ? cur->left : cur->right;
    }
    return 0;
}

static int load_values(const char *filename, int *arr, int max_count) {
    FILE *fp = fopen(filename, "r");
    int count;
    if (!fp) { perror(filename); exit(1); }
    if (fscanf(fp, "%d", &count) != 1 || count < 1 || count > max_count) {
        fprintf(stderr, "รูปแบบไฟล์ไม่ถูกต้องหรือจำนวนข้อมูลเกิน %d: %s\n", max_count, filename);
        fclose(fp); exit(1);
    }
    for (int i = 0; i < count; ++i) {
        if (fscanf(fp, "%d", &arr[i]) != 1) {
            fprintf(stderr, "อ่านข้อมูลไม่ครบ: %s (ตำแหน่ง %d)\n", filename, i);
            fclose(fp); exit(1);
        }
    }
    fclose(fp);
    return count;
}

static void free_tree(AVLNode *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

static int contains_linear(const int *arr, int n, int target) {
    for (int i = 0; i < n; ++i) if (arr[i] == target) return 1;
    return 0;
}

static double measure_ms(AVLNode *root, const int *targets, int tcount) {
    volatile int sink = 0;
    clock_t start = clock();
    for (int r = 0; r < REPEAT; ++r) {
        for (int i = 0; i < tcount; ++i) {
            sink += MySearch(root, targets[i]);
        }
    }
    clock_t end = clock();
    (void)sink;
    return ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0 / (REPEAT * tcount);
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double trimmed_mean(double values[MEASURE_RUNS]) {
    double sorted[MEASURE_RUNS];
    memcpy(sorted, values, sizeof(sorted));
    qsort(sorted, MEASURE_RUNS, sizeof(double), cmp_double);
    return (sorted[1] + sorted[2] + sorted[3]) / 3.0;
}

static void run_one(const char *data_file, const char *target_file) {
    int data[MAXN], targets[MAX_TARGETS];
    int n = load_values(data_file, data, MAXN);
    int tcount = load_values(target_file, targets, MAX_TARGETS);
    if (tcount != 10) fprintf(stderr, "คำเตือน: targets มี %d ค่า (คาดหวัง 10)\n", tcount);

    AVLNode *root = NULL;
    for (int i = 0; i < n; ++i) root = avl_insert(root, data[i]);

    int found = 0;
    for (int i = 0; i < tcount; ++i) {
        int expected = contains_linear(data, n, targets[i]);
        int actual = MySearch(root, targets[i]);
        if (expected != actual) {
            fprintf(stderr, "FAIL: target %d คาดหวัง %s แต่ MySearch ได้ %s\n",
                    targets[i], expected ? "พบ" : "ไม่พบ", actual ? "พบ" : "ไม่พบ");
            free_tree(root); exit(2);
        }
        found += actual;
    }

    double runs[MEASURE_RUNS];
    for (int r = 0; r < MEASURE_RUNS; ++r) {
        runs[r] = measure_ms(root, targets, tcount);
    }
    double avg = trimmed_mean(runs);

    printf("\n==============================================================\n");
    printf("ข้อมูล: %s\n", data_file);
    printf("n = %d | targets = %d | REPEAT = %d\n", n, tcount, REPEAT);
    printf("ตรวจผล: พบ %d / %d (ไม่พบ %d) -> PASS\n", found, tcount, tcount - found);
    printf("วัด 5 ครั้ง (ms/search)\n");
    for (int r = 0; r < MEASURE_RUNS; ++r) printf("  ครั้งที่ %d: %.6f ms\n", r + 1, runs[r]);
    printf("ตัดค่าสูงสุด+ต่ำสุด -> ค่าเฉลี่ย 3 ค่า: %.6f ms\n", avg);
    printf("==============================================================\n");

    free_tree(root);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "วิธีใช้: %s <data_*.txt> <targets_*.txt>\n", argv[0]);
        return 1;
    }
    run_one(argv[1], argv[2]);
    return 0;
}
