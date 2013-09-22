#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>

#include "x86_sync_defns.h"
//#include "tl.h"
#include "tl_cmt_word_64.h"
//#include "tl_enc_word_64.h"


// --------------------------------------------


#define TLRand(sa) MarsagliaXOR(sa)

//#define IS_TX_VALID(x) TxValid(x)
#define IS_TX_VALID(x) ({ if(!TxValid(x)) assert(0); 1;})
//#define TXCOMMIT(x) { if(!TxCommit(x)) assert(0); }
#define TXCOMMIT TxCommit

// --------------------------------------------
volatile int can_stop=0;
int seed ;
int sleep_time;

intptr_t * dboard;
int board_size=100;

static int * qwe_thr=NULL;
static int * * ptr_qwe_thr=NULL;

typedef struct _obj_t {
    int a;
    int b;
    int c;
}
obj_t;

#ifdef OBJ_STM
static obj_t obj1;
#endif

// --------------------------------------------
void board_init(int size) {
    dboard=(intptr_t*)malloc(sizeof(intptr_t)*size);
    int i=0;
    for(i=0;i<size;i++) {
        dboard[i]=0;
    }
}

void board_set(Thread * Self, int size, int val) {
    int i=0;
    for(i=0;i<size;i++) {
        TXSTV(dboard[i],val);
    }
}

int board_check(Thread * Self, int size, int val) {
    int i=0;
    for(i=0;i<size;i++) {
        int v=TXLDV(dboard[i]);
        //printf("v=%d val=%d\n",v,val);
        assert(v==val);
        if(v!=val)
            return 0;
    }
    return 1;
}



// --------------------------------------------

// Test if values persist within the same transaction
int test_general_1(Thread * Self) {
    int num_items=10;
    int res1,res2;


    // test 1
    TxStart (Self, 0) ;
    board_set(Self,num_items,12);
    res1=board_check(Self,num_items,12);
    board_set(Self,num_items,13);
    res2=board_check(Self,num_items,13);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }
    assert(res1);
    assert(res2);


    return 0;
}

// Test if values persist accross transactions
int test_general_2(Thread * Self) {
    int num_items=10;
    int res1;

    TxStart (Self, 0) ;
    board_set(Self,num_items,15);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }

    TxStart (Self, 0) ;
    res1=board_check(Self,num_items,15);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }
    assert(res1);

    TxStart (Self, 0) ;
    board_set(Self,num_items,16);
    board_set(Self,num_items,17);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }

    TxStart (Self, 0) ;
    res1=board_check(Self,num_items,17);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }
    assert(res1);

    return 0;
}

// Test local undo
int test_general_3(Thread * Self) {
    intptr_t LocalVar = 0;

    TxStart (Self, 0) ;
    //TxStoreLocal(Self, &LocalVar,1);
    assert(LocalVar==1);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }

    assert(LocalVar==1);

    TxStart (Self, 0) ;
    //TxStoreLocal(Self, &LocalVar,2);
    assert(LocalVar==2);
    //TxStoreLocal(Self, &LocalVar,3);
    assert(LocalVar==3);
    if(!TXCOMMIT(Self)) {
        assert(0);
    }
    assert(LocalVar==3);

    return 0;
}

// RO transaction
int test_general_4(Thread * Self) {
    int num_items=10;

    TxStart (Self, 0) ;
    board_set(Self,num_items,52);
    TXCOMMIT(Self);

    TxStart (Self, 1) ;
    board_check(Self,num_items,52);
    TXCOMMIT(Self);

    //    TxStart (Self, 1) ;
    //    board_set(Self,num_items,52);
    //    TXCOMMIT(Self);

    return 0;
}

void * test_general_5_collision(void * count)    {
    if(*(int*)count==1) {
        printf("provoking colision - started\n");
        Thread * Self2;
        Self2 = TxNewThread () ;
        // ------------------------------------
        // start main TXN
        TxStart (Self2, 0) ;
        TxStore(Self2, &(dboard[0]), 100);
        TXCOMMIT(Self2);
        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }
    printf("Antes do NULL\n");	
    return NULL;
    printf("Depois do NULL\n");
}

