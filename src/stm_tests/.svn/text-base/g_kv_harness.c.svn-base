#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <libspe2.h>

#include <sys/time.h>
#include <time.h>
#include "g_list_stm_tl.h"

//-----------------------------
#define RDTICK_X86_32() \
	({ unsigned long long __t; __asm__ __volatile__ ("rdtsc" : "=A" (__t)); __t; })

#define RDTICK_X86_64() \
	({ unsigned long long __t, __tl, __th; __asm__ __volatile__ ("rdtsc" : "=a" (__tl), "=d" (__th)); __t=(__th<<32)|__tl; __t; })


#define CELL
//#define MYRAND random
//#define MYRAND rand
#define MYRAND() rand_r(&(run_stats[thread_id].rand_seed))
#define THREAD_INFO Thread
//#define MYRAND() MarsagliaXOR((int*)&(run_stats[thread_id].rand_seed))


//#if defined X86_32
//#define GET_TIME RDTICK_X86_32
//#elif defined X86_64
//#define GET_TIME RDTICK_X86_64
//#else
//#error no flag defined
//#endif

//#define GET_TIME() (0)

#include <sys/time.h>

#define GET_TIME() ({\
	unsigned long long _ull_time;\
	struct timeval ts;\
	gettimeofday(&ts, NULL);\
	_ull_time = (ts.tv_sec * 1.e6) + ts.tv_usec;\
	_ull_time;\
})


#ifdef CELL
/*typedef struct _set_t {
    //double padA [32] ;//Also change the MAX_OBJ_SIZE
    struct _node_t *root;
    //double padB [32] ;
}
set_t;*/


typedef struct{
	unsigned long long int listPointer;
	unsigned long long int runStatsPointer;
	unsigned long long int thread_id;
}userStruct_t __attribute__((aligned(128))) ;
#endif

//-----------------------------
#ifdef TL2
#include "g_kv_stm_tl.h"
//#include <trace.h>
//typedef void set_t ;

#define STAT_ARRAY_MAX_THREADS 1000
#define STAT_ARRAY_MAX_COUNTERS 20
int _tl_stats[STAT_ARRAY_MAX_THREADS][STAT_ARRAY_MAX_COUNTERS];

#define THREAD_INFO Thread
#define NEW_THREAD TXNEWTHREAD
#define END_THREAD TXENDTHREAD

#define kv_put__(set, key, val) kv_put(Self, set, key, val);
#define kv_delete__(set, key) kv_delete(Self, set, key);
#define kv_get__(set, key) kv_get(Self, set, key);
#define kv_verify__(set, v) kv_verify(Self, set, v);
#define kv_print__(set) kv_print(Self, set);
#endif

#ifdef ENNALS
#include "g_kv_stm_ennals.h"
typedef void set_t ;

#define THREAD_INFO void
#define NEW_THREAD() NULL
#define END_THREAD(_s)
int num_threads;

#define kv_put__(set, key, val) kv_put(set,key,val)
#define kv_delete__(set, key) kv_delete(set, key);
#define kv_get__(set, key) kv_get(set, key);
#define kv_verify__(set, v) kv_verify(set, v);
#define kv_print__(set) kv_print(set);
#endif

//------------------------------


typedef struct _thread_stats {
    char padA[32];
    int num_puts;
    int num_deletes;
    int num_gets;

    int num_loads;
    int num_vfys;
    int num_stores;
    int num_rs_ovf;
    int num_ws_ovf;
    int num_ok_commits;
    int num_user_aborts;
    int num_ld_aborts;
    int num_vfy_aborts;
    int num_st_aborts;
    int num_segfault_aborts;
    int num_cmt_aborts;
    int num_total_aborts;

    unsigned long long test_time;
    unsigned long long stm_op_time;
    unsigned long long harness_op_time;

    unsigned int rand_seed;
    char padB[32];
}thread_stats __attribute__((aligned(128)));


typedef struct _controlStruct{
	unsigned long long remoteThreadStats;
	unsigned long long infoPointer;
}controlStruct __attribute__((aligned(128)));

typedef struct _info {
	unsigned long long pointerToList;
	int time;
	int keyRange;
	int pctPut;
	int pctGet;
	int pctRemove;
}info __attribute__((aligned(128)));

