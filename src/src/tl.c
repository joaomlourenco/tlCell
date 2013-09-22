#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/unistd.h>
#include <sys/utsname.h>
#include <libgen.h>


//#include <thread.h>

#include <config.h>

#include "tl.h"
#include "ctl_trace.c"
//#define MULTIBUFFER
//#define CPUCELL
#include <libspe2.h>


static inline vwLock AdjustClock(vwLock volatile *const addr,
				 const int dx);
static inline Tx *CreateTx(Thread * const Self, Tx * tx);
static inline void AbortBackOffTx(Thread * const Self);
static inline AVPair volatile *GetAVPairOf(vwLock const v);
static inline Thread volatile *OwnerOf(vwLock const v);
static inline int ReadSetCoherent(Thread * const Self, int segf);
static inline Thread *GetThread();
//static inline int AbortInternalENC(Thread * const Self, Tx * tx);
//static inline int AbortInternalCMT(Thread * const Self, Tx * tx);
static inline AVPair *MakeList(Thread * const Self, int const sz);
static inline void AppendList(Log * dst, Log * src, int sz);
static inline void AppendListWRENC(Tx * tx, Log * dst, Log * src, int sz);
static inline void CleanupTx(Thread * const Self);


static inline void TrackLoad(Thread * const Self, intptr_t volatile *addr);
static inline AVPair *FindFirstAddr(Log * k, intptr_t volatile *addr);
static inline AVPair *FindFirstLock(Log * k, intptr_t volatile *addr);

#ifndef DISABLE_LOCAL_VARS
static inline void UndoLOCAL(Log * k);
#endif



//#define ENABLE_ENTHROPY
#ifdef ENABLE_ENTHROPY
#warning enabling enthropy

void spin_wait(int num_clocks)
{
	_u64 clock_start = RDTICK();
	_u64 clock_read = 0;
	while ((clock_read = RDTICK()) < clock_start + num_clocks)
	{
		//assert(clock_read>=clock_start);
	}
}

static unsigned int _r_seed;
static int ent_yield_probability = 10;
static int ent_sleep_probability = 10;
static unsigned int ent_max_sleep_time = 20000;	//usec  //must be < 40 000 000 otherwise it overflows

static inline void noise_gen()
{
	int rnd1, rnd2;
	rnd1 = (int)(100.0 * (random() / (RAND_MAX + 1.0)));
	assert(rnd1 >= 0 && rnd1 < 100);

	if (rnd1 < ent_yield_probability)
	{
		sched_yield();
	} else if (rnd1 < ent_yield_probability + ent_sleep_probability)
	{
		rnd2 = (int)(100.0 * (random() / (RAND_MAX + 1.0)));
		assert(rnd2 >= 0 && rnd2 < 100);
		//usleep((ent_max_sleep_time*rnd2)/100);
		spin_wait((ent_max_sleep_time * rnd2) / 100);
	}
}

#define LDLOCK(a) ({\
	noise_gen();\
	__LDLOCK(a);\
})

#define STLOCK(l,n) do { \
	noise_gen();\
	__STLOCK(l,n);\
}while(0)


#define CAS_VWLOCK(a,o,n) ({\
	noise_gen();\
	__CAS_VWLOCK(a,o,n);\
})

#define WORD_COPY(d,s) do {\
	noise_gen();\
	d=s;\
}while(0)

//      ASSERT_DEBUG((uintptr_t)s==(uintptr_t)LINE_BASE(s));
//      ASSERT_DEBUG((uintptr_t)d==(uintptr_t)LINE_BASE(d));

#define LINE_COPY(d,s) do {\
	unsigned int i;\
	ASSERT_DEBUG(WORDS_PER_LINE<=MAX_OBJ_SIZE);\
	for(i=0;i<WORDS_PER_LINE;i++){\
		noise_gen();\
		*(((intptr_t*)d)+i)=*(((intptr_t*)s)+i);\
	}\
}while(0)

#define BYTES_COPY(d,s,c) do {\
	unsigned int i;\
	ASSERT_DEBUG(c>0 && c<=MAX_OBJ_SIZE*sizeof(intptr_t));\
	noise_gen();\
	for(i=0;i<c;i++){\
		*(((char*)d)+i)=*(((char*)s)+i);\
	}\
}while(0)
//      ASSERT_DEBUG(d==->ObjValu || s==p->ObjValu);
//      ASSERT_DEBUG(p->ObjSize==c);


#else
// ----------------------------------------
// WITHOUT NOISE GENERATION
// ----------------------------------------
#define LDLOCK(a) __LDLOCK(a)
#define STLOCK(l,n) __STLOCK(l,n)
#define CAS_VWLOCK(a,o,n) __CAS_VWLOCK(a,o,n)

#define WORD_COPY(d,s) do {\
	d=s;\
}while(0)

//      ASSERT_DEBUG((uintptr_t)s==(uintptr_t)LINE_BASE(s));
//      ASSERT_DEBUG((uintptr_t)d==(uintptr_t)LINE_BASE(d));

#define LINE_COPY(d,s) do {\
	int i;\
	ASSERT_DEBUG(WORDS_PER_LINE<=MAX_OBJ_SIZE);\
	for(i=0;i<WORDS_PER_LINE;i++){\
		*(((intptr_t*)d)+i)=*(((intptr_t*)s)+i);\
	}\
}while(0)

#define BYTES_COPY(d,s,c) do {\
	int i;\
	ASSERT_DEBUG(c>0 && c<=MAX_OBJ_SIZE*sizeof(intptr_t));\
	for(i=0;i<c;i++){\
		*(((char*)d)+i)=*(((char*)s)+i);\
	}\
}while(0)
//      ASSERT_DEBUG(d==p->ObjValu || s==p->ObjValu);
//      ASSERT_DEBUG(p->ObjSize==c);

#endif

static const int CachePad = 64;


// =====================> Platform-specific bindings

// PAUSE() - MP-polite spinning
// Ideally we'd like to drop the priority of our CMT strand.
// consider:  wr %g0,%asi | MEMBAR #sync | MEMBAR #halt
//#define PAUSE() (usleep(1))
//#define PAUSE() (sched_yield())
#define PAUSE()

// We use a degenerate Bloom filter with only one hash function generating
// a single bit.  A traditional Bloom filter use multiple hash functions and
// multiple bits.  Relatedly, the size our filter is small, so it can saturate
// and become useless with a rather small write-set.
// A better solution might be small per-thread hash tables keyed by address that
// point into the write-set.

//#define IS_LOG_FULL(k) (((k)->Put->Next==NULL) ? 1 : 0)
#define IS_LOG_FULL(k) ((k)->Put >= (k)->List+MAX_LIST_SIZE)

// =====================>  TL/TL2 Infrastructure