// Test tx colision T1:Ra - T2:Wa - T1:Wa
int test_general_5(Thread * Self) {
    int res1;
    int count=0;

    for(;;) {
        TxStart (Self, 0) ;
        count++;
        printf("starting tx witch count=%d\n",count);

        res1=TXLDV(dboard[0]);

        //----------------------
        pthread_t thr_a;
        pthread_create(&thr_a,NULL,test_general_5_collision,(void*)&count);
        pthread_join(thr_a,NULL);
        //----------------------

        TXSTV(dboard[0],res1+1);

        printf("going to commit tx witch count=%d\n",count);
        if(TXCOMMIT(Self)) {
            assert(count==2);
            break;
        }
    }
    assert(dboard[0]==res1+1&&res1==100);

    return 0;
}


void * test_general_6_collision(void * count)    {
    if(*(int*)count==1) {
        printf("provoking colision - started\n");
        Thread * Self2;
        Self2 = TxNewThread () ;
        // ------------------------------------
        // start main TXN
        TxStart (Self2, 0) ;
        int res1 = TxLoad(Self2, &(dboard[0]));
        int res2 = TXCOMMIT(Self2);
        printf("res=%d\n",res1);
        assert(!res2 || res1==0 || res1==2);
        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }

    return NULL;
}

// Test tx colision t1:W - t2:R
int test_general_6(Thread * Self) {
    int count=0;
    dboard[0]=0;

    for(;;) {
        TxStart (Self, 0) ;
        count++;
        printf("starting tx witch count=%d\n",count);

        TXSTV(dboard[0], 1);

        //----------------------
        pthread_t thr_a;
        pthread_create(&thr_a,NULL,test_general_6_collision,(void*)&count);
        //----------------------
        sched_yield();
        usleep(100000);
        usleep(100000);
        sched_yield();

        TXSTV(dboard[0], 2);

        if(TXCOMMIT(Self)) {
            pthread_join(thr_a,NULL);
            break;
        }
    }
    assert(count==1);

    return 0;
}



// Test for RS overflow
int test_overflow_1(Thread * Self) {
    int num_items=400;
    int i=0;

    for(;;) {
        TxStart (Self, 0) ;
        for(i=0;i<num_items;i++) {
            TXLDV(dboard[i]);
        }
        if(TXCOMMIT(Self)) {
            break;
        }
    }

    return 0;
}

// Test for WS overflow
int test_overflow_2(Thread * Self) {
    int num_items=400;
    int i=0;

    for(;;) {
        TxStart (Self, 0) ;
        printf("start\n");
        for(i=0;i<num_items;i++) {
            TXSTV(dboard[i],i);
        }
        printf("going to commit\n");
        if(TXCOMMIT(Self)) {
            printf("commited\n");
            break;
        }
    }

    TxStart (Self, 0) ;
    for(i=0;i<num_items;i++) {
        assert(TXLDV(dboard[i])==i);
    }
    if(!TXCOMMIT(Self)) {
        assert(0);
    }

    return 0;
}

// Test for RS and WS overflow
int test_overflow_3(Thread * Self) {
    int i;
    int num_items=800;
    int res1=1;

    for(;;) {
        res1=1;
        TxStart (Self, 0) ;
        for(i=0;i<num_items;i++) {
            TXSTV(dboard[i],100+i);

            int val2 = TXLDV(dboard[i]);
            if(val2!=(100+i))
                res1=0;
        }
        if(TXCOMMIT(Self)) {
            break;
        }
    }
    assert(res1);

    for(;;) {
        res1=1;
        TxStart (Self, 0) ;
        for(i=0;i<num_items;i++) {
            int val1 = TXLDV(dboard[i]);
            if(val1!=(100+i))
                res1=0;
        }
        if(TXCOMMIT(Self)) {
            break;
        }
    }
    assert(res1);

    return 0;
}

// Test abort - without retry (global variables)
int test_abort_1(Thread * Self) {
    intptr_t num_items=10;

    TxStart (Self, 0) ;
    board_set(Self,num_items,52);
    board_check(Self,num_items,52);
    TXCOMMIT(Self);

    TxStart (Self, 0) ;
    board_set(Self,num_items,53);
    board_check(Self,num_items,53);
    TxAbort(Self, 0);

    TxStart (Self, 0) ;
    board_check(Self,num_items,52);
    TXCOMMIT(Self);

    return 0;
}