//-----------------------------

int num_worker_threads=1;
unsigned int sleep_time=10;
int pct_put=40;
int pct_del=40;
int pct_get=20;
int init_size=100;
int key_range=500;

volatile int can_stop=0;
volatile int can_start=0;

set_t *set;

thread_stats * run_stats=NULL;
controlStruct * controlStructThread = NULL;

//-----------------------------


//void dummy_test(THREAD_INFO * Self) ;
//-----------------------------
static inline int MarsagliaXOR (int * rseed) {
    int x = *rseed;
    if (x == 0) {
        x = 1 ;
    }
    x ^= x << 6;
    x ^= ((unsigned)x) >> 21;
    x ^= x << 7 ;

    *rseed = x ;
    return x & 0x7FFFFFFF;
}


static void SigHandler( int sig ) {
    if(!can_stop) {
        can_stop=1;
    } else {
        exit(1);
    }

}



void * worker_thr(void * args) {
    THREAD_INFO * Self ;
    Self = NEW_THREAD() ;

    long thread_id=(long)args;
    //printf ("thread_id = %d\n",thread_id);

    //printf("%d: worker_thr\n", Self->UniqID);

    //    int worker_successfull_commits=0;
    //    int worker_failed_commits=0;
    int num_puts=0;
    int num_deletes=0;
    int num_gets=0;

    while (!can_start)
        sched_yield();

    unsigned long long test_time=0;
    unsigned long long test_start_time=0;
    unsigned long long test_end_time=0;

    unsigned long long harness_op_time=0;
    unsigned long long start_harness_op_time, stop_harness_op_time;

    unsigned long long stm_op_time=0;
    unsigned long long start_stm_op_time, stop_stm_op_time;

    test_start_time=GET_TIME();
    test_start_time=GET_TIME();
    test_start_time=GET_TIME();

    while(!can_stop) {
        //        int i;
        //        for(i=0;i<200000;i++){


        start_harness_op_time = GET_TIME();
        int rnd1 = MYRAND() ;
        int rnd2 = MYRAND() ;
        
        //printf("rnd %12d %12d\n",rnd1,rnd2);
        int pct = (int) (100.0 * (rnd1 / (RAND_MAX + 1.0)));
        setval_t key = (long) ((double)key_range * (rnd2 / (RAND_MAX + 1.0)));
        setkey_t val = key+100;
        //        int key = i;
        //        int val = i;

        assert(pct>=0 &&pct <100);
        assert(key>=0 &&key <key_range);

        start_stm_op_time = stop_harness_op_time = GET_TIME();
        if(pct<pct_put) {
            kv_put__ (set, key, val);
            num_puts++;
        } else if(pct<pct_put+pct_del) {
            kv_delete__(set, key);
            num_deletes++;
        } else if(pct<pct_put+pct_del+pct_get) {
            val = kv_get__(set, key);
            assert(val==0 || val==key+100);
            num_gets++;
        } else {
            assert(0);
        }

        stop_stm_op_time = GET_TIME();

        stm_op_time += (stop_stm_op_time - start_stm_op_time);
        harness_op_time += (stop_harness_op_time - start_harness_op_time);
        //sched_yield();
    }

    test_end_time=GET_TIME();
    test_time=test_end_time-test_start_time;

#ifdef TL2
#ifndef DISABLE_STATS_COUNT

    if(Self!=NULL) {
        run_stats[thread_id].num_loads          =Self->stats[_tl_num_loads];
        run_stats[thread_id].num_vfys           =Self->stats[_tl_num_vfys];
        run_stats[thread_id].num_stores         =Self->stats[_tl_num_stores];
        run_stats[thread_id].num_rs_ovf         =Self->stats[_tl_num_rs_ovf];
        run_stats[thread_id].num_ws_ovf         =Self->stats[_tl_num_ws_ovf];
        run_stats[thread_id].num_ok_commits     =Self->stats[_tl_num_ok_commits];
        run_stats[thread_id].num_user_aborts    =Self->stats[_tl_num_user_aborts];
        run_stats[thread_id].num_ld_aborts      =Self->stats[_tl_num_ld_aborts];
        run_stats[thread_id].num_vfy_aborts     =Self->stats[_tl_num_vfy_aborts];
        run_stats[thread_id].num_st_aborts      =Self->stats[_tl_num_st_aborts];
        run_stats[thread_id].num_segfault_aborts=Self->stats[_tl_num_segfault_aborts];
        run_stats[thread_id].num_cmt_aborts     =Self->stats[_tl_num_cmt_aborts];
        run_stats[thread_id].num_total_aborts   =Self->stats[_tl_num_total_aborts];
    }
#endif
#endif

    END_THREAD(Self);

    run_stats[thread_id].num_puts=num_puts;
    run_stats[thread_id].num_deletes=num_deletes;
    run_stats[thread_id].num_gets=num_gets;

    run_stats[thread_id].test_time = test_time;
    run_stats[thread_id].stm_op_time = stm_op_time;
    run_stats[thread_id].harness_op_time = harness_op_time;


    return NULL;
}//worker_thr