// Consider 4M alignment for LockTab so we can use large-page support.
// Alternately, we could mmap() the region with anonymous DZF pages.
//static vwLock volatile LockTab [_TABSZ];             // PS : PS1M
static vwLock volatile LockTab[_TABSZ] __attribute__ ((aligned(8)));	// PS : PS1M
//static vwLock volatile * LockTab = NULL;             // PS : PS1M

/* Key for the thread-specific buffer */
static pthread_key_t thread_ptr_key;
static pthread_once_t tx_init_once = { PTHREAD_ONCE_INIT };

static intptr_t volatile ThreadUniqID = 0;	// Thread sequence #

#include "logger.c"
#include "gvclock.c"

#ifdef MODE_ENC
#  include "tl_enc.c"
#else
#  include "tl_cmt.c"
#endif

//#ifdef USE_TX_HANDLER
//#include "tl_handler.c"
//#endif


// ---------------------------------------------------------------------------

//static inline AVPair * log_get_next_put(Log * l){
//      int size = (ALIST_INIT * (1<<(l->CurrentList*ALIST_STEP)));
//      printf("currentlist=%d, size=%d\n", l->CurrentList, size);
//      AVPairs * alist_end = l->AList[l->CurrentList]+size;
//      assert(l->Put<alist_end);
//      if(l->Put+1==alist_end){
//              return l->AList[l->CurrentList+1];
//      }else{
//              return l->Put+1;
//      }
//}

// ---------------------------------------------------------------------------
// Atomic Fetch-and-Add()
static inline intptr_t AdjustPtr(intptr_t volatile *const addr,
				 const int dx)
{
	intptr_t v;
	for (v = *addr; CASPO(addr, v, v + dx) != v; v = *addr)
		;
	return v;
}

// Atomic Fetch-and-Add()
static inline vwLock AdjustClock(vwLock volatile *const addr, const int dx)
{
	vwLock v;
	for (v = *addr; CAS_VWLOCK(addr, v, v + dx) != v; v = *addr)
		;
	return v;
}

/*
   static inline int AdjustFF (int volatile * const addr, int const dx) {
   int v = *addr ;
   for (;;) {
   int vfy = CASIO (addr, v, v+dx) ;
   if (vfy == v)
   return v ;
   v = vfy ;
   // Feed-forward value from failing CAS to next iteration
   }
   }
 */

// Simplistlic low-quality Marsaglia SHIFT-XOR RNG.
// Bijective except for the trailing mask operation.
static inline int MarsagliaXORV(int x)
{
	if (x == 0)
		x = 1;
	x ^= x << 6;
	x ^= ((unsigned)x) >> 21;
	x ^= x << 7;
	return x;	// use either x or x & 0x7FFFFFFF
}

static inline int MarsagliaXOR(int *const seed)
{
	int x = MarsagliaXORV(*seed);
	*seed = x;
	return x & 0x7FFFFFFF;
}

static inline int TSRandom(Thread * const Self)
{
	return MarsagliaXOR(&Self->rng);
}

//-------------------------------------------

#ifndef DISABLE_LOCAL_VARS
// Transactional thread local variables are written in-place
// Saves the value of the previous value of the variable on the local-undo-log.
static inline void RecordStoreLOCAL(Log * k, intptr_t volatile *Addr,
				    intptr_t Valu)
{
	AVPair *p = k->Put;
	k->Put = p + 1;
	p->Addr = Addr;
	//LINE_COPY(p->ObjValu, &Valu);
	WORD_COPY(p->ObjValu[0], Valu);

}	//RecordStoreLOCAL

static inline void UndoLOCAL(Log * k)
{
	AVPair *p;
	for (p = k->Put - 1; p != NULL; p--)
	{
		ASSERT_DEBUG(p->Addr != NULL);
		//        LINE_COPY(p->Addr, p->ObjValu);
		WORD_COPY(*p->Addr, p->ObjValu[0]);

		p->Addr = NULL;	// diagnostic hygiene
	}
	k->Put = k->List;
}	//UndoLOCAL
#endif
//-------------------------------------------

static void SegFaultHandler(int sig)
{
	Thread *const Self = GetThread();
	TraceSIGSEGF(Self->UniqID, 0, NULL);
	if (!ReadSetCoherent(Self, 1))
	{
		TRACE_DEBUG
			("%d - segfault and RS is not coherent -> Aborting with retry!\n",
			 Self->UniqID);
		STATS_INC(_tl_num_segfault_aborts);
		AbortBackOffTx(Self);
	} else
	{
		TRACE("ERROR: %d - SEGFAULT AND RS is coherent -> OOOPS!!!\n", Self->UniqID);
		abort();

	}
}


// Set the fault handler on the begining of the main tx
static void SetFaultHandler(Thread * const Self)
{
#ifdef ENABLE_NESTING
	ASSERT_DEBUG(GET_TX(Self) == NULL);
#else

	ASSERT_DEBUG(GET_TX(Self) != NULL);
	ASSERT_DEBUG(GET_TX(Self)->upperTx == NULL);

#endif

	struct sigaction act;
	act.sa_handler = SegFaultHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGSEGV, &act, &(Self->OldSigAction));
}

#ifdef ENABLE_NESTING
// SetNextTx must be called only once per Tx (even if it aborts with retry)
void TxSetNext__(Thread * const Self)
{
	if (GET_TX(Self) == NULL)
	{
		// Main tx
		GET_TX(Self) = &Self->Tx;
	} else
	{
		// Sub TX
		ASSERT_MSG_DEBUG(GET_TX(Self)->Mode == TTXN,
				 "sub tx has aborted and it is calling again setnexttx\n");
		if (GET_TX(Self)->lowerTx != NULL)
		{	// currentTx already has and allocated lower Tx
			GET_TX(Self) = GET_TX(Self)->lowerTx;
		} else
		{	// currentTx does not have and allocated lower Tx
			Tx *new;
			new = CreateTx(Self, NULL);
			GET_TX(Self)->lowerTx = new;
			new->upperTx = GET_TX(Self);
			new->lowerTx = NULL;
			GET_TX(Self) = new;
		}
	}
}	//SetNextTx__
#endif

/*
   static inline void RestoreFaultHandler(Thread * const Self) {
   #ifdef DISABLE_NESTING
   ASSERT_DEBUG(GET_TX(Self)!=NULL && GET_TX(Self)->upperTx==NULL);
   #else

   ASSERT_DEBUG(GET_TX(Self)==NULL);
   #endif

   sigaction(SIGSEGV, &(Self->OldSigAction), NULL);

   }
 */



//-------------------------------------------

static inline Thread *GetThread()
{
	Thread *thr = (Thread *) pthread_getspecific(thread_ptr_key);
	return thr;
}


