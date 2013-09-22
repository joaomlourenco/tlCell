#include <stdio.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <stdint.h>
//#include "g_kv_stm_tl.h" //ATENCAO!
#include "tl_spu.h"





/* Alignment macros */
#define SPE_ALIGNMENT 16
#define SPE_ALIGNMENT_FULL 128
#define SPE_ALIGN __attribute__((aligned(16)))
#define SPE_ALIGN_FULL __attribute__((aligned(128)))
#define ROUND_UP_ALIGN(value, alignment)\
 (((value) + ((alignment) - 1))&(~((alignment)-1)))
/*End of alignment macros*/


#define vfy_assert(x) assert(x || !TxValid(Self))

/* Alignment macros */
#define SPE_ALIGNMENT 16
#define SPE_ALIGNMENT_FULL 128
#define SPE_ALIGN __attribute__((aligned(16)))
#define SPE_ALIGN_FULL __attribute__((aligned(128)))
#define ROUND_UP_ALIGN(value, alignment)\
 (((value) + ((alignment) - 1))&(~((alignment)-1)))
/*End of alignment macros*/



/*LIST DEFINES*/
//---------------------------------

//typedef unsigned long long setkey_t;
//typedef unsigned long long setkey_t;
typedef intptr_t setkey_t;
typedef intptr_t setval_t;

typedef struct _thread_stats {
    char padA[32];
    int num_puts; //align?
    int num_deletes; //align?
    int num_gets;  //align?

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
}thread_stats SPE_ALIGN_FULL;

typedef struct _controlStruct{
	unsigned long long remoteThreadStats;
	unsigned long long infoPointer;
}controlStruct SPE_ALIGN_FULL;

typedef struct _info {
	unsigned long long pointerToList;
	int time;
	int key_range;
	int pct_put;
	int pct_get;
	int pct_del;
}info SPE_ALIGN_FULL;

typedef struct _node_t {
	unsigned long long k;
    unsigned long long v;
    unsigned long long n; //isto tem que ser Unsigned long long e aligned?
    unsigned long long p; //isto tem que ser Unsigned long long e aligned?
} node_t SPE_ALIGN_FULL; //or should it be SPE_ALIGN only?

typedef struct _set_t {
    //double padA [32] ;//Also change the MAX_OBJ_SIZE
    unsigned long long root; //isto tem que ser Unsigned long long
	//struct _node_t *root;
    //double padB [32] ;
}set_t SPE_ALIGN_FULL;



// --------------------------------
#define TXSTART     TxStart
#define TXCOMMIT    TxCommit 
#define TXSTERILIZE TxSterilize 
#define TXVALID     TxValid 
#define TXVALIDATE_AND_ABORT     TxValidateAndAbort 
#define TXNEWTHREAD TxNewThread
#define TXENDTHREAD TxEndThread

/* DEFINES   */
#define READ_OBJ(__o)       
#define WRITE_OBJ(__o, __s)      
#define UNTX(_n)           TXSTERILIZE(Self,_n,sizeof(node_t)/sizeof(intptr_t))
#define VFY(_o)
//--------------
#define GET_KEY(_n)          ((intptr_t)TXLDF((_n),k))
#define GET_VALUE(_n)        ((intptr_t)TXLDF((_n),v))
#define GET_NODE(_n,_f)      ((node_t *)TXLDF((_n),_f))
//--------------
#define SET_KEY(_n,_k)       TXSTF((_n),k,(_k))
#define SET_VALUE(_n,_v)     TXSTF((_n),v,(_v))
#define SET_NODE(_n,_f,_v)   TXSTF((_n),_f,(_v))



/*MORE*/

#define GET_ROOT(_s)         GET_NODE(((set_t*)_s),root)
#define SET_ROOT(_s,_v)      SET_NODE(((set_t*)_s),root,(_v))

#define GET_NEXT(_n)         GET_NODE(_n,n)
#define GET_PREV(_n)         GET_NODE(_n,p)
#define SET_NEXT(_n,_v)      SET_NODE(_n,n,_v)
#define SET_PREV(_n,_v)      SET_NODE(_n,p,_v)