// Test abort - with retry (local variables)
int test_abort_2(Thread * Self) {
    int val;
    volatile int count;
    intptr_t LocalVar=0;

    count=0;

    dboard[0]=0;


    // start TXN
    TxStart (Self, 0) ;

    count++;
    printf("count incremented to count= %d\n", count);

    //TxStoreLocal(Self, &LocalVar,count);
    TXSTV(dboard[0],count);

    if(count==1) {
        printf("aborted with count = %d\n", count);
        TxAbort(Self, 1);
    } else {
        TXCOMMIT(Self);
        printf("commited with count = %d\n", count);

    }
    // end TXN


    val = dboard[0];
    assert(val==2);
    assert(LocalVar==0);

    return 0;
}




// Test abort - without retry (local variables)
int test_abort_3(Thread * Self) {
    intptr_t LocalVar = 1;

    TxStart (Self, 0) ;
    //TxStoreLocal(Self, &LocalVar,2);
    assert(LocalVar==2);
    TxAbort(Self, 0);
    assert(LocalVar==1);

    TxStart (Self, 0) ;
    //TxStoreLocal(Self, &LocalVar,10);
    assert(LocalVar==10);
    //TxStoreLocal(Self, &LocalVar,11);
    assert(LocalVar==11);
    TxAbort(Self, 0);
    assert(LocalVar==1);

    return 0;
}

// 1 nested tx
int test_subtx_1(Thread * Self) {
    int val0;
    int val1;
    int val2;

    dboard[0]=0;
    dboard[1]=0;
    dboard[2]=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    TXSTV(dboard[1],1);

    val0 = TXLDV(dboard[0]);
    assert(val0==0);
    val1 = TXLDV(dboard[1]);
    assert(val1==1);
    val2 = TXLDV(dboard[2]);
    assert(val2==0);

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;

    TXSTV(dboard[2],1);

    val0 = TXLDV(dboard[0]);
    assert(val0==0);
    val1 = TXLDV(dboard[1]);
    assert(val1==1);
    val2 = TXLDV(dboard[2]);
    assert(val2==1);

    TXCOMMIT(Self);
    // end sub TXN ------------------------

    val0 = TXLDV(dboard[0]);
    assert(val0==0);
    val1 = TXLDV(dboard[1]);
    assert(val1==1);
    val2 = TXLDV(dboard[2]);
    assert(val2==1);
    TXCOMMIT(Self);

    val0 = dboard[0];
    assert(val0==0);
    val1 = dboard[1];
    assert(val1==1);
    val2 = dboard[2];
    assert(val2==1);

    return 1;
}

// 2 TX nested in 1 tx
int test_subtx_2(Thread * Self) {
    int val;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    TXSTV(dboard[0],1);
    val = TXLDV(dboard[0]);
    assert(val==1);

    // start sub TXN
    TxStart (Self, 0) ;

    TXSTV(dboard[0],1);
    val = TXLDV(dboard[0]);
    assert(val==1);

    TXCOMMIT(Self);
    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==1);

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;

    val = TXLDV(dboard[0]);
    assert(val==1);
    TXSTV(dboard[0],2);
    val = TXLDV(dboard[0]);
    assert(val==2);

    TXCOMMIT(Self);
    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==2);
    TXCOMMIT(Self);

    val = dboard[0];
    assert(val==2);

    return 1;
}

// sub-sub TX
int test_subtx_3(Thread * Self) {

    int val;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    TXSTV(dboard[0],1);
    val = TXLDV(dboard[0]);
    assert(val==1);

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;

    val = TXLDV(dboard[0]);
    assert(val==1);
    TXSTV(dboard[0],2);
    val = TXLDV(dboard[0]);
    assert(val==2);

    TXCOMMIT(Self);
    // end sub TXN ------------------------



    TXCOMMIT(Self);
    // end sub TXN ------------------------


    val = TXLDV(dboard[0]);
    assert(val==2);
    TXCOMMIT(Self);

    val = dboard[0];
    assert(val==2);


    return 1;
}

