#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXN   100000
#define REPEAT 1000

int data[MAXN];
int sorted_data[MAXN]; 
int n;

int LoadData(const char *filename, int arr[])
{
    FILE *fp = fopen(filename, "r");
    int  count, i;

    if (fp == NULL) {
        printf("Cannot open file %s\n", filename);
        exit(1);
    }
    fscanf(fp, "%d", &count);
    for (i = 0; i < count; i++)
        fscanf(fp, "%d", &arr[i]);
    fclose(fp);
    return count;
}
int CompareInt(const void *a, const void *b)
{
    return (*(const int *)a) - (*(const int *)b);
}

int SequentialSearch(int arr[], int size, int target)
{
    int i;
    for (i = 0; i < size; i++)
        if (arr[i] == target)
            return i;
    return -1;
}

int BinarySearch(int arr[], int size, int target)
{
    int first = 0, last = size - 1, mid;

    while (first <= last) {
        mid = (first + last) / 2;
        if (target > arr[mid])      first = mid + 1;
        else if (target < arr[mid]) last  = mid - 1;
        else                        return mid;
    }
    return -1;
}


int MySearch(int arr[], int size, int target)
{
    int i;
    for (i = 0; i < size; i++) {
        if (arr[i] == target) return i;
    }
    return -1;
}

double MeasureMillisec(int (*SearchFunc)(int[], int, int),
                       int arr[], int size, int targets[], int tcount)
{
    clock_t start, end;
    double  total_sec;
    int     r, i, result = 0;

    start = clock();   
    for (r = 0; r < REPEAT; r++)
        for (i = 0; i < tcount; i++)
            result += SearchFunc(arr, size, targets[i]);
    end = clock();  
    if (result == -99999999) printf(" "); 
    total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    return total_sec * 1000.0 / (REPEAT * tcount); 
}

int main(int argc, char *argv[])
{
    int    targets[100], tcount, i;
    double ms_seq, ms_bin, ms_my;

    if (argc < 3) {
        printf("Usage: %s <data file> <targets file>\n", argv[0]);
        printf("Example: %s data_1000.txt targets_1000.txt\n", argv[0]);
        return 1;
    }

    n      = LoadData(argv[1], data);
    tcount = LoadData(argv[2], targets);

    for (i = 0; i < n; i++) sorted_data[i] = data[i];
    qsort(sorted_data, n, sizeof(int), CompareInt);

    printf("=====================================================\n");
    printf(" Data file         : %s\n", argv[1]);
    printf(" Data size n       : %d\n", n);
    printf(" Number of targets : %d\n", tcount);
    printf(" REPEAT            : %d rounds per target\n", REPEAT);
    printf("=====================================================\n");

    printf("\n[ Correctness check ]\n");
    printf(" %-12s %-12s %-12s\n", "Target", "Sequential", "Binary");
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        printf(" %-12d %-12s %-12s\n", targets[i],
               (a >= 0) ? "Found" : "Not found",
               (b >= 0) ? "Found" : "Not found");
    }

    /* ---- Timing ---- */
    ms_seq = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
    ms_bin = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
    ms_my  = MeasureMillisec(MySearch,         data,        n, targets, tcount);

    printf("\n[ Average time per search ]\n");
    printf(" Sequential Search : %.6f ms\n", ms_seq);
    printf(" Binary Search     : %.6f ms\n", ms_bin);
    printf(" MySearch (group)  : %.6f ms\n", ms_my);
    printf("\nRecord the MySearch value in the table. Repeat until 5 runs are done.\n");

    return 0;
}