static inline void RunOnce()
{
	//    LockTab=(vwLock*)malloc(sizeof(vwLock)*_TABSZ);
	//    int tmp;
	//    tmp=posix_memalign((void**)&LockTab,sizeof(vwLock),sizeof(vwLock)*_TABSZ);
	//    ASSERT_DEBUG(tmp==0);

	ASSERT_DEBUG(__alignof__(LockTab) >= (sizeof(intptr_t)));
	memset((void *)LockTab, 0, sizeof(vwLock) * _TABSZ);

	GVInit();
	ASSERT_DEBUG(__alignof__(_GCLOCK) >= (sizeof(intptr_t)));

	struct utsname un;
	uname(&un);

	// Create thread global variable
	if (pthread_key_create(&thread_ptr_key, NULL))
	{
		TRACE("ERROR: could not create pthread_key!\n");
		abort();
	}

	TraceInit();

#ifdef ENABLE_ENTHROPY

	_r_seed = RDTICK();
	srand(_r_seed);
#endif
}

#ifdef MODE_ENC
#define TxAbortInternal AbortInternalENC
#else
#define TxAbortInternal AbortInternalCMT
#endif


static inline void TrackLoad(Thread * const Self, intptr_t volatile *addr)
{
	Log *k = &GET_TX(Self)->rdSet;
	ASSERT_DEBUG(!IS_LOG_FULL(k));

	k->Put->Addr = addr;
	k->Put++;

	// rdv and Valu fields are undefined for tracked loads

}	//TrackLoad

// Resets thread state for TxStart
static inline void ResetTx(Thread * const Self)
{
	// Reset to ground state
	GET_TX(Self)->Mode = TIDLE;
	GET_TX(Self)->wrSet.BloomFilter = 0;
	GET_TX(Self)->wrSet.Put = GET_TX(Self)->wrSet.List;
	GET_TX(Self)->rdSet.Put = GET_TX(Self)->rdSet.List;
#ifndef DISABLE_LOCAL_VARS

	GET_TX(Self)->LocalUndo.Put = GET_TX(Self)->LocalUndo.List;
#endif
}

// Resets thread state for returning after transaction ends (TxCommit/TxAbort)
// can't be used for retry aborts because *ovf and Retries counters are reset
static inline void CleanupTx(Thread * const Self)
{
	//TxReset (Self) ;
	GET_TX(Self)->Mode = TIDLE;
	Self->Retries = 0;
	//    Self->RSovf = 0 ;
	//    Self->WSovf = 0 ;

#ifdef ENABLE_NESTING

	GET_TX(Self) = GET_TX(Self)->upperTx;
#endif

#ifdef USE_TX_HANDLER
	//_ctl_clean_thread_env(GET_TX_ENV(Self));
	_ctl_clean_thread_handlers(GET_HANDLERS(Self));
#endif

}


// Our mechanism admits mutual abort with no progress - livelock.
// Consider the following scenario where T1 and T2 execute concurrently:
// Thread T1:  WriteLock A; Read B LockWord; detect locked, abort, retry
// Thread T2:  WriteLock B; Read A LockWord; detect locked, abort, retry
//
// Possible solutions:
//
// * try backoff (random and/or exponential), with some mixture
//   of yield or spinning.
//
// * Use a form of deadlock detection and arbitration.
//
// In practice it's likely that a semi-random semi-exponential back-off
// would be best.
//

// Internal call to abort - with backoff time and going to retry
// Cascade abort all upper TXs
// This is called when the transaction is no longer consistent:
// We may reach this if either :
// - the RS is not coherent.
// - TxLoad detected an incosistent read:
//   NOTE: 1) TxLoad doesn't call trackload in this case;
//   NOTE: 2) RO transactions don't ever call TrackLoad
// - SEGFAULT trying to load a free'd value
// - RS/WS overflow
// - TryFastUpdate failed
static inline void AbortBackOffTx(Thread * const Self)
{
	// Beware: back-off is useful for highly contended environments
	// where N threads shows negative scalability over 1 thread.
	// Extreme back-off restricts parallelism and, in the extreme,
	// is tantamount to allowing the N parallel threads to run serially
	// 1 at-a-time in succession.
	//
	// Consider: make the back-off duration a function of:
	// + a random #
	// + the # of previous retries
	// + the size of the previous read-set
	// + the size of the previous write-set
	//
	// Consider using true CSMA-CD MAC style random exponential backoff.

	ASSERT_DEBUG(GET_TX(Self) != NULL);
	ASSERT_DEBUG(GET_TX(Self)->Mode == TTXN);

#ifdef USE_TX_HANDLER
	_ctl_call_pre_abort_handlers(Self);
	_ctl_clean_thread_handlers(GET_HANDLERS(Self));
#endif

	// cascade abort until we reach upper tx
	Tx *tx = GET_TX(Self);
	while (tx != NULL)
	{
		TxAbortInternal(Self, tx);
		tx = tx->upperTx;

	}
#ifdef ENABLE_NESTING
	GET_TX(Self) = &(Self->Tx);
#endif



	ASSERT_DEBUG(GET_TX(Self)->Mode == TABORTED);
	Self->Retries++;

	// BACKOFF
	int minRetries = 3;	// TUNABLE
	if (Self->Retries > minRetries)
	{
		int stall = TSRandom(Self) & 0xF;
		stall += Self->Retries >> 2;

		TRACE_DEBUG("%d: BackOff: retires = %d, stall = %d\n",
			    Self->UniqID, Self->Retries, stall);
		int i;
		for (i = 0; i < stall; i++)
		{
			TRACE_DEBUG("%d: BackOff: %d/%d\n", Self->UniqID,
				    i, stall);
			sched_yield();
		}
	}

	siglongjmp(GET_TX(Self)->OnFailure, LJ_ABORT);

}	//TxAbortBackOff

// Remarks on deadlock:
// Indefinite spinning in the lock acquisition phase admits deadlock.
// We can avoid deadlock by any of the following means:
//
// 1. Bounded spin with back-off and retry.
//    If the we fail to acquire the lock within the interval we drop all
//    held locks, delay (back-off - either random or exponential), and retry
//    the entire txn.
//
// 2. Deadlock detection - detect and recover.
//    Use a simple waits-for graph to detect deadlock.  We can recovery
//    either by aborting *all* the participant threads, or we can arbitrate.
//    One thread becomes the winner and is allowed to proceed or continue
//    spinning.  All other threads are losers and must abort and retry.
//
// 3. Prevent or avoid deadlock by acquiring locks in some order.
//    Address order using LockFor as the key is the most natural.
//    Unfortunately this requires sorting -- See the LockRecord structure.
//
static inline AVPair volatile *GetAVPairOf(vwLock const v)
{
	if (IS_LOCKED(v))
	{
		AVPair volatile *avp;
		ASSERT_DEBUG(IS_LOCK64_OK(v));
		vwLock tmpvwl = v ^ LOCKBIT;
		avp = (AVPair *) (uintptr_t) (tmpvwl);
		return avp;
	} else
	{
		return NULL;
	}
}

