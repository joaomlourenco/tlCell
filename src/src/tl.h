#ifndef TL_H_
#define TL_H_

#include <limits.h>
#include <setjmp.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>

#if defined LIB_COMPILATION && defined HAVE_CONFIG_H
#  include <config.h>
#endif

#include "x86_sync_defns.h"

#ifdef USE_TX_HANDLER
#  include "tl_handler.h"
#endif

//--------------

//#define DISABLE_TXSTERILIZE_EXT
//#define DISABLE_STATS_COUNT
//#define ENABLE_NESTING
#define DISABLE_LOCAL_VARS

// -------------------------------
// Consider 4M alignment for LockTab so we can use large-page support.
// Alternately, we could mmap() the region with anonymous DZF pages.
//#define _TABSZ (1<<20)
#define _TABSZ (1<<20)

// maps variable address to lock address.
// For PW the mapping is simply (UNS(addr)+sizeof(int))
// COLOR attempts to place the lock(metadata) and the data on
// different D$ indexes.
#define TABMSK        (_TABSZ-1)

// #define COLOR         0
// #define COLOR         (256-16)
#define COLOR         (128)

// Alternate experimental mapping functions ....
// #define PSLOCK(a)     (LockTab + 0)                                   // PS1
// #define PSLOCK(a)     (LockTab + ((UNS(a) >> 2) & 0x1FF))             // S512
// #define PSLOCK(a)     (LockTab + (((UNS(a) >> 2) & (TABMSK & ~0x7)))) // PS1M
// #define PSLOCK(a)     (LockTab + (((UNS(a) >> 6) & (TABMSK & ~0x7)))) // PS1M
// #define PSLOCK(a)     (LockTab + ((((UNS(a) >> 2)|0xF) & TABMSK)))    // PS1M
// #define PSLOCK(a)     (LockTab + (-(UNS(a) >> 2) & TABMSK))
// #define PSLOCK(a)     (LockTab + ((UNS(a) >> 6) & TABMSK))
// #define PSLOCK(a)     (LockTab + ((UNS(a) >> 2) & TABMSK))            // PS1M
//ORIG #define PSLOCK(a)      ((LockTab + (((UNS(a)+COLOR) >> PSSHIFT) & TABMSK))   // PS1M

// ILP32 vs LP64.  PSSHIFT == Log2(sizeof(intptr_t)).
//#define PSSHIFT        ((sizeof(void *) == 4) ? 2 : 3)
//#define PSLOCK(a)      (vwLock*)(LockTab + ((UNS(a) >> PSSHIFT) & TABMSK))   // PS1M

//log2 word size in bytes
#define WORD_SIZE_LOG2  ((sizeof(void *) == 4) ? 2 : 3)
//word size in bytes
#define WORD_SIZE        (1<<WORD_SIZE_LOG2)
#define WORD_MASK        (WORD_SIZE-1)

//log2 line size in bytes
#ifndef LINE_SIZE_LOG2
#define LINE_SIZE_LOG2   WORD_SIZE_LOG2
//#define LINE_SIZE_LOG2   (5)
#endif
//line size in bytes
#define LINE_SIZE        (uintptr_t)(1<<LINE_SIZE_LOG2)
#define LINE_BASE_MASK        (~(LINE_SIZE-1))
#define LINE_OFFSET_MASK        (LINE_SIZE-1)
#define LINE_BASE(a) (intptr_t*)(UNS(a)&LINE_BASE_MASK)
#define LINE_OFFSET(a) (UNS(a)&LINE_OFFSET_MASK)

#ifndef OBJ_STM_PO
#define PSLOCK(a)      (vwLock*)(LockTab + (((UNS(a)+COLOR) >> LINE_SIZE_LOG2) & TABMSK))
#else
#define PSLOCK(a)      (vwLock*)(a)
#endif

#define WORDS_PER_LINE (1<<(LINE_SIZE_LOG2-WORD_SIZE_LOG2))


typedef long long time_u64;

enum
{ LJ_ABORT = 20 };

typedef enum
{
	TIDLE = 0,	// Non-transactional
	TTXN = 1,	// Transactional mode
	//TABORTING       = 3,        // aborting - abort pending
	TABORTED = 5	// defunct - moribund txn
	//TULTIMATE
} Modes;

typedef enum
{
	LOCKBIT = 1,
	NULLVER = (int)0xFFFFFFF0,	// CONSIDER 0x1
	NADA
} ManifestContants;

typedef int BitMap;


// maximum read-set/write-set size
//#define MAX_LIST_SIZE 1024
//#define MAX_LIST_SIZE 8192 //ricardo dias
#define MAX_LIST_SIZE 16384	//ricardo dias

// Max object size in words (4/8bytes)
#define MAX_OBJ_SIZE 16

// -------------------------------------------




typedef unsigned char byte;

