#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static int max_int(int a, int b)
{
    return a > b ? a : b;
}

static int height(AVLNode *n)
{
    return n ? n->height : 0;
}

static void update_height(AVLNode *n)
{
    n->height = 1 + max_int(height(n->left), height(n->right));
}

static AVLNode *new_node(int key)
{
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));

    if (!n) {
        perror("malloc");
        exit(1);
    }

    n->key = key;
    n->height = 1;
    n->left = NULL;
    n->right = NULL;

    return n;
}

static AVLNode *rotate_right(AVLNode *y)
{
    AVLNode *x = y->left;
    AVLNode *t2 = x->right;

    x->right = y;
    y->left = t2;

    update_height(y);
    update_height(x);

    return x;
}

static AVLNode *rotate_left(AVLNode *x)
{
    AVLNode *y = x->right;
    AVLNode *t2 = y->left;

    y->left = x;
    x->right = t2;

    update_height(x);
    update_height(y);

    return y;
}

static int balance_factor(AVLNode *n)
{
    return n ? height(n->left) - height(n->right) : 0;
}

static AVLNode *avl_insert(AVLNode *node, int key)
{
    if (!node)
        return new_node(key);

    if (key < node->key)
        node->left = avl_insert(node->left, key);
    else if (key > node->key)
        node->right = avl_insert(node->right, key);
    else
        return node;

    update_height(node);

    int b = balance_factor(node);

    if (b > 1 && key < node->left->key)
        return rotate_right(node);

    if (b < -1 && key > node->right->key)
        return rotate_left(node);

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

/* AVL Search: O(log n) */
static int MySearch(AVLNode *root, int target)
{
    AVLNode *cur = root;

    while (cur) {
        if (target == cur->key)
            return 1;

        if (target < cur->key)
            cur = cur->left;
        else
            cur = cur->right;
    }

    return 0;
}

static int load_values(const char *filename, int *arr, int max_count)
{
    FILE *fp = fopen(filename, "r");
    int count;

    if (!fp) {
        perror(filename);
        exit(1);
    }

    if (fscanf(fp, "%d", &count) != 1 ||
        count < 1 ||
        count > max_count) {

        fprintf(stderr,
                "Invalid file format or data count too large: %s\n",
                filename);

        fclose(fp);
        exit(1);
    }

    for (int i = 0; i < count; ++i) {
        if (fscanf(fp, "%d", &arr[i]) != 1) {

            fprintf(stderr,
                    "Not enough data in file: %s (position %d)\n",
                    filename,
                    i);

            fclose(fp);
            exit(1);
        }
    }

    fclose(fp);

    return count;
}

static void free_tree(AVLNode *root)
{
    if (!root)
        return;

    free_tree(root->left);
    free_tree(root->right);

    free(root);
}

static int contains_linear(const int *arr, int n, int target)
{
    for (int i = 0; i < n; ++i) {
        if (arr[i] == target)
            return 1;
    }

    return 0;
}

static double measure_ms(
    AVLNode *root,
    const int *targets,
    int tcount,
    double *total_ms)
{
    volatile int sink = 0;

    clock_t start = clock();

    for (int r = 0; r < REPEAT; ++r) {
        for (int i = 0; i < tcount; ++i) {
            sink += MySearch(root, targets[i]);
        }
    }

    clock_t end = clock();

    (void)sink;

    double elapsed_ms =
        ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    if (total_ms)
        *total_ms = elapsed_ms;

    return elapsed_ms / ((double)REPEAT * tcount);
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;

    if (x < y)
        return -1;

    if (x > y)
        return 1;

    return 0;
}

static double trimmed_mean(double values[MEASURE_RUNS])
{
    double sorted[MEASURE_RUNS];

    memcpy(sorted, values, sizeof(sorted));

    qsort(
        sorted,
        MEASURE_RUNS,
        sizeof(double),
        cmp_double
    );

    return (sorted[1] + sorted[2] + sorted[3]) / 3.0;
}

static void run_one(
    const char *data_file,
    const char *target_file)
{
    int data[MAXN];
    int targets[MAX_TARGETS];

    int n = load_values(
        data_file,
        data,
        MAXN
    );

    int tcount = load_values(
        target_file,
        targets,
        MAX_TARGETS
    );

    AVLNode *root = NULL;

    for (int i = 0; i < n; ++i)
        root = avl_insert(root, data[i]);

    int found = 0;

    printf("\n");
    printf("==============================================================\n");
    printf("                 AVL SEARCH TIMING TEST\n");
    printf("==============================================================\n");

    printf("Data file   : %s\n", data_file);
    printf("Target file : %s\n", target_file);
    printf("Data size   : %d\n", n);
    printf("Targets     : %d\n", tcount);
    printf("REPEAT      : %d\n", REPEAT);
    printf("Test runs   : %d\n", MEASURE_RUNS);

    printf("\nSearch verification:\n");

    for (int i = 0; i < tcount; ++i) {

        int expected =
            contains_linear(
                data,
                n,
                targets[i]
            );

        int actual =
            MySearch(
                root,
                targets[i]
            );

        printf(
            "Target %d: %d -> %s\n",
            i + 1,
            targets[i],
            actual ? "FOUND" : "NOT FOUND"
        );

        if (expected != actual) {

            printf(
                "ERROR: Target %d search result is incorrect.\n",
                targets[i]
            );

            free_tree(root);
            exit(2);
        }

        found += actual;
    }

    printf(
        "\nVerification result: %d / %d found, %d not found -> PASS\n",
        found,
        tcount,
        tcount - found
    );

    double runs[MEASURE_RUNS];
    double total_runs[MEASURE_RUNS];

    printf("\n");
    printf("==============================================================\n");
    printf("                    TIMING RESULTS\n");
    printf("==============================================================\n");

    printf(
        "Each run = %d repetitions x %d targets = %d searches\n",
        REPEAT,
        tcount,
        REPEAT * tcount
    );

    printf("\n");

    for (int r = 0; r < MEASURE_RUNS; ++r) {

        runs[r] =
            measure_ms(
                root,
                targets,
                tcount,
                &total_runs[r]
            );

        printf(
            "Run %d: Total = %.6f ms | %.9f ms/search\n",
            r + 1,
            total_runs[r],
            runs[r]
        );
    }

    double avg = trimmed_mean(runs);

    double sorted[MEASURE_RUNS];
    memcpy(sorted, runs, sizeof(sorted));

    qsort(
        sorted,
        MEASURE_RUNS,
        sizeof(double),
        cmp_double
    );

    printf("\n");
    printf("--------------------------------------------------------------\n");
    printf("Minimum       : %.9f ms/search\n", sorted[0]);
    printf("Maximum       : %.9f ms/search\n", sorted[4]);
    printf("Average 3 runs: %.9f ms/search\n", avg);
    printf("--------------------------------------------------------------\n");

    printf("\nFINAL RESULT\n");
    printf(
        "n = %d | Average = %.9f ms/search\n",
        n,
        avg
    );

    printf("==============================================================\n");

    free_tree(root);
}

int main(int argc, char **argv)
{
    if (argc != 3) {

        fprintf(
            stderr,
            "Usage: %s <data_file> <target_file>\n",
            argv[0]
        );

        return 1;
    }

    run_one(
        argv[1],
        argv[2]
    );

    return 0;
}