static inline Thread volatile *OwnerOf(vwLock const v)
{
	AVPair volatile *avp = GetAVPairOf(v);
	if (avp != NULL)
	{
		return avp->Owner;
	} else
	{
		return NULL;
	}
}

// Verify readset consistency
static inline int ReadSetCoherent(Thread * const Self, int segf)
{
	//printf("ReadSetCoherent (tl.c) -Value of SELF->TX->Mode = %d\n", GET_TX(Self)->Mode);
	ASSERT_MSG(GET_TX(Self) != NULL
		   && GET_TX(Self)->Mode == TTXN,
		   "ReadSetCoherent called outside an active TX\n");

	TraceRSCoherent(Self->UniqID, 0, NULL, (intptr_t) 0);
	//printf("ReadSetCoherent (tl.c) -> 2\n");
	ASSERT_DEBUG(IS_VERSION(Self->rv));
	//printf("ReadSetCoherent (tl.c) -> 3\n");
#ifdef MODE_ENC

	MEMBARLDLD();
#endif

	Log *const rd = &GET_TX(Self)->rdSet;
	AVPair *p;
	vwLock v = 0;
	//printf("ReadSetCoherent (tl.c) 4-> a percorrer lista de apontadores- prepare for SIG_SEG_V  *AVPair \n");
	for (p = rd->List; p != rd->Put; p++)
	{
		//printf("E vai 1 - p->Addr = %p \n" , p->Addr);

		v = LDLOCK(PSLOCK(p->Addr));
		//printf("E vai 2 - \n");
		//TraceRSCoherent(Self->UniqID, 20, p->Addr, (intptr_t)v);
		if (IS_LOCKED(v))
		{
			//printf("E vai 3 - \n");
			if (OwnerOf(v) != Self)
			{
				//printf("E vai 4 - \n");
				TraceRSCoherent(Self->UniqID, 40, p->Addr, (intptr_t) v);
				return 0;
			}
			if (GetAVPairOf(v)->rdv > Self->rv) { // This is necessary in case we have lock colision
				//printf("E vai 5 - \n");
				TraceRSCoherent(Self->UniqID, 50, p->Addr, (intptr_t) v);
				return 0;
			}
		} else{

			if (v > Self->rv)
			{
				//printf("E vai 6 - \n");
				TraceRSCoherent(Self->UniqID, 60, p->Addr, (intptr_t) v);
				return 0;
			}
		}
	}
	TraceRSCoherent(Self->UniqID, 70, NULL, (intptr_t) 0);

	ASSERT_MSG(!segf, "%d: v=%lu, Self->rv=%lu\n", Self->UniqID, v, (intptr_t) Self->rv);
	//printf("ReadSetCoherent (tl.c) ->  before returning \n");
	return 1;
}


static inline Tx *CreateTx(Thread * const Self, Tx * tx)
{
	Tx *new;
	if (tx == NULL)
	{
		new = (Tx *) malloc(sizeof(Tx));
	} else
	{
		new = tx;
	}
	new->wrSet.List = MakeList(Self, MAX_LIST_SIZE);
	new->rdSet.List = MakeList(Self, MAX_LIST_SIZE);
#ifndef DISABLE_LOCAL_VARS

	new->LocalUndo.List = MakeList(Self, MAX_LIST_SIZE);
#endif

	new->lowerTx = NULL;
	new->upperTx = NULL;
	new->Mode = TIDLE;

#ifdef USE_TX_HANDLER
	//#define PARAM_SIZE 20
	//new->env = _ctl_new_thread_env(PARAM_SIZE);
	new->ths = _ctl_new_thread_handlers();
#endif
	return new;
}


Thread *TxNewThread()
{

	pthread_once(&tx_init_once, RunOnce);
	ASSERT_MSG_DEBUG(GetThread() == NULL,
			 "ERROR: there is already an open thread");

	Thread *t = (Thread *) malloc(sizeof(*t));
	memset(t, 0, sizeof(*t));
	int id = AdjustPtr(&ThreadUniqID, 1);
	t->UniqID = id;
	//    t->xorrng[0] = t->rng = (RDTICK() ^ id) | 1 ;
	t->rng = (RDTICK() ^ id) | 1;

	CreateTx(t, &t->Tx);

#ifdef ENABLE_NESTING

	t->currentTx = NULL;

	//#else
	//
	//    t->currentTx = &t->Tx;
	//
#endif

	t->NextTxId = 1;

#ifndef DISABLE_STATS_COUNT
	memset(t->stats, 0, sizeof(t->stats));
#endif

	//#ifdef USE_TX_HANDLER
	// t->ths = _ctl_new_thread_handlers();
	//#endif

	pthread_setspecific(thread_ptr_key, t);

	SetFaultHandler(t);

	return t;
}	//TxNewThread

void TxEndThread(Thread * const Self)
{
	AVPair *this_chunk, *next_chunk;
	//        PrintAllTraces();

#ifdef ENABLE_NESTING

	ASSERT_MSG_DEBUG(GET_TX(Self) == NULL,
			 "TxEndThread must be called without any active transactions\n");
#endif



	Tx *t = &Self->Tx;
	while (t != NULL)
	{
		next_chunk = this_chunk = t->wrSet.List;

		//        AVPair * avptr;
		//        for(avptr = t->rdSet.List ; avptr!=NULL ; avptr=avptr->Next) {
		//            ASSERT_DEBUG(avptr->Held==0);
		//            ASSERT_DEBUG(avptr->Owner==Self);
		//        }
		//
		//              void * start_addr = t->rdSet.List;
		//              void * end_addr = t->rdSet.List+Self->wrSetSize;
		//
		//              vwLock locki;
		//              for(locki=0; locki<_TABSZ;locki++){
		//                      ASSERT_DEBUG((LockTab[locki]&1)==0 || (LockTab[locki]<(vwLock)start_addr || LockTab[locki]>=(vwLock)end_addr));
		//              }
		//
		//        do {
		//            this_chunk=next_chunk;
		//            next_chunk += (Self->wrSetSize-1);
		//            next_chunk = next_chunk->Next;
		//            //free(this_chunk); DO NOT FREE THIS LIST!!! it may be referenced by other threads ????
		//        } while(next_chunk!=NULL)
		//            ;
		//
		//        next_chunk = this_chunk = t->rdSet.List;
		//        do {
		//            this_chunk=next_chunk;
		//            next_chunk += (Self->rdSetSize-1);
		//            next_chunk = next_chunk->Next;
		//            //            free(this_chunk);
		//        } while(next_chunk!=NULL)
		//            ;
		//
		//        next_chunk = this_chunk = t->LocalUndo.List;
		//        do {
		//            this_chunk=next_chunk;
		//            next_chunk += (Self->LocalUndoSize-1);
		//            next_chunk = next_chunk->Next;
		//            //            free(this_chunk);
		//        } while(next_chunk!=NULL)
		//            ;

		Tx *to_del = t;
		t = t->lowerTx;
		if (to_del->upperTx != NULL)
		{
			// the first tx is not a ptr, it's bundled inside struct Thread
			free(to_del);
		}
	}
#ifdef USE_TX_HANDLER
	//_ctl_delete_thread_env(GET_TX_ENV(Self));
	_ctl_delete_thread_handlers(GET_HANDLERS(Self));
#endif
	//RestoreFaultHandler(Self);
	//    free (Self);

	pthread_setspecific(thread_ptr_key, NULL);

	//rjfd
	free(Self->Tx.wrSet.List);
	free(Self->Tx.rdSet.List);
	free(Self);
	//rjfd

}


