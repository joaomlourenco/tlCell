#ifndef KV_STM_ENNALS_H_
#define KV_STM_ENNALS_H_

#include "portable_defns.h"
#include "ptst.h"
#include "stm.h"

//------------------------------------
typedef long setkey_t;
typedef long setval_t;

extern void     kv_init     ();
extern void *   kv_create   ();
extern int      kv_put      (void * sk, setkey_t k, setval_t v) ;
extern int      kv_delete   (void * sk, setkey_t k);
extern setval_t kv_get      (void * sk, setkey_t k) ;
extern int      kv_verify   (void * sk, int Verbose) ;
extern void     kv_print    (void * sk) ;


//-------------------


#define write_stm_blk__ (node_t*)write_stm_blk
#define read_stm_blk__  (node_t*)read_stm_blk
#define init_stm_blk__  (node_t*)init_stm_blk
#define new_stm_blk__   (stm_blk*)new_stm_blk
#define free_stm_blk__  free_stm_blk

#define READ_OBJ(_o) read_stm_blk__(ptst, tx, _o)
#define WRITE_OBJ(_o) write_stm_blk__(ptst, tx, _o)

#endif /*RB_STM_H_*/
