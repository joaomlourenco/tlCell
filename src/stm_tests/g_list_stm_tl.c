#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>



#include "g_list_stm_tl.h"
#include "trace_log.h"

//------------------
//#include "../tl2/tl.c"
//------------------

unsigned int total_cf=0;

node_t * _new_node(Thread * Self) {
    node_t * new;
#if 1

    new = (node_t *) malloc (sizeof(node_t));
    //memset (new, 0xFF, sizeof(node_t));
#else // this may be necessary when using different line sizes
    int num_blocks=((sizeof(node_t)-1)/LINE_SIZE)+1;//ceil
    //new = (node_t *) malloc (num_blocks*LINE_SIZE);
    new = (node_t *) memalign(LINE_SIZE, num_blocks*LINE_SIZE);
#endif

#ifdef OBJ_STM_PO

    new->_POLOCK=0;
#endif

    return new;
}

void _release_node(Thread * Self, node_t* n) {
    UNTX(n);
    //    n->n=(node_t*)0xFFFF;
    //    n->p=(node_t*)0xFFFF;
    //    n->k=-1;
    //    n->v=-1;
    free(n);
}

//insert new node in the list
//new->key and new->value must be already filled
node_t * _insert(Thread * Self, set_t * s, setkey_t key, setval_t val, node_t * new) {
    node_t * prev;
    node_t * next;

    WRITE_OBJ(new, sizeof(node_t));

    READ_OBJ(s);
    node_t * ptr = GET_ROOT(s);
    VFY(s);

    if(ptr==NULL) {
        //list is empty
        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s,new);
        SET_NEXT(new,NULL);
        SET_PREV(new,NULL);
        SET_KEY(new,key);
        SET_VALUE(new,val);
        return NULL;
    }

    //TraceEvent(Self->UniqID, 10, _tl_kv_put, NULL, 0, 0);

    READ_OBJ(ptr);

    //---------
    prev=GET_PREV(ptr);
    VFY(ptr);
    vfy_assert(prev==NULL);
    //---------

    while(GET_NEXT(ptr)!=NULL && GET_KEY(ptr) < key) {
        //        setkey_t kkk = GET_KEY(ptr);
        //        setval_t vvv = GET_VALUE(ptr);
        //        VFY(ptr);
        //        vfy_assert(kkk+100==vvv);

#if defined PARTIAL_VFY && defined OBJ_STM

        node_t * ptr_tmp = ptr;
#endif

        ptr=GET_NEXT(ptr);
        VFY(ptr_tmp);
        READ_OBJ(ptr);
    }
    VFY(ptr);

    //TraceEvent(Self->UniqID, 20, _tl_kv_put, NULL, 0, 0);
    WRITE_OBJ(ptr, sizeof(node_t));

    //either list has only 1 node or the insertion will be on the first node
    if(key<GET_KEY(ptr)) {
        //TraceEvent(Self->UniqID, 30, _tl_kv_put, NULL, 0, 0);
        //ins before
        prev=GET_PREV(ptr);
        if(ptr==GET_ROOT(s)) {
            //TraceEvent(Self->UniqID, 40, _tl_kv_put, NULL, 0, 0);
            VFY(s);
            vfy_assert(prev==NULL);
            WRITE_OBJ(s, sizeof(set_t));
            SET_ROOT(s,new);
            SET_PREV(new,NULL);

        } else {
            //TraceEvent(Self->UniqID, 50, _tl_kv_put, NULL, 0, 0);
            VFY(s);
            vfy_assert(prev!=NULL);
            WRITE_OBJ(prev, sizeof(node_t));
            SET_NEXT(prev,new);
            SET_PREV(new,prev);
        }
        SET_NEXT(new,ptr);
        SET_PREV(ptr,new);

        SET_KEY(new,key);
        SET_VALUE(new,val);

        //        vfy_assert(GET_PREV(new)==NULL || GET_KEY(GET_PREV(new))<key);
        //        vfy_assert(GET_NEXT(new)!=NULL && GET_KEY(GET_NEXT(new))>key);

        return NULL;
    } else if(key>GET_KEY(ptr)) {
        //TraceEvent(Self->UniqID, 60, _tl_kv_put, NULL, 0, 0);
        // ins after
        SET_PREV(new,ptr);
        SET_NEXT(new,GET_NEXT(ptr));
        if(GET_NEXT(ptr)!=NULL) {
            //TraceEvent(Self->UniqID, 70, _tl_kv_put, NULL, 0, 0);
            next=GET_NEXT(ptr);
            WRITE_OBJ(next, sizeof(node_t));
            SET_PREV(next, new);
        }
        SET_NEXT(ptr,new);

        SET_KEY(new,key);
        SET_VALUE(new,val);

        //        vfy_assert(GET_PREV(new)!=NULL && GET_KEY(GET_PREV(new))<key);
        //        vfy_assert(GET_NEXT(new)==NULL || GET_KEY(GET_NEXT(new))>key);

        return NULL;
    } else {
        //TraceEvent(Self->UniqID, 80, _tl_kv_put, NULL, 0, 0);
        //replace key and value
        setkey_t ptrkey=GET_KEY(ptr);
        vfy_assert(key==ptrkey);
        SET_KEY(ptr,key);//unnecessary...
        SET_VALUE(ptr,val);

        //        vfy_assert(prev==NULL || GET_KEY(prev)<key);
        //        vfy_assert(next==NULL || GET_KEY(next)>key);

        return new;
    }
}