// Never call this directly -> use TxStart Macro
void TxStart__(Thread * const Self, int ROFlag)
{

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL &&
			 (GET_TX(Self)->Mode == TIDLE ||
			  GET_TX(Self)->Mode == TABORTED),
			 "TxStart__ can't be called directly. Use TxStart\n");
	ASSERT_MSG_DEBUG(GET_TX(Self)->Mode != TTXN
			 || ROFlag == Self->IsRO,
			 "ERROR: ROFlag must be consistent across nested transactions\n");


	ResetTx(Self);
	GET_TX(Self)->Mode = TTXN;
#ifdef ENABLE_NESTING

	GET_TX(Self)->TxId = Self->NextTxId++;
#endif

	//rjfd    //Self->rv   = GVRead () ;
	//      Only reads clock if is main Tx
	if (GET_TX(Self)->upperTx == NULL)
	{
		Self->rv = GVRead();
	}
#ifdef USE_TX_HANDLER
	else
	{
		//GET_TX(Self)->env = NULL;
		GET_TX(Self)->ths = NULL;
	}
#endif
	//rjfd

	ASSERT_DEBUG(IS_VERSION(Self->rv));

	MEMBARLDLD();	// ?????


	Self->IsRO = ROFlag;

	TraceTxStart(Self->UniqID, 0, Self->rv);

#ifndef DISABLE_LOCAL_VARS

	ASSERT_DEBUG(GET_TX(Self)->LocalUndo.Put ==
		     GET_TX(Self)->LocalUndo.List);
#endif

	ASSERT_DEBUG(GET_TX(Self)->wrSet.Put == GET_TX(Self)->wrSet.List);
	ASSERT_DEBUG(GET_TX(Self)->rdSet.Put == GET_TX(Self)->rdSet.List);
#ifdef USE_TX_HANDLER
	_ctl_call_pos_start_handlers(Self);
#endif

	return;
}	//TxStart

// User level call to TxAbort
void TxAbort(Thread * const Self, int retry)
{
	printf("Aborting Transaction.. and retrying");
	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxAbort must be called within an active transaction\n");

	STATS_INC(_tl_num_user_aborts);

	TxAbortInternal(Self, GET_TX(Self));
	if (retry)
	{
		siglongjmp(GET_TX(Self)->OnFailure, LJ_ABORT);
	} else
	{
#ifdef USE_TX_HANDLER
		_ctl_call_pos_abort_handlers(Self);
#endif
		CleanupTx(Self);
	}
}	//TxAbort

#ifndef DISABLE_LOCAL_VARS
// Store transactional thread local variable - update is made inplace
void TxStoreLocal(Thread * const Self, intptr_t volatile *Addr,
		  intptr_t Valu)
{
	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxStoreLocal must be called within an active transaction\n");

	// Update in-place, saving the original contents in the undo log
	RecordStoreLOCAL(&GET_TX(Self)->LocalUndo, Addr, *Addr);
	*Addr = Valu;
}
#endif

// Use TxSterilize() any time an object passes out of the transactional domain
// and will be accessed solely with normal non-transactional load and store
// operations. TxSterilize() allows latent pending txn STs to drain before allowing
// the object to escape.  This avoids use-after-free errors, for instance.
// Note that we need to know or track the length of the malloc()ed objects.
// In practice, however, most malloc() subsystems can compute the object length
// in a very efficient manner, so a simple extension to the mallo()-free()
// interface would suffice.
// Length is in number of words (intptr_t)
void TxSterilize(Thread * const Self, void volatile *Base,
		 size_t const Length)
{
#ifdef ENABLE_NESTING
	ASSERT_MSG_DEBUG(GET_TX(Self) == NULL,
			 "TxSterilize can't be called while the TX is active. It must be called after the TX commits\n");
#endif

	TraceTxSterilize(Self->UniqID, 0, Base);

	intptr_t volatile *Addr = (intptr_t *) Base;
	intptr_t volatile *End = Addr + Length;
	ASSERT_DEBUG(Addr <= End);
	while (Addr < End)
	{
		vwLock volatile *Lock = PSLOCK(Addr);

		// update lock version of the object to be free'd to invalidate further reads
		vwLock newGV = GVRead();
		ASSERT_DEBUG(IS_VERSION(newGV));

		do
		{
			vwLock old;
			vwLock cv;

			// we have to wait for threads that are locking this word to:
			// - finish locking
			// - verify their inconsistency
			// - put back the version number
			cv = LDLOCK(Lock);
			if (IS_LOCKED(cv))
			{	// it's still locked -> spin
				continue;
			} else
			{

#ifdef DISABLE_TXSTERILIZE_EXT
				break;
#endif

				// When the variable is not locked we update it's version number to
				// the current GV. This will enable txloads from other TXs to detect that there was
				// a change to the variable (it was removed) and provoke and abort on the TX.
				// Otherwise the other TXs might fail to see that variable was free'd and could return
				// the already free'd data to the user.
				// It may happen that some other thread updates the clock (PSLOCK collision)
				if (cv >= newGV)
				{
					break;
				}
				old = CAS_VWLOCK(Lock, cv, newGV);
				if (old == cv)
				{	// CAS was successfull
					ASSERT_DEBUG(IS_VERSION(old));
					break;
				}
				if (IS_VERSION(old) && old >= newGV)
				{
					// some othrer thread already updated the clock to a version >= than newGV
					break;
				}
			}

		} while (1);

		Addr++;
	}

}