// main tx commit
// sub tx abort(retry=1)
int test_subtx_4(Thread * Self) {
    int val;
    volatile int countA=0;
    volatile int countB=0;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;
    printf("starting main tx\n");

    countA++;
    assert(countA<=1);
    assert(countB==0);

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;
    printf("starting sub tx\n");

    countB++;
    TXSTV(dboard[0],countB);

    if(countB==1) {
        printf("aborted with countB = %d\n", countB);
        TxAbort(Self, 1);
    } else {
        TXCOMMIT(Self);
        printf("commited with countB = %d\n", countB);
    }
    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==2);

    TXCOMMIT(Self);

    val = dboard[0];
    assert(val==2);

    return 1;
}

// sub tx abort(retry=0) - main tx commit
int test_subtx_5(Thread * Self) {
    int val;
    volatile int countA=0;
    volatile int countB=0;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN 1
    TxStart (Self, 0) ;
    printf("starting main tx\n");

    countA++;
    assert(countA==1);
    assert(countB==0);

    // ------------------------------------
    // start sub TXN 1-1
    TxStart (Self, 0) ;
    printf("starting sub tx\n");

    countB++;
    assert(countB==1);

    TXSTV(dboard[0],0);

    printf("aborted with countB = %d\n", countB);
    TxAbort(Self, 0);

    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==0);

    TXCOMMIT(Self);

    val = dboard[0];
    assert(val==0);

    return 1;
}

// sub tx abort(retry=0) - main tx abort(retry=1)
int test_subtx_6(Thread * Self) {
    int val;
    volatile int countA=0;
    volatile int countB=0;
    // ------------------------------------
    // start main TXN 2
    TxStart (Self, 0) ;
    printf("starting main tx 2\n");

    countA++;
    assert(countA<=2);
    assert(countB<=1);

    // ------------------------------------
    // start sub TXN 2-1
    TxStart (Self, 0) ;
    printf("starting sub tx 2-1\n");

    countB++;

    assert((countB==1 && countA==1) || (countB==2 && countA==2));

    TXSTV(dboard[0],0);

    printf("aborted with countB = %d\n", countB);
    TxAbort(Self, 0);

    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==0);

    if(countA==1) {
        assert(countB==1);
        TxAbort(Self, 1);
    } else if(countA==2) {
        assert(countB==2);
        TxAbort(Self, 0);
    } else {
        assert(0);
    }

    val = dboard[0];
    assert(val==0);

    return 1;
}


// main tx with 3 sub tx:
// main tx commits
// sub tx: abort commit abort
int test_subtx_7(Thread * Self) {
    int val;
    volatile int countA=0;
    volatile int countB=0;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;
    printf("starting main tx\n");

    countA++;
    assert(countA<=1);
    assert(countB==0);

    // ------------------------------------
    // start sub TXN
    TxStart (Self, 0) ;
    printf("starting sub tx\n");

    countB++;
    TXSTV(dboard[0],1);

    if(countB==1) {
        printf("aborted with countB = %d\n", countB);
        TxAbort(Self, 0);
    } else {
        assert(0);
    }
    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==0);

    // ------------------------------------
    // start 2nd sub TXN -> commit
    countB=0;
    TxStart (Self, 0) ;
    printf("starting sub tx\n");

    countB++;
    TXSTV(dboard[0],2);

    if(countB==1) {
        printf("commited with countB = %d\n", countB);
        TXCOMMIT(Self);
    } else {
        assert(0);
    }
    // end sub TXN ------------------------

    // ------------------------------------
    // start 3rd sub TXN -> abort
    countB=0;
    TxStart (Self, 0) ;
    printf("starting sub tx\n");

    countB++;
    TXSTV(dboard[0],3);

    if(countB==1) {
        printf("aborted with countB = %d\n", countB);
        TxAbort(Self, 0);
    } else {
        assert(0);
    }
    // end sub TXN ------------------------

    val = TXLDV(dboard[0]);
    assert(val==2);


    TXCOMMIT(Self);

    val = dboard[0];
    assert(val==2);

    return 1;

}

