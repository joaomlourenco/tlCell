#ifndef LIST_STM_ENNALS_H_
#define LIST_STM_ENNALS_H_

#include "g_kv_stm_ennals.h"
#include "portable_defns.h"
#include "ptst.h"
#include "stm.h"

//------------------------------------
typedef enum {RED=0,BLACK=1} COLOUR;

#define IS_BLACK(_x)      (_x==NULL || _x->c==BLACK)
#define IS_RED(_x)        (!IS_BLACK(_x))

stm *memory;    /* read-only */
#define MEMORY memory

// ------------------------------------------------

typedef struct _node {
    setkey_t key;
    setval_t value;
    stm_blk *next;
    stm_blk *prev;
}
node_t;

typedef struct _set {
    stm_blk * root;
}
set_t;

//-------------------


#endif