// If TxValid() returns FALSE the caller is expected to unwind and restart the
// transactional attempt.  An alternative would be to call setjmp()
// in TxStart() and have TxValid() check and optionally longjmp() to
// restart and retry an operation that failed because of conflicts.
int TxValid(Thread * const Self)
{
	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL,
			 "TxValid must be called within an active Tx.\n");

	if (GET_TX(Self)->Mode == TABORTED)
	{
		return 0;
	} else
	{
#if (defined OBJ_STM && defined NO_VFY)
		if (!ReadSetCoherent(Self, 0))
		{
			return 0;
		} else
		{
			return 1;
		}
#else
		return 1;
#endif

	}
}

int TxValidateAndAbort(Thread * const Self)
{
	if (TxValid(Self))
	{
		return 1;
	} else
	{
		//ASSERT_MSG(0, "TxValidateAndAbort found not valid TX -> ABORT!\n");
		AbortBackOffTx(Self);
		return 0;
	}
}


void TxShutdownStats()
{
	TRACE("Shutdown: GCLOCK=%llX\n", LDLOCK(&_GCLOCK));
}

char *TxDescribe()
{
	static char buf[128];
	sprintf(buf, "A+%s", _GVFLAVOR);
	return buf;
}

// Allocate the primary list as a large chunk so we can guarantee
// ascending & adjacent addresses through the list.
// This improves D$ and DTLB behaviour.
static inline AVPair *MakeList(Thread * const Self, int const sz)
{
	ASSERT_DEBUG(sz > 0);
	// Use CachePad to reduce the odds of false-sharing.
	AVPair *ap = (AVPair *) malloc((sizeof(*ap) * sz) + CachePad);
	ASSERT(ap != NULL);
	memset(ap, 0, sizeof(*ap) * sz);
	int i;
	for (i = 0; i < sz; i++)
	{
		(ap + i)->Owner = Self;
	}
	return ap;
}

//AVPair * FindFirst (Log * k, vwLock volatile * Lock) {
static inline AVPair *FindFirstAddr(Log * k, intptr_t volatile *addr)
{
	AVPair *p;

	for (p = k->List; p != k->Put; p++)
	{
		if (p->Addr == addr)
			return p;
	}
	return NULL;
}

//AVPair * FindFirst (Log * k, vwLock volatile * Lock) {
static inline AVPair *FindFirstLock(Log * k, intptr_t volatile *addr)
{
	AVPair *p;

	vwLock volatile *Lock = PSLOCK(addr);

	for (p = k->List; p != k->Put; p++)
	{
		if (Lock == PSLOCK(p->Addr))	// keep in mind that several addresses can map to the same position  in LockTab
			return p;
	}
	return NULL;
}

static inline void AppendList(Log * dst, Log * src, int sz)
{
	AVPair *s, *d;
	for (s = src->List; s != src->Put; s++)
	{
		d = dst->Put;

		ASSERT_DEBUG(d < dst->List + MAX_LIST_SIZE);

		d->Addr = s->Addr;

#ifdef MODE_ENC

		if (s->ObjSize == 0)
		{
			LINE_COPY(d->ObjValu, s->ObjValu);
		} else
		{
			BYTES_COPY(d->ObjValu, s->ObjValu, s->ObjSize);
		}
		d->ObjSize = s->ObjSize;
#else

		WORD_COPY(d->ObjValu[0], s->ObjValu[0]);
#endif


		d->Held = s->Held;
		ASSERT_DEBUG(s->Held == 0);
		d->rdv = s->rdv;
		d->Owner = s->Owner;
		dst->Put = d + 1;
	}
	dst->BloomFilter |= src->BloomFilter;
}

// Append list for wr set of sub tx commit in encounter mode
// ARGS: ptr to sub tx descriptor, dst and src wr set, size of wr set chunk
static inline void AppendListWRENC(Tx * tx, Log * dst, Log * src, int sz)
{
#ifdef ENABLE_NESTING
	AVPair *s, *d;
	ASSERT_DEBUG(tx->upperTx != NULL);

	for (s = src->List; s != src->Put; s++)
	{
		d = dst->Put;

		ASSERT_DEBUG(d < dst->List + MAX_LIST_SIZE);

		//---------------
		ASSERT_DEBUG(s->Held);
		vwLock volatile *Lock = PSLOCK(s->Addr);
		ASSERT_DEBUG(IS_LOCKED(LDLOCK(Lock)));

		AVPair volatile *avp = GetAVPairOf(LDLOCK(Lock));
		ASSERT_DEBUG(avp != NULL);
		if (avp->TxId != tx->TxId)
		{
			// the lock was not acquired by child tx (was acquired by some parent tx)
			s->Held = 0;
			continue;
		} else
		{
			// the lock was acquired by child tx -> move lock owner to parent tx
			//---------------
			d->TxId = tx->upperTx->TxId;
			d->Addr = s->Addr;

#ifdef MODE_ENC

			if (s->ObjSize == 0)
			{
				LINE_COPY(d->ObjValu, s->ObjValu);
			} else
			{
				BYTES_COPY(d->ObjValu, s->ObjValu,
					   s->ObjSize);
			}
			d->ObjSize = s->ObjSize;
#else

			assert(0);
			WORD_COPY(d->ObjValu[0], s->ObjValu[0]);
#endif

			d->Held = s->Held;
			s->Held = 0;

			d->rdv = s->rdv;
			d->Owner = s->Owner;
			dst->Put = d + 1;

			//---------------
			STLOCK(Lock, ADDR_2_VWLOCK(d));
		}
	}
	dst->BloomFilter |= src->BloomFilter;
#else
	assert(0);
#endif
}	//AppendListWRENC





//-------------------------------------------------------------------

void CTAsserts()
{
	// Compile-time assertions -- establish critical invariants
	// It's always better to fail at compile-time than at run-time.
#ifdef CPU_X86
	// ILP32
	CTASSERT(sizeof(char) == 1);
	CTASSERT(sizeof(short) == 2);
	CTASSERT(sizeof(int) == 4);
	CTASSERT(sizeof(long) == 4);
	CTASSERT(sizeof(long long) == 8);
	CTASSERT(sizeof(intptr_t) == 4);
	CTASSERT(sizeof(void *) == 4);
#elif defined CPU_X86_64
	// LP64
	CTASSERT(sizeof(char) == 1);
	CTASSERT(sizeof(short) == 2);
	CTASSERT(sizeof(int) == 4);
	CTASSERT(sizeof(long) == 8);
	CTASSERT(sizeof(long long) == 8);
	CTASSERT(sizeof(intptr_t) == 8);
	CTASSERT(sizeof(void *) == 8);
#endif


#ifndef MODE_ENC

	CTASSERT(WORDS_PER_LINE == 1);
	CTASSERT(LINE_SIZE_LOG2 == WORD_SIZE_LOG2);
#endif

	CTASSERT(LINE_SIZE >= WORD_SIZE);

	// _TABSZ must be power-of-two
	CTASSERT((_TABSZ & (_TABSZ - 1)) == 0);
}