void * test_subtx_8_collision(void * countA)    {
    if(*(int*)countA==1) {
        printf("provoking colision - started\n");
        Thread * Self2;
        Self2 = TxNewThread () ;
        // ------------------------------------
        // start main TXN
        TxStart (Self2, 0) ;
        TxStore(Self2, &(dboard[0]), 100);
        TXCOMMIT(Self2);
        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }

    return NULL;
}


// simulate rs not coherent within subtx
int test_subtx_8(Thread * Self) {
    int val;
    volatile int countA=0;
    volatile int countB=0;

    dboard[0]=0;

    // ------------------------------------
    // start main TXN
    for(;;) {
        TxStart (Self, 0) ;
        printf("starting main tx with countA=%d and countB=%d\n", countA, countB);

        countA++;
        assert(countA<=2);

        // ------------------------------------
        // start sub TXN
        TxStart (Self, 0) ;
        printf("starting sub tx with countA=%d and countB=%d\n", countA, countB);

        countB++;
        assert((countB==1 && countA==1) || (countB==2 && countA==2));

        TXLDV(dboard[0]);
        TXSTV(dboard[0],1);

        //----------------------
        pthread_t thr_a;
        pthread_create(&thr_a,NULL,test_subtx_8_collision,(void*)&countA);
        sched_yield();//ensure the other thread got the chance to colide
        sched_yield();//ensure the other thread got the chance to colide
        usleep(100000);
        sched_yield();//ensure the other thread got the chance to colide
        sched_yield();//ensure the other thread got the chance to colide
        //----------------------
        printf("starting sub tx commit with countA=%d and countB=%d\n", countA, countB);
        if(!TXCOMMIT(Self)) {
            assert(countB==1);
        }
        // end sub TXN ------------------------

        val = TXLDV(dboard[0]);
        //assert(val==1);


        printf("starting main tx commit with countA=%d and countB=%d\n", countA, countB);
        if(TXCOMMIT(Self)) {
            pthread_join(thr_a,NULL);
            break;
        }
    }

    //a==1 && b==1 on enc
    //a==1 && b==1 on cmt
    assert((countB==1 && countA==1) || (countB==2 && countA==2));

    val = dboard[0];
    assert(val==1||val==100);

    return 1;
}

void * test_sterilize_1_collision(void * count) {
    int * to_release = NULL;
    if(*(int*)count==1) {
        printf("provoking colision - started\n");

        Thread * Self2;
        Self2 = TxNewThread () ;
        // ------------------------------------
        // start main TXN
        TxStart(Self2, 0) ;
        to_release = (int*)TxLoad(Self2,(intptr_t*)&qwe_thr);
        TxStore(Self2, (intptr_t *)(&qwe_thr), (uintptr_t) NULL);
        TXCOMMIT(Self2);

        TxSterilize(Self2,to_release,1);
        *to_release=100;
        free(to_release);

        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }

    return NULL;
}


//test TxSterilize
int test_sterilize_1(Thread * Self) {
    // pointer to real value
    qwe_thr = (int*)malloc(sizeof(int));
    *qwe_thr=1;

    int qwe,* ptr_qwe;

    volatile int count=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    count++;
    printf("startin with count = %d\n", count);

    ptr_qwe = (int*)TXLDV(qwe_thr);

    //----------------------
    pthread_t thr_a;
    pthread_create(&thr_a,NULL,test_sterilize_1_collision,(void*)&count);
    pthread_join(thr_a,NULL);

    if(ptr_qwe!=NULL) {
        qwe = TXLDA((uintptr_t)ptr_qwe); // it will abort and retry
        //assert(0); // if we reach here -> TX is inconsistent
        assert(!TxValid(Self));
    } else {
        assert(count==2);
    }

    TXCOMMIT(Self);

    return 1;
}


void * test_sterilize_2_collision(void * count)    {


    int ** to_release1 = NULL;
    int * to_release2 = NULL;
    if(*(int*)count==1) {
        printf("provoking colision - started\n");

        Thread * Self2;
        Self2 = TxNewThread () ;

        // ------------------------------------
        // start main TXN
        TxStart(Self2, 0) ;
        to_release1 = (int**)TxLoad(Self2,(intptr_t *)&ptr_qwe_thr);
        to_release2 = (int*)TxLoad(Self2,(intptr_t *)ptr_qwe_thr);
        TxStore(Self2, (intptr_t *)(&ptr_qwe_thr), (uintptr_t) NULL);
        TXCOMMIT(Self2);

        TxSterilize(Self2,to_release1,1);
        TxSterilize(Self2,to_release2,1);
        *to_release2=100;
        free(to_release2);
        *to_release1=(int*)0xBAD;
        free(to_release1);

        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }

    return NULL;
}

