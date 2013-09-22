#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>

#include "tl.h"
#include "x86_sync_defns.h"



//-----------------------------

intptr_t * dboard;
int board_size=100;
volatile int can_stop=0;
//-----------------------------

void board_init(int size, int val) {
    dboard=(intptr_t*)malloc(sizeof(intptr_t)*size);
    int i=0;
    for(i=0;i<size;i++) {
        dboard[i]=val;
    }

}

void * thr_reader(void * args) {
    Thread * Self ;
    Self = TxNewThread () ;

    int reader_successfull_commits=0;
    int reader_failed_commits=0;

    _u64 t_ld=0, ts_ld;
    _u64 t_start=0, ts_start;
    _u64 t_cmt=0, ts_cmt;
    _u64 num_lds=0;
    _u64 num_tx=0;

    while(!can_stop) {
        ts_start=RDTICK();
        TxStart (Self, 0) ;
        t_start+=(RDTICK()-ts_start);

num_tx++;

        int val1, val2;

        ts_ld=RDTICK();
        val1 = TXLDV(dboard[0]);
        t_ld+=(RDTICK()-ts_ld);
        num_lds++;

        int i;
        for(i=1;i<board_size;i++) {
            ts_ld=RDTICK();
            val2=TXLDV(dboard[i]);
            t_ld+=(RDTICK()-ts_ld);
            num_lds++;
            assert(!TxValid(Self) || val1==val2);
        }

        int res;
        ts_cmt=RDTICK();
        res=TxCommit(Self);
        t_cmt+=(RDTICK()-ts_cmt);
        if (res) {
            reader_successfull_commits++;
        } else {
            reader_failed_commits++;
        }
        //sched_yield();
    }

    printf("t_ld=%llu, num_lds=%llu, tper_ld=%lf\n",t_ld,num_lds,(double)t_ld/num_lds);
    printf("t_start=%llu, num_tx=%llu, tper_start=%lf\n",t_start,num_tx,(double)t_start/num_tx);
    printf("t_cmt=%llu, num_tx=%llu, tper_cmt=%lf\n",t_cmt,num_tx,(double)t_cmt/num_tx);


    TxEndThread(Self);
    printf("%d: reader_successfull_commits=%d\n", getpid(), reader_successfull_commits);
    printf("%d: reader_failed_commits=%d\n", getpid(), reader_failed_commits);

    return NULL;
}


void * thr_writer(void * args) {
    Thread * Self ;
    Self = TxNewThread () ;
    int seed = ((long) &seed) + RDTICK() ;

    int writer_successfull_commits=0;
    int writer_failed_commits=0;

    seed |=1 ;

    _u64 t_st=0, ts_st;
    _u64 t_start=0, ts_start;
    _u64 t_cmt=0, ts_cmt;
    _u64 num_sts=0;
    _u64 num_tx=0;

    while(!can_stop) {
        intptr_t val = rand() ;
        ts_start=RDTICK();
        TxStart (Self, 0) ;
        t_start+=(RDTICK()-ts_start);
        //printf("val=%d\n", val);

num_tx++;

        int i;
        for(i=0;i<board_size;i++) {
            //            TXLDV(dboard[i]);
            ts_st=RDTICK();
            TXSTV(dboard[i],val);
            t_st+=(RDTICK()-ts_st);
            num_sts++;
        }


        int res;
        ts_cmt=RDTICK();
        res=TxCommit(Self);
        t_cmt+=(RDTICK()-ts_cmt);
        if (res) {
            writer_successfull_commits++;
        } else {
            writer_failed_commits++;
        }
        //sched_yield();
    }
    printf("t_st=%llu, num_sts=%llu, tper_st=%lf\n",t_st,num_sts,(double)t_st/num_sts);
    printf("t_start=%llu, num_tx=%llu, tper_start=%lf\n",t_start,num_tx,(double)t_start/num_tx);
    printf("t_cmt=%llu, num_tx=%llu, tper_cmt=%lf\n",t_cmt,num_tx,(double)t_cmt/num_tx);

    printf("%d: writer_successfull_commits=%d\n", getpid(), writer_successfull_commits);
    printf("%d: writer_failed_commits=%d\n", getpid(), writer_failed_commits);
    TxEndThread(Self);


    return NULL;
}

//-----------------------------

int main(int argc, char *argv[]) {
    unsigned int sleep_time=10;
    int num_writer_threads=3;
    int num_reader_threads=1;


    if(argc!=5) {
        printf("syntax: sleep_time num_reader_thr num_writer_thr board_size\n");
        exit(1);
    }
    sleep_time = atoi(argv[1]);
    num_reader_threads = atoi(argv[2]);
    num_writer_threads = atoi(argv[3]);
    board_size = atoi(argv[4]);

    board_init(board_size,0);

    //TxOnce();

    pthread_t *w_thr_a=(pthread_t*)malloc(sizeof(pthread_t)*num_writer_threads);
    pthread_t *r_thr_a=(pthread_t*)malloc(sizeof(pthread_t)*num_reader_threads);

    int i=0;
    for(i=0; i<num_writer_threads;i++) {
        pthread_create(&(w_thr_a[i]),NULL,thr_writer,NULL);
    }

    for(i=0; i<num_reader_threads;i++) {
        pthread_create(&(r_thr_a[i]),NULL,thr_reader,NULL);
    }

    sleep(sleep_time);
    can_stop=1;

    for(i=0; i<num_reader_threads;i++) {
        pthread_join(r_thr_a[i],NULL);
    }

    for(i=0; i<num_writer_threads;i++) {
        pthread_join(w_thr_a[i],NULL);
    }

    //TxShutdown() ;

    free(w_thr_a);
    free(r_thr_a);
    w_thr_a=NULL;
    r_thr_a=NULL;
    free (dboard);

    return 0;
}