#ifdef ENABLE_NESTING
#warning ENABLE_NESTING is set
#endif
#ifdef DISABLE_TXSTERILIZE_EXT
#warning DISABLE_TXSTERILIZE_EXT is set
#endif
#ifdef DISABLE_STATS_COUNT
#warning DISABLE_STATS_COUNT is set
#endif


int IsOpenR(Thread * Self, void *Addr)
{
	Log *rd = &GET_TX(Self)->rdSet;
	AVPair *avp = FindFirstAddr(rd, Addr);
	return (avp != NULL || !ReadSetCoherent(Self, 0));
}

int IsOpenW(Thread * Self, void *Addr)
{
	Log *wr = &GET_TX(Self)->wrSet;
	AVPair *avp = FindFirstAddr(wr, Addr);
	return (avp != NULL || !ReadSetCoherent(Self, 0));
}

void ___tx_free_pointer__(Thread *Self, void *pointer) {
	free(pointer);
}

typedef struct ppu_pthread_data {
  spe_context_ptr_t context;
  unsigned int entry;
  void *argp;
  void *envp;
  Thread * thread;
  void *controlStruct;
} ppu_pthread_data_t;

typedef struct controlStruct{
	unsigned long long userStruct;
	unsigned long long txStruct;
	vwLock rv;
	unsigned long long lockTab; //huh...
	unsigned long long wrSet;
	unsigned long long rdSet;
	unsigned long long wrSetPut;
	unsigned long long rdSetPut;
	unsigned long long Owner;
	unsigned long long pointerToObjValu;
	unsigned long long CurrentListwrSet;
	unsigned long long CurrentListrdSet;
} controlStruct_t __attribute__((aligned(128)));


void PrintInfo(AVPair *pair){
	printf("Printing info on a AVPair...\n");
	printf("Address = %p \n", pair->Addr);
	printf("Owner is = %p \n", pair->Owner);
	printf("rdv is = %d \n", pair->rdv);
	printf("Value of object (if any) is = %d\n", (int) pair->ObjValu[0]);
	return;
}

#ifdef MULTIBUFFER
void *ppu_pthread_function_listener(void*arg) {
	spe_stop_info_t stop_info;
	ppu_pthread_data_t *data = (ppu_pthread_data_t *) arg;
	unsigned int aux; //address to look for in WriteSet
	unsigned long long returnAddr;
	unsigned long long addrLookup;
	AVPair *auxPair;
	int flag = 0;

	printf("Listener for SPE = %d  started..\n", data->context);

	while (flag == 0) {
		while (spe_out_mbox_status(data->context) == 0) {};
		printf("Detected Message\n");
		spe_out_mbox_read(data->context, &aux, 1);
		printf("Received address to find in WrSet. Addr= %i \n", aux);
		if (aux == 0) {
			printf("changing flag to 1\n");
			flag = 1;
		} //pthread_exit(NULL); //aux=0 -> Terminating transaction! TxCommit basically
		else {
			//SEARCH
			//static inline AVPair *FindFirstAddr(Log * k, intptr_t volatile *addr);
			aux=1;
			addrLookup = ((controlStruct_t *) (data->controlStruct))->pointerToObjValu;
			printf("Before FindFirstAddr - Find address = %llx\n", aux);
			auxPair =  FindFirstAddr(&(data->thread->Tx.wrSet), addrLookup);
			printf("After FindFirstAddr - Returning Address = %i\n", auxPair);
			if (auxPair == NULL)
				aux = 0;
			if (spe_in_mbox_write(data->context, aux, 1, SPE_MBOX_ANY_NONBLOCKING) == 0) {
				printf("Message could not be written\n");
			}
		}

	}
	//printf("Parece que sim-> estamos a começar threads\n");
	pthread_exit( NULL);
}
#endif


/*Non-blocking start of the spe execution*/
void *ppu_pthread_function(void *arg) {

	/* this variable is used to return data regarding an abnormal return from the SPE*/
	spe_stop_info_t stop_info;
	ppu_pthread_data_t *data = (ppu_pthread_data_t *) arg;

	//spe_context_run(context,&entry,&theStruct,NULL,NULL,NULL);
	if (spe_context_run(data->context, &data->entry, 0, data->argp, data->envp, &stop_info) < 0) {
		fprintf(stderr, "FAILED: spe_context_run(errno=%d strerror=%s)\n", errno, strerror(errno));
		fprintf("stop_info information=%s" , stop_info.stop_reason);
		exit(1);
	}
	pthread_exit(NULL);
}

//#define DEBUGON
/*Here will be TxStartSpe(spe_program_handle_t spuCode, void* pointerToUserStruct)
 * Notice each call for this fuction will start a new PPE thread in order for non-blocking execution of the spuCode
 */