node_t * _delete(Thread * Self, set_t * s, node_t * ptr) {
    //TraceEvent(Self->UniqID, 10, _tl_kv_delete, (intptr_t *)ptr, 0, 0);
    if(ptr==NULL) {
        return NULL;
    }

    READ_OBJ(ptr);
    node_t * prev = GET_PREV(ptr);
    node_t * next = GET_NEXT(ptr);
    VFY(ptr);
    WRITE_OBJ(prev, sizeof(node_t));
    WRITE_OBJ(next, sizeof(node_t));

    if(next!=NULL) {
        //TraceEvent(Self->UniqID, 20, _tl_kv_delete, NULL, 0, 0);
        //not the last element
        SET_PREV(next,prev);
    }

    if(prev!=NULL) {
        //TraceEvent(Self->UniqID, 30, _tl_kv_delete, NULL, 0, 0);
        //not the first element
        SET_NEXT(prev,next);
    } else {
        //TraceEvent(Self->UniqID, 40, _tl_kv_delete, NULL, 0, 0);
        //it is the first element
        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s,next);
    }

    return ptr;
}

node_t * _lookup(Thread * Self, set_t * s, setkey_t key) {
    READ_OBJ(s);

    node_t * ptr = GET_ROOT(s);
    VFY(s);
    if(ptr==NULL) {
        return NULL;
    }

    READ_OBJ(ptr);

    //------------
    node_t * prev;
    prev=GET_PREV(ptr);
    VFY(ptr);
    //    if(!TXVALID(Self)) {
    //        return NULL;
    //    }
    vfy_assert(prev==NULL);
    //------------


    while(ptr!=NULL && GET_KEY(ptr) < key) {
        //        setkey_t kkk = GET_KEY(ptr);
        //        setval_t vvv = GET_VALUE(ptr);
        //        VFY(ptr);
        //        vfy_assert(kkk+100==vvv);

#if defined PARTIAL_VFY && defined OBJ_STM

        node_t * ptr_tmp = ptr;
#endif

        ptr=GET_NEXT(ptr);
        VFY(ptr_tmp);

        //        if(!TXVALID(Self)) {
        //            return NULL;
        //        }
        READ_OBJ(ptr);
    }

    VFY(ptr);
    if(ptr==NULL)
        return NULL;

    if(GET_KEY(ptr) == key) {
        VFY(ptr);
        return ptr;
    }

    VFY(ptr);
    return NULL;
}


//-------------------------------------


void kv_init() {}




void *kv_create() {
    set_t * n = (set_t * ) malloc (sizeof(*n));
    n->root = NULL;
    return n;
}

//#include <trace.h>