typedef struct _AVPair
{	// read-set and write-set log entry
	//    struct _AVPair * Next ;      // Next element in log
	//    struct _AVPair * Prev ;      // Prev element in log

	intptr_t volatile *Addr;	// (Address,Value) pair
	//    vwLock volatile * LockFor ;  // LockFor points to the vwLock covering Addr
	int ObjSize;
	vwLock rdv;	// read-version @ time of 1st read - observed
	byte Held;
	// On CMT: Held=1 means lock is held.
	// On ENC: Held=1 means value is on the undo log.
	struct _Thread *Owner;
#ifdef ENABLE_NESTING
	unsigned int TxId;
#endif
	//    intptr_t Valu ;
	//intptr_t Valu[WORDS_PER_LINE];
	//void * ObjValu;
	intptr_t ObjValu[MAX_OBJ_SIZE];
	unsigned int useless;
	//    intptr_t Valu[WORDS_PER_LINE]   __attribute__ ((aligned (LINE_SIZE)));
	//    intptr_t *Valu;
} AVPair __attribute__((aligned(128)));

typedef struct _Log
{
	int CurrentList __attribute__((aligned(128)));
	AVPair *Put;	// Insert position - cursor
	AVPair *List __attribute__((aligned(128)));
	BitMap BloomFilter;	// Address exclusion fast-path test
}
	Log __attribute__((aligned(128)));



typedef struct _Tx
{
	Log rdSet;
	Log wrSet;

#ifndef DISABLE_LOCAL_VARS
	Log LocalUndo;
#endif

	int volatile Mode;

	sigjmp_buf OnFailure;

	struct _Tx *upperTx;
	struct _Tx *lowerTx;

#ifdef ENABLE_NESTING
	unsigned int TxId;
#endif

#ifdef USE_TX_HANDLER
	//ThreadEnv *env;
	ThreadHandlers *ths;
#endif
}
	Tx;



typedef struct _Thread
{
	//char padA[32];
	int UniqID;
	int volatile Retries;

	vwLock volatile rv;

	//        vwLock wv ;
	//vwLock abv ;
	int IsRO;
	//    int Aborts ;                    // Tally of # of aborts
	int rng;
	//    int xorrng [1] ;
	//    int RSovf ;
	//    int WSovf ;
	//    int Color ;
	//    struct _LockRecord * LockList ;

	Tx Tx;
#ifdef ENABLE_NESTING

	Tx *currentTx;
#endif
	//    int wrSetSize;
	//    int rdSetSize;
	//    int LocalUndoSize;
	struct sigaction OldSigAction;

	unsigned int NextTxId;

#ifndef DISABLE_STATS_COUNT
	int stats[20];
#endif
	//    int TxST, TxLD ;
	//char padB[32];
	//#ifdef USE_TX_HANDLER
	//            ThreadHandlers *ths;
	//#endif
}
	Thread;

enum _estats
{ _tl_num_loads, _tl_num_vfys, _tl_num_stores, _tl_num_rs_ovf,
  _tl_num_ws_ovf, _tl_num_ok_commits, _tl_num_user_aborts,
  _tl_num_ld_aborts, _tl_num_vfy_aborts, _tl_num_st_aborts,
  _tl_num_segfault_aborts, _tl_num_cmt_aborts,
  _tl_num_total_aborts };




// =====================> Generic Infrastructure

#define DIM(A)      (sizeof(A)/sizeof((A)[0]))
#define UNS(a)      ((uintptr_t)(a))
#define CTASSERT(x) { int tag[1-(2*!(x))]; tag[0]=0; printf ("Tag @%X\n", (int)tag[0]); }
//#define TRACE(nom)  (0)

// Bloom Filter
#define FILTERHASH(a)   ((UNS(a) >> 2) ^ (UNS(a) >> 5))
#define FILTERBITS(a)   (1 << (FILTERHASH(a) & 0x1F))

#define HIGH_32BITS 0xFFFFFFFF00000000ULL
#define LOW_32BITS  0x00000000FFFFFFFFULL
#define IS_LOCKED(x) ((x & LOCKBIT))
#define IS_VERSION(x) (!IS_LOCKED(x))

#define ADDR_2_VWLOCK(a) (UNS(a)|LOCKBIT)
#define VWLOCK_2_ADDR(l) {ASSERT_DEBUG(IS_LOCKED(l)); l^LOCKBIT;}

// Load Store utility macros for Addresses, Values and Fields
#define TXLDA(a)      TxLoad   (Self, (intptr_t *)(a))
#define TXSTA(a,v)    TxStore  (Self, (intptr_t *)(a),(v))
#define TXLDV(a)      TxLoad   (Self, (intptr_t *) &(a))
#define TXSTV(a,v)    TxStore  (Self, (intptr_t *) &(a), (v))
#define TXLDF(o,f)    TxLoad   (Self, (intptr_t *)(&((o)->f)))
#define TXSTF(o,f,v)  TxStore  (Self, (intptr_t *)(&((o)->f)), (intptr_t)(v))