typedef struct ppu_pthread_data {
  int i;
} ppu_pthread_data_t;

void *ppu_pthread_function2(void *arg) {
  //TxStartSPE(spuCode,&dataToSend);
	ppu_pthread_data_t *data = (ppu_pthread_data_t *) arg;
	//TxStartSPE(list_stm_tl_spu,&controlStructThread[data->i]);
	//TxStartSPEBlocking(spuCode,&dboard);
  pthread_exit(NULL);
}





int main(int argc, char *argv[]) {
    int res;
    struct timeval time_seed, time_start, time_end;
    extern spe_program_handle_t list_stm_tl_spu;


    gettimeofday(&time_seed, NULL);


    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);


		//trace_log_begin();

    if(argc!=8) {
        printf("syntax: bin sleep_time num_worker_threads pct_ins pct_upd pct_del pct_get key_range init_size\n");
        if(argc!=1 && argc!=8)
            exit(1);
    }

    if(argc==8) {
        sleep_time = atoi(argv[1]);
        num_worker_threads = atoi(argv[2]);
        pct_put = atoi(argv[3]);
        pct_del = atoi(argv[4]);
        pct_get = atoi(argv[5]);
        key_range = atoi(argv[6]);
        init_size = atoi(argv[7]);
    }
    //ppu_pthread_data_t ptdata[num_worker_threads];
    assert(pct_put+pct_del+pct_get==100);
    assert(init_size <=key_range);
    assert(key_range>0);
    assert(num_worker_threads > 0);

    run_stats=malloc(sizeof(thread_stats)*num_worker_threads);
    controlStructThread= malloc(sizeof(controlStruct)*num_worker_threads);
    pthread_t *worker_thr_a=malloc(sizeof(pthread_t)*num_worker_threads);
    //init random seed
	
	int i;
    for(i=0;i<num_worker_threads;i++) {

        run_stats[i].rand_seed=(unsigned int)(&run_stats[i])+time_seed.tv_sec+time_seed.tv_usec+1000*i;
        //seed=2348990670U;
        //printf("seed=%u\n",seed);
    }
    srand(run_stats[0].rand_seed);

    THREAD_INFO * Self ;
    Self = NEW_THREAD() ;

    kv_init();
    set = kv_create(Self);


    //printf("starting initial fill\n");
    //----------------------------
    for(i=0;i<init_size;i++) {
        //setkey_t key = i;

        setval_t key = (setkey_t) ((double)key_range * (random() / (RAND_MAX + 1.0)));


        kv_put__(set, key, key+100);
    }
    printf("Set created..\n");
    //printf("starting initial verification\n");
    //res = kv_verify__(set,1);
		res = kv_verify__(set,0);
    assert(res>=0);

    //printf("starting test run\n");


    END_THREAD(Self);


#ifndef CELL
    for(i=0; i<num_worker_threads;i++) {
        pthread_create(&(worker_thr_a[i]),NULL,worker_thr,(int*)(long)i);
    }
#endif

    gettimeofday(&time_start, NULL);
    can_start=1;
    usleep(sleep_time*100000);
    can_stop=1;
    gettimeofday(&time_end, NULL);

    //printf("test run finished - waiting for workers\n");
    //fflush(stdout);