void TxStartSPE(/*Thread * const Self, int ROFlag,*/spe_program_handle_t spuCode, uintptr_t userStruct) {

	/*This is the pthread control block */
	ppu_pthread_data_t ptdata;
	ppu_pthread_data_t ptdata_listener;


	/* this variable is used to return data regarding an abnormal return from the SPE*/
	//spe_stop_info_t stop_info;

	/* this is the handle which will be returned by "spe_context_create.*/
	spe_context_ptr_t speid;
	/* this variable is the SPU entry point address which is initially set to the default*/
	//unsigned int entry = SPE_DEFAULT_ENTRY;

	/* here is the variable to hold the address returned by the malloc() call.*/
	//int *data;
	pthread_t pthread;
#ifdef MULTIBUFFER
	pthread_t pthread_listener;
#endif
	int i;
	controlStruct_t structToSend __attribute__((aligned(128)));

	/*Starting PPE-sided transaction*/
	Thread * Self;
	Self = TxNewThread();
	TxStart(Self,0);

	/*End of creation*/

	//printf("Status of the readSet: \n");
	//printf("Pointer to beggining of List = %p\n", (Self)->Tx.rdSet.List);
	//printf("Pointer to current position in List = %p\n", (Self)->Tx.rdSet.Put);
	//printf("Size of List = %d\n", (Self)->Tx.rdSet.CurrentList);
	//printf("End of Status of the readSet in PPE \n");

	//printf("Status of the writeSet: \n");
	//printf("Pointer to beggining of List = %p\n", (Self)->Tx.wrSet.List);
	//printf("Pointer to current position in List = %p\n", (Self)->Tx.wrSet.Put);
	//printf("Size of List = %d\n", (Self)->Tx.wrSet.CurrentList);
	//printf("Value of first position of WriteSet = %d\n", (Self->Tx.wrSet.List)->ObjValu);
	//printf("End of Status of the writeSet in PPE \n");

	//printf("Reached inside TxStartSPE Step 1\n Received userStruct pointer= %p\n", userStruct);
	if (spe_cpu_info_get(SPE_COUNT_PHYSICAL_SPES, -1) < 1) {
		fprintf(stderr, "System doesn't have a working SPE.  I'm leaving.\n");
		return;
	}
	/* create the SPE context*/
	if ((speid = spe_context_create(SPE_EVENTS_ENABLE, NULL)) == NULL) {
		fprintf(stderr, "FAILED: spe_context_create(errno=%d strerror=%s)\n",
				errno, strerror(errno));
		exit(1);
	}
	/* load the SPE program into the SPE context*/
	if ((spe_program_load(speid, &spuCode)) != 0) {
		fprintf(stderr, "FAILED: spe_program_load(errno=%d strerror=%s)\n", errno, strerror(errno));
		exit(1);
	}

	//Aqui tenho que alocar espaço para as váriaveis alteradas em contexto SPE, dado que o commit vai ser feito deste lado e o apontador de
	//AVPair tem q apontar para algum sitio válido. (q neste momento é uma localização remota)
	unsigned long long volatile objValu __attribute__((aligned(128)));

	//printf("Entering StructToSend-> pointing\n");
	structToSend.userStruct = (unsigned long long) userStruct;
	structToSend.txStruct = (unsigned long long) &(Self->Tx);
	structToSend.rv = Self->rv;
	structToSend.lockTab = (unsigned long long) &LockTab;
	structToSend.rdSet = (unsigned long long) (Self->Tx.rdSet.List);
	structToSend.wrSet = (unsigned long long) (Self->Tx.wrSet.List);
	structToSend.rdSetPut = (unsigned long long) (Self->Tx.rdSet.Put);
	structToSend.wrSetPut = (unsigned long long) (Self->Tx.wrSet.Put);
	structToSend.Owner = (unsigned long long) Self;
	structToSend.pointerToObjValu = (unsigned long long) &objValu;
	structToSend.CurrentListrdSet = (unsigned long long) &(Self->Tx.rdSet.CurrentList);
	structToSend.CurrentListwrSet = (unsigned long long) &(Self->Tx.wrSet.CurrentList);



	ptdata.context=speid;
	ptdata.entry = SPE_DEFAULT_ENTRY;
	ptdata.argp = (void *) &structToSend;
	ptdata.envp = (void *) 128;
	ptdata.thread = Self;
	ptdata.controlStruct = &structToSend;
	//ptdata_listener
	ptdata_listener.context=speid;
	ptdata_listener.thread=Self;
	ptdata_listener.controlStruct=&structToSend;

	//if (spe_context_run(speid, &ptdata.entry, 0, (void *) &structToSend, (void *) 128, &stop_info) < 0) {
	//	fprintf(stderr, "FAILED: spe_context_run(errno=%d strerror=%s)\n", errno, strerror(errno));
	//	fprintf("stop_info information=%s", stop_info.stop_reason);
	//	exit(1);
	//}


	/*This will make a thread that will run the context (non-blocking approach)*/
	pthread_create(&pthread, NULL, &ppu_pthread_function, &ptdata);
#ifdef MULTIBUFFER
	pthread_create(&pthread_listener, NULL, &ppu_pthread_function_listener, &ptdata_listener);
#endif

	pthread_join(pthread, NULL);
	
#ifdef MULTIBUFFER
	pthread_join(pthread_listener, NULL);
#endif
	
	

	spe_context_destroy(speid);

	/*Before commiting we need to set coherent state of both read-set and write-set, things which weren't made by SPE*/

	//printf("nr readset in PPE = %d\n", Self->Tx.rdSet.CurrentList);
	for (i = 0; i < Self->Tx.rdSet.CurrentList; i++) {
		//PrintInfo(Self->Tx.rdSet.Put);
		Self->Tx.rdSet.Put++;
	}
	//printf("nr writeset in PPE = %d\n", Self->Tx.wrSet.CurrentList);
	for (i = 0; i < Self->Tx.wrSet.CurrentList; i++) {
		//PrintInfo(Self->Tx.wrSet.Put);
		Self->Tx.wrSet.Put++;
	}
	Self->Tx.Mode=1;

#ifdef DEBUGON
	printf("#######Before commiting lets check Transaction info##########\n");
	//printf("Library - tl.c - Destination of rdSet is = %p\n", &(Self)->Tx.rdSet);
	//printf("Library - tl.c - Destination of wrSet is = %p\n", &(Self)->Tx.wrSet);
	//Log *tempWr = &(Self)->Tx.wrSet.List;
	//Log *tempRd = &(Self)->Tx.rdSet.List;

	printf("Status of the readSet: \n");
	printf("Pointer to beggining of List = %p\n", (Self)->Tx.rdSet.List);
	printf("Pointer to current position in List = %p\n", (Self)->Tx.rdSet.Put);
	printf("Size of List = %d\n", (Self)->Tx.rdSet.CurrentList);
	printf("End of Status of the readSet in PPE \n");

	printf("Status of the writeSet: \n");
	printf("Pointer to beggining of List = %p\n", (Self)->Tx.wrSet.List);
	printf("Pointer to current position in List = %p\n", (Self)->Tx.wrSet.Put);
	printf("Size of List = %d\n", (Self)->Tx.wrSet.CurrentList);
	printf("Value of first position of WriteSet = %ld\n", (Self->Tx.wrSet.List)->ObjValu[0]);
	printf("End of Status of the writeSet in PPE \n");

	//printf("Printinf info on sizes\n");
	//printf("Size of intprt_t = %d\n", sizeof(intptr_t));
	//printf("Size of unsigned long long = %d\n", sizeof(unsigned long long));
	//printf("Size of pointer = %d\n", sizeof(sizeof(void *)));
	//printf("Size of AVPair = %d\n", sizeof(AVPair));
	//printf("END OF Printinf info on sizes\n");

	//printf("Value of objValu = %llx\n" , objValu);
	printf("Size of AVPAIR is %d", sizeof(AVPair));
#endif


	/*Now we commit - voilá*/
	TxCommit(Self);
	//printf("Returning from TxStartSPE - TxCommit was sucessufull\n");
	//printf("TxStartSPE is returning now, sucessufull Commit.\n");
	//return;

	/* run the SPE context*/
	/*if (spe_context_run(speid, &entry, 0, &data, NULL, &stop_info) < 0) {
		fprintf(stderr, "FAILED: spe_context_run(errno=%d strerror=%s)\n",
				errno, strerror(errno));
		exit(1);
	}*/
}
