#ifndef RB_H_
#define RB_H_

#include "g_kv_stm_tl.h"

//#define vfy_assert assert
#define vfy_assert(x) assert(x || !TxValid(Self))

//-----------------------------
typedef enum { RED=0, BLACK=1 } Colors ;

typedef struct _node {
#ifdef OBJ_STM_PO
    vwLock _POLOCK;
#endif

    setkey_t k ;
    setval_t v ;
    struct _node * p ;
    struct _node * l ;
    struct _node * r ;
    intptr_t c ;
}
node_t ;

typedef struct _set {
#ifdef OBJ_STM_PO
    vwLock _POLOCK;
#endif
    //double padA [32] ;//Also change the MAX_OBJ_SIZE
    node_t * root ;
    //double padB [32] ;
}
set_t ;

// ------------------------------

#ifndef OBJ_STM
//----------------------------------
#define READ_OBJ(__o)       
#define WRITE_OBJ(__o, __s)      
#define UNTX(_n)           TXSTERILIZE(Self,_n,sizeof(node_t)/sizeof(intptr_t))
#define VFY(_o)
//--------------
#define GET_KEY(_n)          ((intptr_t)TXLDF((_n),k))
#define GET_VALUE(_n)        ((intptr_t)TXLDF((_n),v))
#define GET_NODE(_n,_f)      ((node_t *)TXLDF((_n),_f))
#define GET_COLOUR(_n)       ((intptr_t)TXLDF((_n),c))
//--------------
#define SET_KEY(_n,_k)       TXSTF((_n),k,(_k))
#define SET_VALUE(_n,_v)     TXSTF((_n),v,(_v))
#define SET_NODE(_n,_f,_v)   TXSTF((_n),_f,(_v))
#define SET_COLOUR(_n,_c)    TXSTF((_n),c,(_c))
//------------------------------------
#else
// --------------------------------
#ifdef FULL_VFY
#define READ_OBJ(__o) 		TxOpenRead(Self,__o)
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s)      
#define UNTX(_n)            TXSTERILIZE(Self,_n,1)
#define VFY(_o)	            
//--------------
#define GET_KEY(_n)          TxReadField(Self, (_n), k)
#define GET_VALUE(_n)        TxReadField(Self, (_n), v)
#define GET_NODE(_n,_f)      TxReadField(Self, (_n), _f)
#define GET_COLOUR(_n)       TxReadField(Self, (_n), c)
//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))
#define SET_COLOUR(_n,_c)    ((_n)->c=(_c))
#endif

#ifdef PARTIAL_VFY
#define READ_OBJ(__o)       TxOpenRead(Self,__o) 
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s)      
#define UNTX(_n)           TXSTERILIZE(Self,_n,1)
#define VFY(_o)				 TxVerifyAddr(Self, _o)
//--------------
#define GET_KEY(_n)          ((intptr_t)((_n)->k))
#define GET_VALUE(_n)        ((intptr_t)((_n)->v))
#define GET_NODE(_n,_f)      ((node_t *)((_n)->_f))
#define GET_COLOUR(_n)       ((intptr_t)((_n)->c))
//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))
#define SET_COLOUR(_n,_c)    ((_n)->c=(_c))
#endif

#ifdef NO_VFY
#define READ_OBJ(__o)       TxOpenRead(Self,__o) 
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s)      
#define UNTX(_n)           TXSTERILIZE(Self,_n,1)
#define VFY(_o)				 
//--------------
#define GET_KEY(_n)          ((intptr_t)((_n)->k))
#define GET_VALUE(_n)        ((intptr_t)((_n)->v))
#define GET_NODE(_n,_f)      ((node_t *)((_n)->_f))
#define GET_COLOUR(_n)       ((intptr_t)((_n)->c))
//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))
#define SET_COLOUR(_n,_c)    ((_n)->c=(_c))
#endif

#endif //OBJ_STM

#define IS_BLACK(_n)         (_n == NULL || GET_COLOUR(_n) == BLACK)
#define IS_RED(_n)           (!IS_BLACK((_n)))

#define GET_GRAND_PARENT(_n) GET_PARENT(GET_PARENT((_n)))
#define GET_LEFT_UNCLE(_n)   GET_LEFT(GET_PARENT(GET_PARENT((_n))))
#define GET_RIGHT_UNCLE(_n)  GET_RIGHT(GET_PARENT(GET_PARENT((_n))))

#define GET_ROOT(_s)         GET_NODE(((set_t*)_s),root)
#define SET_ROOT(_s,_v)      SET_NODE(((set_t*)_s),root,(_v))

#define GET_PARENT(_n)     GET_NODE(_n,p)
#define GET_LEFT(_n)       GET_NODE(_n,l)
#define GET_RIGHT(_n)      GET_NODE(_n,r)
#define SET_PARENT(_n,_v)  SET_NODE(_n,p,_v)
#define SET_LEFT(_n,_v)    SET_NODE(_n,l,_v)
#define SET_RIGHT(_n,_v)   SET_NODE(_n,r,_v)


#endif /*RB_H_*/

/*
// Enable this to verify if any reads/writes are being made without the corresponding open
#define READ_OBJ(__o)       TxOpenRead(Self,__o) 
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s)      
#define UNTX(_n)           TXSTERILIZE(Self,_n,1)
#define VFY(_o)				 TxVerifyAddr(Self, _o)
//--------------
#define GET_KEY(_n)        ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((intptr_t)((_n)->k));})
#define GET_VALUE(_n)      ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((intptr_t)((_n)->v));})
#define GET_NODE(_n,_f)       ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((node_t*)((_n)->_f));})
#define GET_COLOUR(_n)       ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((intptr_t)((_n)->c));})

#define SET_KEY(_n,_k)    ({ assert(IsOpenW(Self, _n));\
	((_n)->k=(_k));})
#define SET_VALUE(_n,_v)  ({ assert(IsOpenW(Self, _n));\
	((_n)->v=(_v));})
#define SET_NODE(_n,_f,_v)  ({ assert(IsOpenW(Self, _n));\
	((_n)->_f=(_v));})
#define SET_COLOUR(_n,_c)  ({ assert(IsOpenW(Self, _n));\
	((_n)->c=(_c));})
*/