#ifdef CELL
    info myInfo;// = (myInfo) malloc(sizeof(info));
    myInfo.pointerToList = (unsigned long long) set;
    myInfo.time = sleep_time;
    myInfo.keyRange = key_range;
    myInfo.pctPut =pct_put;
    myInfo.pctGet =pct_get;
    myInfo.pctRemove =pct_del;
    
    
    
    //ATENCAO! aqui efectuar
    /*userStruct_t userStruct = malloc(sizeof(userStruct_t));
    userStruct->listPointer = ;
    userStruct->runStatsPointer = run_stats;
    userStruct ->thread_id = ;*/


    //Aqui fazer lista de apontadores para estrutura UserStruct.
    //qq coisa dentro do for que: userStruct vector[maxThread]
    //depois TxStartSPE(g_list_stm_tl_spu, userStruct[i])
    printf("Launching TxStartSPE\n");
    for(i=0;i<num_worker_threads;i++){

			controlStructThread[i].remoteThreadStats= (unsigned long long) &run_stats[i];
			controlStructThread[i].infoPointer = (unsigned long long) &myInfo;
			printf("##HarnessCellPPE## Setting pointer controlStruct[%d] -> %p  \n", i, &controlStructThread[i]);
			printf("##HarnessCellPPE## Setting pointer remoteThreadStats -> %llx  \n", controlStructThread[i].remoteThreadStats);
			printf("##HarnessCellPPE## Default infoPointer -> %p  \n", &myInfo);
			printf("##HarnessCellPPE## Setting pointer infoPointer -> %llx  \n", controlStructThread[i].infoPointer);
			printf("##HarnessCellPPE## Setting pointer pointerToList -> %llx  \n", myInfo.pointerToList);
			printf("##HarnessCellPPE## Size of mySet -> %d  \n", sizeof(set));
			printf("##HarnessCellPPE## Size of node_t -> %d  \n", sizeof(node_t));
			printf("##HarnessCellPPE## Size of info -> %d  \n", sizeof(myInfo));
			printf("##HarnessCellPPE## Pointer to root(%p) -> %p  \n", set->root);
			printf("##HarnessCellPPE## Pointer to root(%uul) -> %p  \n", (unsigned long long) set->root );
			printf("##HarnessCellPPE## root.k = %d root.v=%d\n", set->root->k, set->root->v);
			printf("##HarnessCellPPE## root.n = %p root.p=%p\n", set->root->n, set->root->p);

			//ptdata[i]->i=i;
			//pthread_create(&pthread, NULL, &ppu_pthread_function, &ptdata[i]);

			//Aqui fazer TXJOIN ouutra vez.
			//pthread_join(worker_thr_a[i], NULL);

        	TxStartSPE(list_stm_tl_spu, &controlStructThread[i]);
        	//TxStartSPE(list_stm_tl_spu, &res);
        	printf("##Starting SPE thread## TxStartSPE\n");
    }
#endif

#ifndef CELL
    for(i=0; i<num_worker_threads;i++) {
        pthread_join(worker_thr_a[i],NULL);
        pthread_join(pthread, i);
        printf("worker_join\n");
    }