//test TxSterilize with segfault
int test_sterilize_2(Thread * Self) {
    // pointer to real value
    qwe_thr = (int*)malloc(sizeof(int));
    *qwe_thr=1;

    //pointer to pointer to real value
    ptr_qwe_thr = (int**)malloc(sizeof(int));
    *ptr_qwe_thr=qwe_thr;

    qwe_thr=NULL;

    int *qwe,** ptr_qwe;
    int val;

    volatile int count=0;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    count++;
    printf("startin with count = %d\n", count);

    ptr_qwe = (int**)TXLDV(ptr_qwe_thr);
    if(ptr_qwe==NULL) {
        assert(count==2);
        TxAbort(Self,0);
        return 1;
    }
    assert(count==1);


    //----------------------
    pthread_t thr_a;
    pthread_create(&thr_a,NULL,test_sterilize_2_collision,(void*)&count);
    pthread_join(thr_a,NULL);
    //----------------------

    printf("starting read of qwe\n");
    qwe = (int*)TxLoad(Self, (intptr_t *)ptr_qwe_thr);
    printf("starting read of val\n");
    val = (int) TxLoad(Self, (intptr_t *)qwe);
    assert(0);



    printf("starting commit with count = %d\n", count);
    TXCOMMIT(Self);
    printf("finished commit with count = %d\n", count);

    return 1;
}




void * test_sterilize_3_collision(void * count)    {


    int ** to_release1 = NULL;
    int * to_release2 = NULL;
    if(*(int*)count==1) {
        printf("provoking colision - started\n");

        Thread * Self2;
        Self2 = TxNewThread () ;

        // ------------------------------------
        // start main TXN
        TxStart(Self2, 0) ;
        to_release1 = (int**)TxLoad(Self2,(intptr_t *)&ptr_qwe_thr);
        to_release2 = (int*)TxLoad(Self2,(intptr_t *)ptr_qwe_thr);
        TxStore(Self2, (intptr_t *)(&ptr_qwe_thr), (uintptr_t) NULL);
        TXCOMMIT(Self2);

        TxSterilize(Self2,to_release1,1);
        TxSterilize(Self2,to_release2,1);
        *to_release2=100;
        free(to_release2);
        *to_release1=(int*)0xBAD;
        free(to_release1);

        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }
    return NULL;
}

//test TxSterilize with segfault on sub tx
int test_sterilize_3(Thread * Self) {
    // pointer to real value
    qwe_thr = (int*)malloc(sizeof(int));
    *qwe_thr=1;

    //pointer to real value
    ptr_qwe_thr = (int**)malloc(sizeof(int));
    *ptr_qwe_thr=qwe_thr;

    qwe_thr=NULL;

    int *qwe,** ptr_qwe;
    int val;

    volatile int count=0;
    count=0;


    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    // ------------------------------------
    // start main TXN
    TxStart (Self, 0) ;

    count++;
    printf("startin with count = %d\n", count);



    ptr_qwe = (int**)TXLDV(ptr_qwe_thr);
    if(ptr_qwe==NULL) {
        assert(count==2);
        TxAbort(Self,0);
        TxAbort(Self,0);
        return 1;
    }
    assert(count==1);


    //----------------------
    pthread_t thr_a;
    pthread_create(&thr_a,NULL,test_sterilize_3_collision,(void*)&count);
    pthread_join(thr_a,NULL);
    //----------------------

    qwe = (int*)TxLoad(Self, (intptr_t *)ptr_qwe_thr);
    val = (int) TxLoad(Self, (intptr_t *)qwe);
    assert(0);



    printf("starting commit with count = %d\n", count);
    TXCOMMIT(Self);
    printf("finished commit with count = %d\n", count);
    TXCOMMIT(Self);
    return 1;
}


