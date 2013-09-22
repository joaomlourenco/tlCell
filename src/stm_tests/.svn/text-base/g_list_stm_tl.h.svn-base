#ifndef LIST_STM_H_
#define LIST_STM_H_

#include "g_kv_stm_tl.h"



//#define vfy_assert assert
#define vfy_assert(x) assert(x || !TxValid(Self))

//-----------------------------
typedef struct _node_t {
#ifdef OBJ_STM_PO
    vwLock _POLOCK;
#endif
    setkey_t k;
    setval_t v;
    struct _node_t *n;
    struct _node_t *p;
}
node_t;

typedef struct _set_t {
#ifdef OBJ_STM_PO
    vwLock _POLOCK;
#endif
    //double padA [32] ;//Also change the MAX_OBJ_SIZE
    struct _node_t *root;
    //double padB [32] ;
}
set_t;

//-----------------------------
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
//--------------
#define SET_KEY(_n,_k)       TXSTF((_n),k,(_k))
#define SET_VALUE(_n,_v)     TXSTF((_n),v,(_v))
#define SET_NODE(_n,_f,_v)   TXSTF((_n),_f,(_v))
//------------------------------------
#else
// --------------------------------
#ifdef FULL_VFY
#define READ_OBJ(__o)       TxOpenRead(Self,__o) 
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s)      
#define UNTX(_n)            TXSTERILIZE(Self,_n,1)
#define VFY(_o)	            
//--------------
#define GET_KEY(_n)          TxReadField(Self, (_n), k)
#define GET_VALUE(_n)        TxReadField(Self, (_n), v)
#define GET_NODE(_n,_f)      TxReadField(Self, (_n), _f)
//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))
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
//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))

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

//--------------
#define SET_KEY(_n,_k)       ((_n)->k=(_k))
#define SET_VALUE(_n,_v)     ((_n)->v=(_v))
#define SET_NODE(_n,_f,_v)   ((_n)->_f=(_v))

#endif

#endif // OBJ_STM


#define GET_ROOT(_s)         GET_NODE(((set_t*)_s),root)
#define SET_ROOT(_s,_v)      SET_NODE(((set_t*)_s),root,(_v))

#define GET_NEXT(_n)         GET_NODE(_n,n)
#define GET_PREV(_n)         GET_NODE(_n,p)
#define SET_NEXT(_n,_v)      SET_NODE(_n,n,_v)
#define SET_PREV(_n,_v)      SET_NODE(_n,p,_v)


#endif /*LIST_STM_H_*/ 

/*
// Enable this to verify if any reads/writes are being made without the corresponding open
#define READ_OBJ(__o)       TxOpenRead(Self,__o); 
#define WRITE_OBJ(__o, __s) TxOpenWrite(Self, __o, __s);      
#define UNTX(_n)           TXSTERILIZE(Self,_n,1)
#define VFY(_o)				 TxVerifyAddr(Self, _o);
// ---------------
#define GET_KEY(_n)        ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((intptr_t)((_n)->k));})
#define GET_VALUE(_n)      ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((intptr_t)((_n)->v));})
#define GET_NODE(_n,_f)       ({ assert(IsOpenR(Self, _n)||IsOpenW(Self, _n));\
	((node_t*)((_n)->_f));})

#define SET_KEY(_n,_k)    ({ assert(IsOpenW(Self, _n));\
	((_n)->k=(_k));})
#define SET_VALUE(_n,_v)  ({ assert(IsOpenW(Self, _n));\
	((_n)->v=(_v));})
#define SET_NODE(_n,_f,_v)  ({ assert(IsOpenW(Self, _n));\
	((_n)->_f=(_v));})
*/

