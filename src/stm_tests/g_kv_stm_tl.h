#ifndef G_GENERAL_STM_TL2_H_
#define G_GENERAL_STM_TL2_H_

#include <tl_cmt_word_64.h>

//#define NO_VFY
//#define PARTIAL_VFY
//#define FULL_VFY
#if defined OBJ_STM && ! defined NO_VFY && ! defined PARTIAL_VFY && ! defined FULL_VFY
#warning no flag VFY defined - defaulting to FULL verification
#define FULL_VFY
#endif 

//---------------------------------

typedef intptr_t setkey_t;
typedef intptr_t setval_t;

// --------------------------------
#define TXSTART     TxStart
#define TXCOMMIT    TxCommit 
#define TXSTERILIZE TxSterilize 
#define TXVALID     TxValid 
#define TXVALIDATE_AND_ABORT     TxValidateAndAbort 
#define TXNEWTHREAD TxNewThread
#define TXENDTHREAD TxEndThread

/*
#define TXVALID(x) ({ \
	int _qwe_ = TxValid(x); \
	assert(_qwe_); \
	_qwe_; \
	})
*/

extern void     kv_init();
extern void *   kv_create () ;
extern int      kv_put      (Thread * Self, void * sk, setkey_t k, setval_t v) ;
extern int      kv_delete   (Thread * Self, void * sk, setkey_t k) ;
extern setval_t kv_get      (Thread * Self, void * sk, setkey_t k) ;
extern int      kv_verify   (Thread * Self, void * sk, int Verbose) ;

#endif
