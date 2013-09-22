#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>

#include "g_list_stm_ennals.h"

// GENERAL MACROS FOR LISTS

#define GET_KEY(n)        ((setkey_t)(n->key))
#define GET_VALUE(n)      ((setval_t)(n->value))
#define GET_NEXT(n)       ((stm_blk*)(n->next))
#define GET_PREV(n)       ((stm_blk*)(n->prev))

#define SET_KEY(n,_key)    (n->key=_key)
#define SET_VALUE(n,_val)  (n->value=_val)
#define SET_NEXT(n,_next)  (n->next=_next)
#define SET_PREV(n,_prev)  (n->prev=_prev)

#define GET_ROOT(s)        ((stm_blk*)(((set_t*)s)->root))
#define SET_ROOT(s,n)      (((set_t*)s)->root=n)

//--------------------------------------------------------------

unsigned int total_cf=0;

static stm_blk* _new_node(ptst_t *ptst) {
    stm_blk * newb;
    newb = new_stm_blk__(ptst, MEMORY);
//    node_t * new = init_stm_blk__(ptst, memory, newb);
//    new->next=(stm_blk*)0xFFFF;
//    new->prev=(stm_blk*)0xFFFF;
//    new->key=-1;
//    new->value=-1;
    return newb;
}

static void _release_node(ptst_t *ptst, stm_blk * b) {
//    node_t * node = init_stm_blk__(ptst, MEMORY, b);
//    node->next=(stm_blk*)0xFFFF;
//    node->prev=(stm_blk*)0xFFFF;
//    node->key=-1;
//    node->value=-1;
    free_stm_blk__(ptst, MEMORY, b);
}

//insert new node in the list
//new->key and new->value must be already filled
static stm_blk * _insert(ptst_t *ptst, stm_tx *tx, stm_blk *sb, setkey_t key, setval_t val, stm_blk * newb) {
    node_t  *new;
    stm_blk *prevb,*nextb;
    node_t *prev,*next;
    new = init_stm_blk__(ptst, MEMORY, newb);
    set_t * s = (set_t*) read_stm_blk__(ptst, tx, sb);
    if(GET_ROOT(s)==NULL) {
        //list is empty
        s = (set_t*) write_stm_blk__(ptst, tx, sb);
        SET_ROOT(s,newb);
        new->next=NULL;
        new->prev=NULL;
        new->key=key;
        new->value=val;
        return NULL;
    }

    stm_blk * ptrb = GET_ROOT(s);
    node_t * ptr = (node_t*)read_stm_blk__(ptst,tx,ptrb);

    //---------
    prevb=GET_PREV(ptr);
    if(!validate_stm_tx(ptst,tx)) {
        return NULL;
    }
    assert(prevb==NULL);
    //---------

    while(GET_NEXT(ptr)!=NULL && GET_KEY(ptr) < key) {
        ptrb=GET_NEXT(ptr);
        ptr = (node_t*)read_stm_blk__(ptst,tx,ptrb);
    }

    ptr = (node_t*)write_stm_blk__(ptst,tx,ptrb);

    //either list has only 1 node or the insertion will be on the first node
    if(key<GET_KEY(ptr)) {

        //ins before
        prevb=GET_PREV(ptr);
        if(ptrb==GET_ROOT(s)) {
            if(!validate_stm_tx(ptst,tx)) {
                return NULL;
            }
            assert(prevb==NULL);
            s = (set_t*) write_stm_blk__(ptst, tx, sb);
            SET_ROOT(s,newb);
            new->prev=NULL;
        } else {
            if(!validate_stm_tx(ptst,tx)) {
                return NULL;
            }
            assert(prevb!=NULL);
            prev=(node_t*)write_stm_blk__(ptst,tx,prevb);
            SET_NEXT(prev,newb);
            new->prev=prevb;
        }
        new->next=ptrb;
        SET_PREV(ptr,newb);

        new->key=key;
        new->value=val;

        //        prevb=GET_PREV(new);
        //        nextb=GET_NEXT(new);
        //        prev=(node_t*)read_stm_blk__(ptst,tx,prevb);
        //        next=(node_t*)read_stm_blk__(ptst,tx,nextb);
        //        if(!validate_stm_tx(ptst,tx)) {
        //            return NULL;
        //        }
        //        assert(prevb==NULL || GET_KEY(prev)<key);
        //        assert(nextb!=NULL && GET_KEY(next)>key);

        return NULL;
    } else if(key>GET_KEY(ptr)) {
        // ins after
        new->prev=ptrb;
        new->next=GET_NEXT(ptr);
        if(GET_NEXT(ptr)!=NULL) {
            nextb=GET_NEXT(ptr);
            next=(node_t*)write_stm_blk__(ptst,tx,nextb);
            SET_PREV(next, newb);
        }
        SET_NEXT(ptr,newb);

        new->key=key;
        new->value=val;

        //        prevb=GET_PREV(new);
        //        nextb=GET_NEXT(new);
        //        prev=(node_t*)read_stm_blk__(ptst,tx,prevb);
        //        next=(node_t*)read_stm_blk__(ptst,tx,nextb);
        //        if(!validate_stm_tx(ptst,tx)) {
        //            return NULL;
        //        }
        //        assert(prevb!=NULL && GET_KEY(prev)<key);
        //        assert(nextb==NULL || GET_KEY(next)>key);

        return NULL;
    } else {
        //replace key and value
        setkey_t ptrkey=GET_KEY(ptr);
        if(!validate_stm_tx(ptst,tx)) {
            return NULL;
        }
        assert(key==ptrkey);
        SET_KEY(ptr,key);
        SET_VALUE(ptr,val);

        //        prevb=GET_PREV(ptr);
        //        nextb=GET_NEXT(ptr);
        //        prev=(node_t*)read_stm_blk__(ptst,tx,prevb);
        //        next=(node_t*)read_stm_blk__(ptst,tx,nextb);
        //        if(!validate_stm_tx(ptst,tx)) {
        //            return NULL;
        //        }
        //        assert(prev==NULL || GET_KEY(prev)<key);
        //        assert(next==NULL || GET_KEY(next)>key);

        return newb;
    }
}