// -----------------------------------------------------------

#ifdef OBJ_STM
// OBJSTM with simple tx
int test_objstm_1(Thread * Self) {
    int res1, res2, res3;
    obj1.a=0;
    obj1.b=0;
    obj1.c=0;

    TxStart (Self, 0) ;

    TxOpenReadENC(Self,&dboard[0]);
    res1=obj1.a;
    res2=obj1.b;
    res3=obj1.c;
    assert(res1==0 &&res2==0&&res3==0);

    TxOpenWriteENC(Self, &obj1,sizeof(obj_t));
    obj1.b=1;
    obj1.c=2;

    if(!TXCOMMIT(Self)) {
        assert(0);
    }

    assert(obj1.a==0 && obj1.b==1 && obj1.c==2);

    return 1;
}

// OBJSTM with tx abort
int test_objstm_2(Thread * Self) {
    int res1, res2, res3;
    obj1.a=0;
    obj1.b=0;
    obj1.c=0;

    TxStart (Self, 0) ;

    TxOpenReadENC(Self,&obj1);
    res1=obj1.a;
    res2=obj1.b;
    res3=obj1.c;
    assert(res1==0 &&res2==0&&res3==0);

    TxOpenWriteENC(Self, &obj1, sizeof(obj_t));
    obj1.b=1;
    obj1.c=2;

    TxAbort(Self, 0);

    assert(obj1.a==0 && obj1.b==0 && obj1.c==0);

    return 1;
}



void * test_objstm_3_collision(void * count)    {
    if(*(int*)count==1) {
        printf("provoking colision - started\n");
        Thread * Self2;
        Self2 = TxNewThread () ;
        // ------------------------------------
        // start main TXN
        TxStart (Self2, 0) ;
        TxOpenWriteENC(Self2, &obj1, sizeof(obj_t));
        obj1.a=100;
        obj1.b=101;
        obj1.c=102;
        TXCOMMIT(Self2);
        TxEndThread(Self2);
        printf("provoking colision - finished\n");
    }

    return NULL;
}

//objstm with colision
int test_objstm_3(Thread * Self) {
    int count=0;
    //int res1, res2, res3;
    obj1.a=0;
    obj1.b=0;
    obj1.c=0;

    for(;;) {
        TxStart (Self, 0) ;
        count++;
        printf("starting tx witch count=%d\n",count);

        TxOpenReadENC(Self, &obj1);
        if(count==1) {
            assert(obj1.a==0 && obj1.b==0 && obj1.c==0);
        } else if(count==2) {
            assert(obj1.a==100 && obj1.b==101 && obj1.c==102);
        } else {
            assert(0);
        }

        //----------------------
        pthread_t thr_a;
        pthread_create(&thr_a,NULL,test_objstm_3_collision,(void*)&count);
        pthread_join(thr_a,NULL);
        //----------------------
        assert(obj1.a==100 && obj1.b==101 && obj1.c==102);

        printf("going to commit tx witch count=%d\n",count);
        if(TXCOMMIT(Self)) {
            assert(count==2);
            break;
        }
    }
    assert(obj1.a==100 && obj1.b==101 && obj1.c==102);

    return 1;
}
#endif


// -------------------------------------------------------------------------
int general_tests(Thread * Self) {
    printf("STARTING GENERAL TESTS\n");

    printf("test_general_1 started\n");
    test_general_1(Self);
    printf("test_general_1 finished\n\n");

    printf("test_general_2 started\n");
    test_general_2(Self);
    printf("test_general_2 finished\n\n");

    //printf("test_general_3 started\n");
    //test_general_3(Self);
    //printf("test_general_3 finished\n\n");

    printf("test_general_4 started\n");
    test_general_4(Self);
    printf("test_general_4 finished\n\n");

    printf("test_general_5 started\n");
    test_general_5(Self);
    printf("test_general_5 finished\n\n");

    printf("test_general_6 started\n");
    test_general_6(Self);
    printf("test_general_6 finished\n\n");

    return 1;
}

