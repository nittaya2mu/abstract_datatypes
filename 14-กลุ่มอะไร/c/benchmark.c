/* Adapted from the instructor timing_template.c. Preparation is outside clock(). */
#include "adt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#define REPEAT 1000000
#define MAXN 100000
static volatile long long result_sink;
static int positive_integer(FILE *f,int *value) {
    char token[64],*end;long number;int c,used=0,overlong=0;
    do{c=fgetc(f);}while(c!=EOF&&isspace((unsigned char)c));
    if(c==EOF)return 0;
    do{
        if(used<(int)sizeof(token)-1)token[used++]=(char)c;else overlong=1;
        c=fgetc(f);
    }while(c!=EOF&&!isspace((unsigned char)c));
    token[used]=0;if(overlong)return 0;
    errno=0;number=strtol(token,&end,10);
    if(errno==ERANGE||*end||number<1||number>INT_MAX)return 0;
    *value=(int)number;return 1;
}
static int load(const char *path,int **arr,int maximum) {
    FILE *f=fopen(path,"r");if(!f){fprintf(stderr,"Cannot open %s\n",path);return -1;}
    int n; if(!positive_integer(f,&n)||n>maximum){fclose(f);return -1;}
    *arr=malloc((size_t)n*sizeof(int));if(!*arr){fclose(f);return -1;}
    for(int i=0;i<n;i++)if(!positive_integer(f,&(*arr)[i])){free(*arr);*arr=NULL;fclose(f);return -1;}
    char extra[2];int trailing=fscanf(f,"%1s",extra);fclose(f);
    if(trailing==1){free(*arr);*arr=NULL;return -1;}return n;
}
static int sequential(const int *data,int n,int target) { for(int i=0;i<n;i++)if(data[i]==target)return i;return -1; }
static int MySearch(const Hash *h,const char *key) { return hash_get(h,key); }
static int cmp_double(const void *a,const void *b) { double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y); }
int benchmark_main(int argc,char **argv) {
    /* --benchmark DATA_DIR [OUTPUT.csv]; fixed n and all ten required targets. */
    if(argc<3||argc>4){fprintf(stderr,"Usage: %s --benchmark DATA_DIR [OUTPUT.csv]\n",argv[0]);return 2;}
    FILE *csv=NULL;if(argc==4){csv=fopen(argv[3],"w");if(!csv){perror("Output CSV");return 2;}
        fputs("n,repeat,targets,found,missing,run1_ms,run2_ms,run3_ms,run4_ms,run5_ms,trimmed_mean_ms,ratio,k\n",csv);}
    const int sizes[3]={1000,10000,100000};double previous=0;
    puts("Hash Table (FNV-1a + separate chaining); clock(); REPEAT=1000000; 10 targets/run");
    puts("LOCAL measurement only. Rerun on classroom computer for official results.");
    for(int s=0;s<3;s++) {
        char path[1024];int *data=NULL,*targets=NULL;
        snprintf(path,sizeof(path),"%s/data_%d.txt",argv[2],sizes[s]);int n=load(path,&data,MAXN);
        snprintf(path,sizeof(path),"%s/targets_%d.txt",argv[2],sizes[s]);int t=load(path,&targets,10);
        if(n!=sizes[s]||t!=10){fprintf(stderr,"Invalid input count/format for n=%d\n",sizes[s]);free(data);free(targets);if(csv)fclose(csv);return 2;}
        Hash h;if(!hash_init(&h,n)){free(data);free(targets);if(csv)fclose(csv);return 2;}
        char key[KEY_BYTES],keys[10][KEY_BYTES];int ok=1,found=0;
        for(int i=0;i<n;i++){snprintf(key,sizeof(key),"%d",data[i]);if(hash_get(&h,key)>=0||!hash_put(&h,key,i)){ok=0;break;}}
        printf("\nVALIDATION n=%d (original unsorted indices, zero-based)\n",n);
        for(int i=0;i<t;i++){
            snprintf(keys[i],KEY_BYTES,"%d",targets[i]);int actual=MySearch(&h,keys[i]),expected=sequential(data,n,targets[i]);
            printf("target=%d index=%d expected=%d %s\n",targets[i],actual,expected,actual==expected?"PASS":"FAIL");
            if(actual!=expected)ok=0;if(actual>=0)found++;
        }
        if(found!=7||!ok){fprintf(stderr,"Validation failed; timing aborted.\n");hash_free(&h);free(data);free(targets);if(csv)fclose(csv);return 1;}
        double ms[5],sorted[5];
        for(int run=0;run<5;run++){
            clock_t start=clock();
            for(int r=0;r<REPEAT;r++)for(int i=0;i<t;i++)result_sink=MySearch(&h,keys[i]);
            clock_t end=clock();
            if(start==(clock_t)-1||end==(clock_t)-1||end<start){fprintf(stderr,"clock() failed\n");hash_free(&h);free(data);free(targets);if(csv)fclose(csv);return 1;}
            double total=(double)(end-start)/CLOCKS_PER_SEC;
            ms[run]=total*1000.0/((double)REPEAT*t);sorted[run]=ms[run];
            printf("RUN %d total_sec=%.6f per_search_ms=%.9f\n",run+1,total,ms[run]);
        }
        qsort(sorted,5,sizeof(double),cmp_double);double mean=(sorted[1]+sorted[2]+sorted[3])/3.0;
        if(mean<=0){fprintf(stderr,"Clock resolution insufficient: zero mean; do not infer k.\n");hash_free(&h);free(data);free(targets);if(csv)fclose(csv);return 1;}
        double ratio=previous>0?mean/previous:0,k=previous>0?log(ratio)/log(10.0):0;
        printf("SUMMARY n=%d found=%d missing=%d trimmed_mean_ms=%.9f",n,found,t-found,mean);
        if(previous>0)printf(" ratio=%.6f k=%.6f",ratio,k);puts("");
        if(csv){fprintf(csv,"%d,%d,%d,%d,%d",n,REPEAT,t,found,t-found);for(int i=0;i<5;i++)fprintf(csv,",%.9f",ms[i]);fprintf(csv,",%.9f,",mean);if(previous>0)fprintf(csv,"%.6f,%.6f",ratio,k);else fputs(",",csv);fputs("\n",csv);}
        previous=mean;hash_free(&h);free(data);free(targets);
    }
    if(csv&&fclose(csv)!=0){perror("CSV close");return 2;}
    puts("Expected lookup O(1), worst O(n); preparation/space O(n). Near-zero k alone cannot distinguish O(1) from O(log n).");
    return 0;
}