#endif
    //printf("end of worker_join's\n");
    //fflush(stdout);

    int total_time, total_s, total_u;
    total_s = (time_end.tv_sec - time_start.tv_sec);
    total_u = (time_end.tv_usec - time_start.tv_usec);
    total_time = total_s * 1000 + total_u/1000 ;
    //printf("Totaltime(ms) = %d\n", total_time);

    int total_puts=0;
    int total_deletes=0;
    int total_gets=0;
    int total_total=0;
    int total_num_loads=0;
    int total_num_vfys=0;
    int total_num_stores=0;
    int total_num_rs_ovf=0;
    int total_num_ws_ovf=0;
    int total_num_ok_commits=0;
    int total_num_user_aborts=0;
    int total_num_ld_aborts=0;
    int total_num_vfy_aborts=0;
    int total_num_st_aborts=0;
    int total_num_segfault_aborts=0;
    int total_num_cmt_aborts=0;
    int total_num_total_aborts=0;

    unsigned long long total_test_time=0;
    unsigned long long total_stm_op_time=0;
    unsigned long long total_harness_op_time=0;

    for(i=0; i<num_worker_threads;i++) {
        int thread_total= run_stats[i].num_puts+run_stats[i].num_deletes+run_stats[i].num_gets;

        total_puts+=run_stats[i].num_puts;
        total_deletes+=run_stats[i].num_deletes;
        total_gets+=run_stats[i].num_gets;
        total_total+=thread_total;

        total_num_loads += run_stats[i].num_loads;
        total_num_vfys += run_stats[i].num_vfys;
        total_num_stores += run_stats[i].num_stores;
        total_num_rs_ovf += run_stats[i].num_rs_ovf;
        total_num_ws_ovf += run_stats[i].num_ws_ovf;
        total_num_ok_commits += run_stats[i].num_ok_commits;
        total_num_user_aborts += run_stats[i].num_user_aborts;
        total_num_ld_aborts += run_stats[i].num_ld_aborts;
        total_num_vfy_aborts += run_stats[i].num_vfy_aborts;
        total_num_st_aborts += run_stats[i].num_st_aborts;
        total_num_segfault_aborts += run_stats[i].num_segfault_aborts;
        total_num_cmt_aborts += run_stats[i].num_cmt_aborts;
        total_num_total_aborts += run_stats[i].num_total_aborts;

        total_test_time += run_stats[i].test_time;
        total_stm_op_time += run_stats[i].stm_op_time;
        total_harness_op_time += run_stats[i].harness_op_time;

    }
		/*
    printf("TOTALS, put=%d, del=%d, get=%d total=%d\n",
           total_puts,
           total_deletes,
           total_gets,
           total_total);

    printf("num_loads             = %d\n", total_num_loads);
    printf("num_vfys              = %d\n", total_num_vfys);
    printf("num_stores            = %d\n", total_num_stores);
    printf("num_rs_ovf            = %d\n", total_num_rs_ovf);
    printf("num_ws_ovf            = %d\n", total_num_ws_ovf);
    printf("num_ok_commits        = %d\n", total_num_ok_commits);
    printf("num_user_aborts       = %d\n", total_num_user_aborts);
    printf("num_ld_aborts         = %d\n", total_num_ld_aborts);
    printf("num_vfy_aborts        = %d\n", total_num_vfy_aborts);
    printf("num_st_aborts         = %d\n", total_num_st_aborts);
    printf("num_segfault_aborts   = %d\n", total_num_segfault_aborts);
    printf("num_cmt_aborts        = %d\n", total_num_cmt_aborts);
    printf("num_total_aborts      = %d\n", total_num_total_aborts);
    printf("total_test_time       = %llu\n", total_test_time);
    printf("total_stm_op_time     = %llu\n", total_stm_op_time);
    printf("total_harness_op_time = %llu\n", total_harness_op_time);
    printf("GREP_T: %d %d %d %d - %d %d %d %d %d %d %d %d %d %d %d %d %d %llu %llu %llu\n",
           total_puts,
           total_deletes,
           total_gets,
           total_total,
           total_num_loads,
           total_num_vfys,
           total_num_stores,
           total_num_rs_ovf,
           total_num_ws_ovf,
           total_num_ok_commits,
           total_num_user_aborts,
           total_num_ld_aborts,
           total_num_vfy_aborts,
           total_num_st_aborts,
           total_num_segfault_aborts,
           total_num_cmt_aborts,
           total_num_total_aborts,
           total_test_time,
           total_stm_op_time,
           total_harness_op_time);
		*/
		unsigned long long total_op = total_total;
		printf("%d\t%d\t%llu\t%llu\t%llu\t%d\t%0.f\t%.0f\n",
           total_total,
           total_num_total_aborts,
           total_test_time,
           total_stm_op_time,
           total_harness_op_time,
					 total_time,
					 (float)total_total*1000/total_time,
					 (float)(total_op*1000000)/((float)total_test_time/num_worker_threads));

#ifndef CELL
    free(worker_thr_a);
#endif

		//trace_log_end();

    //printf("starting final verification\n");
    Self = NEW_THREAD() ;
    //res = kv_verify__(set,1);
		res = kv_verify__(set,0);
    assert(res>=0);
    END_THREAD(Self);
    return 0;
}