static stm_blk * _delete(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk * ptrb) {
    if(ptrb==NULL) {
        return NULL;
    }

    node_t * ptr=(node_t*)read_stm_blk__(ptst,tx,ptrb);
    stm_blk * prevb=GET_PREV(ptr);
    stm_blk * nextb=GET_NEXT(ptr);
    node_t * prev=(node_t*)write_stm_blk__(ptst,tx,prevb);
    node_t * next=(node_t*)write_stm_blk__(ptst,tx,nextb);

    if(next!=NULL) {
        //not the last element
        SET_PREV(next,prevb);
    }

    if(prev!=NULL) {
        //not the first element
        SET_NEXT(prev,nextb);
    } else {
        //it is the first element
        set_t * s = (set_t*) write_stm_blk__(ptst, tx, sb);
        SET_ROOT(s,nextb);
    }

    return ptrb;
}

static stm_blk * _lookup(ptst_t *ptst, stm_tx *tx, stm_blk *sb, setkey_t key) {
    set_t * s = (set_t*) read_stm_blk__(ptst, tx, sb);
    if(GET_ROOT(s)==NULL) {
        return NULL;
    }

    stm_blk * ptrb = GET_ROOT(s);
    node_t * ptr = (node_t*)read_stm_blk__(ptst,tx,ptrb);

    //------------
    stm_blk * prevb;
    prevb=GET_PREV(ptr);

    if(!validate_stm_tx(ptst,tx)) {
        return NULL;
    }
    assert(prevb==NULL);
    //------------

    while(ptr!=NULL && GET_KEY(ptr) < key) {
        ptrb=GET_NEXT(ptr);
        //        if(!validate_stm_tx(ptst,tx)) {
        //            return NULL;
        //        }
        ptr = (node_t*)read_stm_blk__(ptst,tx,ptrb);
    }

    if(ptr==NULL)
        return NULL;

    if(GET_KEY(ptr) == key)
        return ptrb;

    return NULL;
}


//-------------------------------------


void kv_init(void) {
    ptst_t  *ptst=NULL;

    //-----------------------
    _init_ptst_subsystem();
    _init_gc_subsystem();

    //-----------------------

    ptst = critical_enter();
    _init_stm_subsystem(0);
    MEMORY = new_stm(ptst, (int)sizeof(node_t));
    critical_exit(ptst);



}//kv_init

void *kv_create(void) {
    //        shared.set = set_alloc();
    ptst_t  *ptst=NULL;
    stm_blk  *msetb;
    set_t  *mset;
    //    node_t *root;

    ptst = critical_enter();

    msetb = new_stm_blk__(ptst, MEMORY);
    mset = (set_t*)init_stm_blk__(ptst, MEMORY, msetb);
    mset->root=NULL;

    critical_exit(ptst);

    return msetb;
}//kv_create