setval_t kv_get (Thread * Self, void  * s, setkey_t k) {
    node_t * n;
    setval_t v;

    //    for (;;) {
    TXSTART (Self, 1);
    //TraceEvent(Self->UniqID, 0, _tl_kv_get, NULL, 0, k);
		//trace_log_tx_start(2);


    n = _lookup(Self, s, k);
    if (n != NULL) {
        READ_OBJ(n);
        v = GET_VALUE(n);
        VFY(n);
        if (TXCOMMIT(Self)) {
            return v;
        }
        total_cf++;
    } else {
        if (TXCOMMIT(Self)) {
            return 0;
        }
        total_cf++;
    }
    //    }

    return -1;
}

// puts even if already present
// returns 0 if key was not present
// returns 1 if key was already present
int kv_put (Thread * Self, void * s, setkey_t k, setval_t Val) {
    node_t * nn = _new_node(Self);
    //    for (;;) {
    TXSTART (Self, 0);
    //TraceEvent(Self->UniqID, 0, _tl_kv_put, (intptr_t*)nn, 0, k);
		//trace_log_tx_start(0);

    node_t * ex = _insert (Self, s, k, Val, nn);
    if (ex != NULL) {
        if (TXCOMMIT(Self)) {
            //_release_node (Self, nn);
            nn->n=(node_t*)0xFFFF;
            nn->p=(node_t*)0xFFFF;
            nn->k=-1;
            nn->v=-1;
            free(nn);
            return 1;
        }
        total_cf++;
    } else {
        if (TXCOMMIT(Self)) {
            return 0;
        }
        total_cf++;
    }
    //    }
    return -1;
}

int kv_delete(Thread * Self, void *s, setkey_t k) {
    node_t * node = NULL;

    //    for (;;) {
    TXSTART (Self, 0);
		//trace_log_tx_start(1);

    node = _lookup(Self, s, k);
    if (node != NULL) {
        node = _delete(Self, s, node);
    }
    if (TXCOMMIT(Self)) {
        //            break;
    } else {
        total_cf++;
    }
    //    }
    if (node != NULL)
        _release_node(Self, node);
    return (node != NULL);
}


int kv_verify (Thread * Self, void * s, int Verbose) {
    int num;
    node_t * ptr=NULL;

    node_t * head = ((set_t*)s)->root;

    node_t * head_prev=NULL;
    if(head!=NULL) {
        head_prev=head->p;

    }
    assert(head==NULL||head_prev==NULL);

    num=0;
    ptr=head;
    while(ptr!=NULL) {
        node_t* next = ptr->n;

        if(next!=NULL) {
            // has a next node ->let's see if next/prev match
            node_t * tmp_this = next->p;

            setkey_t ptr_key,next_key;
            ptr_key=ptr->k;
            next_key=next->k;
            assert(tmp_this==ptr);
            assert(ptr_key<next_key);
        }

        int key = ptr->k;
        int val = ptr->v;
        assert(val==key+100);
        printf("#PPE#KV_VERIFY  val=%d key=%d \n", (int) val, (int) key);

        //        sched_yield();

        node_t* next2 = ptr->n;

        if(next!=next2) {
            printf("XXXXX num = %d, ptr=%lx, next=%lx, next2=%lx - %lx %lx %lx %lx %lx %lx\n",
                   num,(long)(uintptr_t)ptr,
                   (long)(uintptr_t)next,(long)(uintptr_t)next2,
                   (long)(uintptr_t)ptr->n, (long)(uintptr_t)ptr->p,
                   (long)(uintptr_t)next->n, (long)(uintptr_t)next->p,
                   (long)(uintptr_t)next2->n, (long)(uintptr_t)next2->p);
        }
        assert(next==next2);

        ptr=next;
        num++;
    }

    return num;

}//kv_verify

// /usr/bin/ppu-gcc -g  -W -Wall -Winline   -I.  -I /usr/include -I /home/lopeici/1.0.2/src -I /opt/cell/sdk/usr/include -mabi=altivec -maltivec -O3 -c testePPC.c
// /usr/bin/ppu-gcc -g -o  testePPC  testePPC.o      -L/usr/lib64 -L/opt/cell/sdk/usr/lib64 -L /home/lopeici/CTL/CTLlib/lib/ -R/opt/cell/sdk/usr/lib64 ../spu/spuCode.a -lspe2 -lctl-cmt-word-64