int overflow_tests(Thread * Self) {
    printf("STARTING OVERFLOW TESTS\n");

    printf("test_overflow_1 started\n");
    test_overflow_1(Self);
    printf("test_overflow_1 finished\n\n");

    printf("test_overflow_2 started\n");
    test_overflow_2(Self);
    printf("test_overflow_2 finished\n\n");

    printf("test_overflow_3 started\n");
    test_overflow_3(Self);
    printf("test_overflow_3 finished\n\n");

    return 1;
}

int abort_tests(Thread * Self) {
    printf("STARTING ABORT TESTS\n");

    printf("test_abort_1 started\n");
    test_abort_1(Self);
    printf("test_abort_1 finished\n\n");

    printf("test_abort_2 started\n");
    test_abort_2(Self);
    printf("test_abort_2 finished\n\n");

    printf("test_abort_3 started\n");
    test_abort_3(Self);
    printf("test_abort_3 finished\n\n");

    return 1;
}

int subtx_tests(Thread * Self) {
    printf("STARTING SUBTX TESTS\n");

#ifndef DISABLE_NESTING

    printf("test_subtx_1 started\n");
    test_subtx_1(Self);
    printf("test_subtx_1 finished\n\n");

    printf("test_subtx_2 started\n");
    test_subtx_2(Self);
    printf("test_subtx_2 finished\n\n");

    printf("test_subtx_3 started\n");
    test_subtx_3(Self);
    printf("test_subtx_3 finished\n\n");

    printf("test_subtx_4 started\n");
    test_subtx_4(Self);
    printf("test_subtx_4 finished\n\n");

    printf("test_subtx_5 started\n");
    test_subtx_5(Self);
    printf("test_subtx_5 finished\n\n");

    printf("test_subtx_6 started\n");
    test_subtx_6(Self);
    printf("test_subtx_6 finished\n\n");

    printf("test_subtx_7 started\n");
    test_subtx_7(Self);
    printf("test_subtx_7 finished\n\n");

    printf("test_subtx_8 started\n");
    test_subtx_8(Self);
    printf("test_subtx_8 finished\n\n");
#endif

    return 1;
}

int sterilize_tests(Thread * Self) {
    printf("STARTING STERILIZE TESTS\n");

    printf("test_sterilize_1 started\n");
    test_sterilize_1(Self);
    printf("test_sterilize_1 finished\n\n");

#if ! defined DISABLE_TXSTERILIZE_EXT

    printf("test_sterilize_2 started\n");
    test_sterilize_2(Self);
    printf("test_sterilize_2 finished\n\n");

#if ! defined DISABLE_NESTING

    printf("test_sterilize_3 started\n");
    test_sterilize_3(Self);
    printf("test_sterilize_3 finished\n\n");
#endif
#endif

    return 1;
}

int objstm_tests(Thread * Self) {
    printf("STARTING OBJSTM TESTS\n");

#if defined MODEENC && defined OBJ_STM
    printf("test_objstm_1 started\n");
    test_objstm_1(Self);
    printf("test_objstm_1 finished\n\n");

    printf("test_objstm_2 started\n");
    test_objstm_2(Self);
    printf("test_objstm_2 finished\n\n");

    printf("test_objstm_3 started\n");
    test_objstm_3(Self);
    printf("test_objstm_3 finished\n\n");

#endif

    return 1;
}

// --------------------------------------------
int main(int argc, char *argv[]) {
    seed= ((int) &seed) + RDTICK() ;
    seed |=1 ;

    sleep_time = 1;
    board_size = 1000;
    /*
        if(argc!=3) {
            printf("syntax: bin \n");
            exit(1);
        }
        sleep_time = atoi(argv[1]);
        board_size = atoi(argv[2]);
    */
    board_init(board_size);

    //TxOnce();

    Thread * Self ;
    Self = TxNewThread () ;



    general_tests(Self) ;
    overflow_tests(Self) ;
//    abort_tests(Self) ;
//    sterilize_tests(Self) ;
	subtx_tests(Self) ;
    objstm_tests(Self);


    /*
     * mais testes:
     * isolamento
     * synchronizao
    rs/ws overflow within subtx
    are we running in consistent states
     * 
     * */



    TxEndThread(Self);


    printf("END OF PROGRAM. TY for using.\n\n");
    return 0;
}
