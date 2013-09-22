#ifndef RB_STM_ENNALS_H_
#define RB_STM_ENNALS_H_

#include "g_kv_stm_ennals.h"
#include "portable_defns.h"
#include "ptst.h"
#include "stm.h"

//------------------------------------
typedef enum {RED=0,BLACK=1} COLOUR;

stm *memory;    /* read-only */
#define MEMORY memory

typedef struct _node {
    setkey_t k;
    setval_t v;
    int c;
    stm_blk *l, *r, *p;
}
node_t;

typedef struct _set {
    stm_blk * root;
}
set_t;

//-------------------
#define GET_COLOUR(_n)       ((int)((_n)->c))
#define GET_KEY(_n)          ((setkey_t)((_n)->k))
#define GET_VALUE(_n)        ((setval_t)((_n)->v))
#define GET_NODE(_n,_f)      ((stm_blk *)((_n)->_f))
#define SET_COLOUR(_n,_c)    ((_n)->c=_c)
#define SET_KEY(_n,_k)       ((_n)->k=_k)
#define SET_VALUE(_n,_v)     ((_n)->v=_v)
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))

#define IS_BLACK(_n)         (_n == NULL || GET_COLOUR(_n) == BLACK)
#define IS_RED(_n)           (!IS_BLACK((_n)))

#define GET_ROOT(s)          ((stm_blk*)(((set_t*)s)->root))
#define SET_ROOT(s,n)        (((set_t*)s)->root=n)

#define SET_PARENT(_n, _v)   SET_NODE(_n, p, _v)
#define SET_LEFT(_n, _v)     SET_NODE(_n, l, _v)
#define SET_RIGHT(_n, _v)    SET_NODE(_n, r, _v)
#define GET_PARENT(_n)       GET_NODE(_n, p)
#define GET_LEFT(_n)         GET_NODE(_n, l)
#define GET_RIGHT(_n)        GET_NODE(_n, r)




#endif /*RB_STM_H_*/