#ifdef ENABLE_NESTING
#define GET_TX(_s) ((_s)->currentTx)
#else
#define GET_TX(_s) (&((*_s).Tx))
#endif


#ifdef USE_TX_HANDLER
#define GET_HANDLERS(_s) ((_s)->Tx.ths)
//#define GET_TX_ENV(_s) ((_s)->Tx.env)
#endif /*USE_TX_HANDLER */


//--------------

#ifndef ENABLE_NESTING
// ---------------------
#define TxStart(_s,_f) do {				\
		/*_ctl_call_pre_start_handlers(_s);*/	\
		sigsetjmp(GET_TX(_s)->OnFailure,1);	\
		TxStart__(_s,_f);			\
	} while(0)

#elif defined ENABLE_NESTING
// ---------------------
// SetNextTx must be called only once (even if tx aborts with retry)
// setjmp must be called in the context of the TX (not within TXStart)
#define TxStart(_s,_f) do {						\
		/*_ctl_call_pre_start_handlers(_s);*/			\
		TxSetNext__(_s);					\
		sigsetjmp(GET_TX(_s)->OnFailure,1);			\
		TxStart__(_s,_f);					\
	} while(0)

#endif




#ifdef DISABLE_STATS_COUNT
#define STATS_INC(x)
#else
#define STATS_INC(x) Self->stats[x]++
#endif




// --------------------------------------

// Tx control
extern Thread *TxNewThread();
extern void TxStart__(Thread * const Self, int ROFlag);
extern void TxStoreLocal(Thread * const Self, intptr_t volatile *Addr,
			 intptr_t Valu);
extern int TxValid(Thread * const Self);
extern int TxValidateAndAbort(Thread * const Self);
//intptr_t TxLoad          (Thread * const Self, intptr_t volatile * Addr);
//void     TxStore         (Thread * const Self, intptr_t volatile * addr, intptr_t valu);
//int      TxCommit        (Thread * const Self) ;
//void     TxAbort         (Thread * const Self, int retry) ;
extern intptr_t TxLoadCMT(Thread * const Self, intptr_t volatile *Addr);
extern void TxStoreCMT(Thread * const Self, intptr_t volatile *addr,
		       intptr_t valu);
extern int TxCommitCMT(Thread * const Self);
extern void TxAbortCMT(Thread * const Self, int retry);
extern intptr_t TxLoadENC(Thread * const Self, intptr_t volatile *Addr);
extern void TxStoreENC(Thread * const Self, intptr_t volatile *addr,
		       intptr_t valu);
extern int TxCommitENC(Thread * const Self);
extern void TxAbortENC(Thread * const Self, int retry);

extern int TxOpenReadENC(Thread * const Self, void volatile *Addr);
extern int TxOpenWriteENC(Thread * const Self, void volatile *addr,
			  unsigned int size);
extern int TxVerifyAddrENC(Thread * const Self, void volatile *Addr);

extern void TxSterilize(Thread * const Self, void volatile *Base,
			size_t const Length);
extern void TxEndThread(Thread * const Self);
// Optionals
extern char *TxDescribe();
extern void TxShutdownStats();
// Internals
extern void TxSetNext__(Thread * const Self);

#define TxReadField(_s,_o,f) ({				\
			__typeof__(_o->f) _v = _o->f;	\
			TxVerifyAddrENC(_s, _o);	\
			_v;})


// -----------------------------
// ENC OR CMT MODE
#ifdef MODE_ENC
#define TxCommit TxCommitENC
#define TxAbort TxAbortENC
#define TxLoad TxLoadENC
#define TxStore TxStoreENC
#define TxOpenRead TxOpenReadENC
#define TxVerifyAddr TxVerifyAddrENC
#define TxOpenWrite TxOpenWriteENC
#else //CMT MODE
#define TxCommit TxCommitCMT
#define TxAbort TxAbortCMT
#define TxLoad TxLoadCMT
#define TxStore TxStoreCMT
#define TxOpenRead TxOpenReadCMT
#define TxVerifyAddr TxVerifyAddrCMT
#define TxOpenWrite TxOpenWriteCMT
#endif

#ifdef USE_TX_HANDLER
#ifndef __TX_MALLOC_WRAPPER__
#define __TX_MALLOC_WRAPPER__
void ___tx_free_pointer__(Thread *Self, void *pointer);

#define TxMalloc(Self,size) ({void *___pointer234__ = malloc(size); \
	_ctl_register_pre_abort_handler((Self),___tx_free_pointer__, ___pointer234__);\
	___pointer234__;})

#define TxFree(Self,pointer) _ctl_register_pos_commit_handler((Self),___tx_free_pointer__,(pointer))

#endif /*__TX_MALLOC_WRAPPER__*/
#endif /*USE_TX_HANDLER*/

extern int IsOpenR(Thread * Self, void *Addr);
extern int IsOpenW(Thread * Self, void *Addr);

#endif /* TL_H_ */