setval_t kv_get(void *sb, setkey_t k) {
    ptst_t  *ptst=NULL;
    stm_tx  *tx=NULL;
    node_t  *n;
    stm_blk * nb;
    setval_t v;

    ptst = critical_enter();

    for(;;) {
        new_stm_tx(tx, ptst, MEMORY);

        nb = _lookup(ptst, tx, sb, k);
        if(nb!=NULL) {
            n = read_stm_blk__(ptst, tx, nb);
            v = n->value;
            if(commit_stm_tx(ptst, tx)) {
                return v;
            }
            total_cf++;
        } else {
            if(commit_stm_tx(ptst, tx)) {
                return 0;
            }
            total_cf++;
        }

    }

    critical_exit(ptst);

    return -1;
}

// puts even if already present
// returns 0 if key was not present
// returns 1 if key was already present
inline int kv_put(void * sb, setkey_t k, setval_t v) {
    ptst_t  *ptst=NULL;
    stm_tx  *tx=NULL;

    ptst = critical_enter();

    stm_blk * newb = _new_node(ptst);
    stm_blk * exb;

    for (;;) {
        new_stm_tx(tx, ptst, MEMORY);
        exb = _insert(ptst, tx, sb, k, v, newb);
        if (exb != NULL) {
            if (commit_stm_tx(ptst, tx)) {
                _release_node (ptst, newb);
                return 1;
            }
            total_cf++;
        } else {
            if (commit_stm_tx(ptst, tx)) {
                return 0;
            }
            total_cf++;
        }
    }
}


// Remove key k on set s
int kv_delete(void *sb, setkey_t k) {
    ptst_t  *ptst=NULL;
    stm_tx  *tx=NULL;

    stm_blk * nodeb=NULL;

    ptst = critical_enter();

    for(;;) {
        new_stm_tx(tx, ptst, MEMORY);

        nodeb = _lookup(ptst, tx, sb, k);



        if (nodeb != NULL) {
            nodeb = _delete(ptst, tx, sb, nodeb);
        }
        if(commit_stm_tx(ptst, tx)) {
            break;
        }
        total_cf++;
    }

    if (nodeb != NULL)
        _release_node(ptst, nodeb);

    critical_exit(ptst);

    return (nodeb != NULL);
}// kv_delete




int kv_verify (void * sb, int Verbose) {
    int num;
    for (;;) {
        stm_blk* ptrb=NULL;
        node_t * ptr=NULL;

        set_t*s = (set_t*)init_stm_blk__(NULL, MEMORY, (stm_blk*)sb);

        ptrb = GET_ROOT(s);
        ptr=init_stm_blk__(NULL, MEMORY, ptrb);

        assert(ptrb==NULL||(node_t*)GET_PREV(ptr)==NULL);

        num=0;
        while(ptr!=NULL) {
            stm_blk* nextb = GET_NEXT(ptr);
            node_t* next = init_stm_blk__(NULL, MEMORY, nextb);

            if(next!=NULL) {
                // has a next node ->let's see if next/prev match
                stm_blk * tmp_thisb = GET_PREV(next);
                assert(tmp_thisb==ptrb);
                assert(GET_KEY(ptr)<GET_KEY(next));
            }

            int key = GET_KEY(ptr);
            int val = GET_VALUE(ptr);
            assert(val==key+100);

            //        sched_yield();

            stm_blk* next2b = GET_NEXT(ptr);
            node_t* next2 = init_stm_blk__(NULL, MEMORY, next2b);

            if(next!=next2) {
                printf("XXXXX num = %d, ptr=%lx, next=%lx, next2=%lx - %lx %lx %lx %lx %lx %lx\n",
                       num,(long)(uintptr_t)ptr,
                       (long)(uintptr_t)next,(long)(uintptr_t)next2,
                       (long)(uintptr_t)ptr->next, (long)(uintptr_t)ptr->prev,
                       (long)(uintptr_t)next->next, (long)(uintptr_t)next->prev,
                       (long)(uintptr_t)next2->next, (long)(uintptr_t)next2->prev);
            }

            assert(next==next2);

            ptrb=nextb;
            ptr=next;
            num++;
        }

        break;

    }
    return num;

}//kv_verify

