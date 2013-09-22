#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <tl_cmt_word_32.h>
#include "g_rb_stm_tl.h"
//#include "trace_log.h"

//------------------
//#include "../tl2/tl.c"
//------------------


unsigned int total_cf=0;

static node_t  * _new_node (Thread * Self) {
    node_t * new;
#if 1

    new = (node_t *) malloc (sizeof(node_t));
    //memset (new, 0xFF, sizeof(node_t));
#else  // this may be necessary when using different line sizes
    int num_blocks=((sizeof(node_t)-1)/LINE_SIZE)+1;//ceil
    //new = (node_t *) malloc (num_blocks*LINE_SIZE);
    new = (node_t *) memalign(LINE_SIZE, num_blocks*LINE_SIZE);
#endif

#ifdef OBJ_STM_PO

    new->_POLOCK=0;
#endif

    return new;
}



static void _release_node (Thread * Self, node_t * n) {
    UNTX(n);
    //memset (n, 0xFF, sizeof(node_t));
    free (n);
}


// x must be already open for writing
// x must have a parent
static void _left_rotate (Thread * Self, set_t * s, node_t * x) {
    node_t *r, *rl, *p;


    r = GET_RIGHT(x);
    p = GET_PARENT(x);

    WRITE_OBJ(r, sizeof(node_t));
    WRITE_OBJ(p, sizeof(node_t));
    rl = GET_LEFT(r);

    WRITE_OBJ(x, sizeof(node_t));
    SET_RIGHT(x,rl);
    if (rl != NULL) {
        WRITE_OBJ(rl, sizeof(node_t));
        SET_PARENT(rl,x);
    }

    SET_PARENT(r,p);
    if (p == NULL) {
        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s,r);
    } else if (GET_LEFT(p) == x) {
        SET_LEFT(p,r);
    } else {
        SET_RIGHT(p,r);
    }
    SET_LEFT(r,x);
    SET_PARENT(x,r);
}//left_rotate

// x must be already open for writing
// x must have a parent
static void _right_rotate (Thread * Self, set_t * s, node_t * x) {
    node_t *l, *lr, *p;


    l = GET_LEFT(x);
    p = GET_PARENT(x);

    WRITE_OBJ(l, sizeof(node_t));
    WRITE_OBJ(p, sizeof(node_t));
    lr = GET_RIGHT(l);

    WRITE_OBJ(x, sizeof(node_t));
    SET_LEFT(x,lr);
    if (lr != NULL) {
        WRITE_OBJ(lr, sizeof(node_t));
        SET_PARENT(lr,x);
    }

    SET_PARENT(l,p);
    if (p == NULL) {
        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s,l);
    } else if (GET_RIGHT(p) == x) {
        SET_RIGHT(p,l);
    } else {
        SET_LEFT(p,l);
    }
    SET_RIGHT(l,x);
    SET_PARENT(x,l);
}//right_rotate



// Rebalance tree after insertion of x
// x must be already open for write
static void _fix_after_insertion(Thread * Self, set_t * s, node_t * x) {
    node_t * p, *gp, *y, *lu;


    SET_COLOUR(x,RED);
    // rebalance tree
    READ_OBJ(s);
    while (x != NULL && GET_PARENT(x) != NULL) {
        p = GET_PARENT(x);
        READ_OBJ(p);
        if (IS_BLACK(p)) // case 2 - parent is black
            break;

        gp = GET_PARENT(p);
        VFY(p);
        READ_OBJ(gp);
        lu = GET_LEFT(gp);

        if (p == lu) {
            // parent is red, p=GET_LEFT(g)
            y = GET_RIGHT(gp); // y (uncle)
            VFY(gp);
            READ_OBJ(y);
            if (IS_RED(y)) {
                // case 3 - parent is red, uncle is red (p = GET_LEFT(gp))
                WRITE_OBJ(p, sizeof(node_t));
                WRITE_OBJ(y, sizeof(node_t));
                WRITE_OBJ(gp, sizeof(node_t));
                SET_COLOUR(p, BLACK);
                SET_COLOUR(y, BLACK);
                SET_COLOUR(gp, RED);
                x = gp;

            } else {
                VFY(y);
                // parent is red, uncle is black (p = GET_LEFT(gp))
                if (x == GET_RIGHT(p)) {
                    // case 4 - parent is red, uncle is black, x = GET_RIGHT(p), p = GET_LEFT(gp)
                    x = p;
                    WRITE_OBJ(p, sizeof(node_t));
                    _left_rotate(Self, s, x);
                    p = GET_PARENT(x);
                }
                // case 5 - parent is red, uncle is black, x = GET_LEFT(p), p = GET_LEFT(gp)
                WRITE_OBJ(p, sizeof(node_t));
                gp = GET_PARENT(p);
                WRITE_OBJ(gp, sizeof(node_t));
                SET_COLOUR(p, BLACK);
                SET_COLOUR(gp, RED);
                if (gp != NULL) {
                    _right_rotate(Self, s, gp);
                }
            }
        } else {
            // parent is red, p = GET_RIGHT(gp)
            VFY(gp);
            y = lu;
            READ_OBJ(y);
            if (IS_RED(y)) {
                // case 3 - parent is red, uncle is red (p = GET_RIGHT(gp))
                WRITE_OBJ(p, sizeof(node_t));
                WRITE_OBJ(y, sizeof(node_t));
                WRITE_OBJ(gp, sizeof(node_t));
                SET_COLOUR(p, BLACK);
                SET_COLOUR(y, BLACK);
                SET_COLOUR(gp, RED);
                x = gp;

            } else {
                VFY(y);
                // parent is red, uncle is black (p = GET_RIGHT(gp))
                if (x == GET_LEFT(p)) {
                    // case 4 - parent is red, uncle is black, x = GET_LEFT(p), p = GET_RIGHT(gp)
                    x = p;
                    WRITE_OBJ(p, sizeof(node_t));
                    _right_rotate(Self, s,x);
                    p = GET_PARENT(x);
                }
                // case 5 - parent is red, uncle is black, x = GET_RIGHT(p), p = GET_RIGHT(gp)
                WRITE_OBJ(p, sizeof(node_t));
                gp = GET_PARENT(p);
                WRITE_OBJ(gp, sizeof(node_t));
                SET_COLOUR(p,  BLACK);
                SET_COLOUR(gp, RED);
                if (gp != NULL) {
                    _left_rotate(Self, s, gp);
                }
            }
        }
    }

    READ_OBJ(s);
    node_t * ro = GET_ROOT(s);
    VFY(s);
    READ_OBJ(ro);
    if (IS_RED(ro)) {
        WRITE_OBJ(ro, sizeof(node_t));
        SET_COLOUR(ro,BLACK);
    }
}

static void _fix_after_deletion(Thread * Self, set_t * s, node_t *  x) {
    node_t * p, *pl, *sib, *sibl, *sibr;



    while (GET_PARENT(x)!=NULL && IS_BLACK(x)) {
        p = GET_PARENT(x);
        WRITE_OBJ(p, sizeof(node_t));
        pl = GET_LEFT(p);
        if (x == pl) {
            sib = GET_RIGHT(p);
            WRITE_OBJ(sib, sizeof(node_t));
            if (IS_RED(sib)) {
                SET_COLOUR(sib, BLACK);
                SET_COLOUR(p, RED);
                _left_rotate(Self, s, p);
                p = GET_PARENT(x);
                WRITE_OBJ(p, sizeof(node_t));
                sib = GET_RIGHT(p);
                WRITE_OBJ(sib, sizeof(node_t));
            }

            sibl = GET_LEFT(sib);
            READ_OBJ(sibl);
            sibr = GET_RIGHT(sib);
            READ_OBJ(sibr);
            if (IS_BLACK(sibl) && IS_BLACK(sibr)) {
                SET_COLOUR(sib,  RED);
                x = GET_PARENT(x);
                WRITE_OBJ(x, sizeof(node_t));
            } else {
                if (IS_BLACK(sibr)) {
                    WRITE_OBJ(sibl, sizeof(node_t));
                    SET_COLOUR(sibl, BLACK);
                    SET_COLOUR(sib, RED);
                    _right_rotate(Self, s, sib);
                    p = GET_PARENT(x);
                    WRITE_OBJ(p, sizeof(node_t));
                    sib = GET_RIGHT(p);
                }

                WRITE_OBJ(sib, sizeof(node_t));
                SET_COLOUR(sib, GET_COLOUR(p));
                WRITE_OBJ(p, sizeof(node_t));
                SET_COLOUR(p, BLACK);
                sibr=GET_RIGHT(sib);
                WRITE_OBJ(sibr, sizeof(node_t));
                SET_COLOUR(sibr, BLACK);
                _left_rotate(Self, s, p);
                READ_OBJ(s);
                x = GET_ROOT(s);
                VFY(s);
                WRITE_OBJ(x, sizeof(node_t));
                break;
            }
        } else  { // inverse
            sib = pl;
            WRITE_OBJ(sib, sizeof(node_t));
            if (IS_RED(sib)) {
                SET_COLOUR(sib, BLACK);
                SET_COLOUR(p, RED);
                _right_rotate(Self, s, p);
                p = GET_PARENT(x);
                WRITE_OBJ(p, sizeof(node_t));
                sib = GET_LEFT(p);
                WRITE_OBJ(sib, sizeof(node_t));
            }

            sibl = GET_LEFT(sib);
            READ_OBJ(sibl);
            sibr = GET_RIGHT(sib);
            READ_OBJ(sibr);
            if (IS_BLACK(sibl) && IS_BLACK(sibr)) {
                SET_COLOUR(sib, RED);
                x = GET_PARENT(x);
                WRITE_OBJ(x, sizeof(node_t));
            } else {
                if (IS_BLACK(sibl)) {
                    WRITE_OBJ(sibr, sizeof(node_t));
                    SET_COLOUR(sibr, BLACK);
                    SET_COLOUR(sib, RED);
                    _left_rotate(Self, s, sib);
                    p = GET_PARENT(x);
                    WRITE_OBJ(p, sizeof(node_t));
                    sib = GET_LEFT(p);
                }

                WRITE_OBJ(sib, sizeof(node_t));
                SET_COLOUR(sib, GET_COLOUR(p));
                WRITE_OBJ(p, sizeof(node_t));
                SET_COLOUR(p, BLACK);
                sibl = GET_LEFT(sib);
                WRITE_OBJ(sibl, sizeof(node_t));
                SET_COLOUR(sibl, BLACK);
                _right_rotate(Self, s, p);
                READ_OBJ(s);
                x = GET_ROOT(s);
                VFY(s);
                WRITE_OBJ(x, sizeof(node_t));
                break;
            }
        }
    }

    if(IS_RED(x)) {
        SET_COLOUR(x, BLACK);
    }
}//fix_after_deletion


// Return the left most node in the tree
static node_t * _first_node_non_stm (Thread * Self, set_t * s) {
    node_t * p;



    p = s->root;

    if (p != NULL) {

        while (p->l != NULL) {
            p = p->l;

        }
    }
    return p;
}//_first_node_non_stm

// Return the given node's successor node---the node which has the
// next key in the the left to right ordering. If the node has
// no successor, a null pointer is returned rather than a pointer to
// the nil node.
static node_t * _successor(Thread * Self, node_t * t) {
    node_t * p, *pl, *ch;


    if (t == NULL) {
        return NULL;
    } else {
        READ_OBJ(t);
        if (GET_RIGHT(t) != NULL) {
            p = GET_RIGHT(t);
            VFY(t);
            READ_OBJ(p);
            while ((pl = GET_LEFT(p)) != NULL) {
                VFY(p);
                p = pl;
                READ_OBJ(p);
            }
            VFY(p);
            return p;
        } else {//GET_RIGHT(t) == NULLL
            p = GET_PARENT(t);
            VFY(t);
            READ_OBJ(p);
            ch = t;

            while (p != NULL && ch == GET_RIGHT(p)) {
                ch = p;
                p = GET_PARENT(p);
                VFY(ch);
                READ_OBJ(p);
            }
            VFY(p);

            return p;
        }
    }
}

static node_t * _successor_non_stm(Thread * Self, node_t * t) {
    node_t * p, *pl, *ch;


    if (t == NULL) {
        return NULL;
    } else {
        if (t->r != NULL) {
            p = t->r;
            while ((pl = p->l) != NULL) {
                p = pl;
            }
            return p;
        } else {//GET_RIGHT(t) == NULLL
            p = t->p;
            ch = t;

            while (p != NULL && ch == p->r) {
                ch = p;
                p = p->p;
            }
            return p;
        }
    }
}

static node_t * _lookup (Thread * Self, set_t * s, setkey_t k) {
    node_t * p;

    READ_OBJ(s);
    p = GET_ROOT(s);
    VFY(s);

    while (p != NULL) {
        READ_OBJ(p);
        //vfy_assert(GET_KEY(p)+100==GET_VALUE(p));
        int cmp = k - GET_KEY(p);
        if (cmp == 0) {
            VFY(p);
            return p;
        }
#if defined PARTIAL_VFY && defined OBJ_STM
        node_t * p_tmp=p;
#endif

        p = (cmp < 0) ? GET_LEFT(p) : GET_RIGHT(p);
        VFY(p_tmp);
    }
    return NULL;
}


static node_t *  _insert (Thread * Self, set_t * s, setkey_t k, setval_t v, node_t * new) {
    node_t *x, *p;
    int cmp = 0;

    READ_OBJ(s);
    x = GET_ROOT(s);
    VFY(s);
    p = NULL;

    if (x == NULL) {
        //insert at the root
        WRITE_OBJ(new, sizeof(node_t));
        SET_KEY(new, k);
        SET_COLOUR(new, BLACK);
        SET_VALUE(new, v);
        SET_LEFT(new, NULL);
        SET_RIGHT(new, NULL);
        SET_PARENT(new, NULL);

        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s, new);
        return NULL;
    }

    while(x!=NULL) // find place to insert
    {
        READ_OBJ(x);
        //vfy_assert(GET_KEY(x)+100==GET_VALUE(x));

        cmp = k - GET_KEY(x);
        if (cmp == 0) {
            VFY(x);
            break;
        } else if (cmp < 0) {
            p = x;
            x = GET_LEFT(x);
            VFY(p);
        } else { // cmp > 0
            p = x;
            x = GET_RIGHT(x);
            VFY(p);
        }
    }
    //vfy_assert((x==NULL) == (cmp!=0));

    WRITE_OBJ(x, sizeof(node_t));// update leaf ptr x/xb
    if (cmp == 0) {
        SET_VALUE(x,v);
        return x;
    }

    // new insertion.
    WRITE_OBJ(new, sizeof(node_t));
    SET_PARENT(new, p);
    SET_LEFT(new, NULL);
    SET_RIGHT(new, NULL);
    SET_KEY(new, k);
    SET_VALUE(new, v);
    //SET_COLOUR(new, BLACK); // fixAfterInsertion() will set RED

    WRITE_OBJ(p, sizeof(node_t));
    if (cmp < 0) {
        SET_LEFT(p, new);
    } else {
        SET_RIGHT(p, new);
    }

    _fix_after_insertion(Self, s,new);
    return NULL;
}

static node_t * _delete (Thread * Self, set_t * s, node_t * x) {
    node_t *suc, *l, *r, *p, *rep;


    READ_OBJ(x);
    l = GET_LEFT(x);
    r = GET_RIGHT(x);
    VFY(x);
    if (l != NULL && r != NULL) {
        // x has 2 children
        suc = _successor (Self, x);
        READ_OBJ(suc);
        WRITE_OBJ(x, sizeof(node_t));

        SET_KEY(x, GET_KEY(suc));
        SET_VALUE(x, GET_VALUE(suc));

        x = suc;
        l = GET_LEFT(x);
        r = GET_RIGHT(x);
        VFY(x);
    }

    //    vfy_assert(l==NULL || r==NULL);

    rep = (l != NULL) ? l : r;
    WRITE_OBJ(rep, sizeof(node_t));

    if (rep != NULL) {
        p = GET_PARENT(x);
        VFY(x);
        WRITE_OBJ(p, sizeof(node_t));
        SET_PARENT (rep, p);
        if (p == NULL) {
            WRITE_OBJ(s, sizeof(set_t));
            SET_ROOT(s, rep);
        } else if (x == GET_LEFT(p)) {
            SET_LEFT(p, rep);
        } else {
            SET_RIGHT(p, rep);
        }

        // Null out links so they are OK to use by fixAfterDeletion.
        WRITE_OBJ(x, sizeof(node_t));
        SET_LEFT(x, NULL);
        SET_RIGHT(x, NULL);
        SET_PARENT(x, NULL);

        // Fix replacement
        if (IS_BLACK(x)) {
            _fix_after_deletion(Self, s, rep);
        }

    } else if (GET_PARENT(x) == NULL) { // return if we are the only node.
        VFY(x);
        WRITE_OBJ(s, sizeof(set_t));
        SET_ROOT(s, NULL);
    } else { //  No children. Use self as phantom replacement and unlink.
        if (IS_BLACK(x)) {
            WRITE_OBJ(x, sizeof(node_t));
            _fix_after_deletion(Self, s,x);
        }

        p = GET_PARENT(x);
        VFY(x);
        if (p != NULL) {
            WRITE_OBJ(p, sizeof(node_t));
            if (x == GET_LEFT(p)) {
                SET_LEFT(p, NULL);
            } else if (x == GET_RIGHT(p)) {
                SET_RIGHT(p, NULL);
            }
            WRITE_OBJ(x, sizeof(node_t));
            SET_PARENT(x, NULL);
        }
    }
    return x;
}


// ========================[API and Accessors]========================


void kv_init() {}




void *kv_create() {
    set_t * n = (set_t * ) malloc (sizeof(*n));
    n->root = NULL;
    return n;
}

#include <trace.h>

// Search for key k on set s
setval_t kv_get (Thread * Self, void  * s, setkey_t k) {
    setval_t v;
    //TraceEvent(Self->UniqID, 0, _tl_kv_get, NULL, k, 0);
    for (;;) {
        TXSTART (Self, 1);
				/**********/
				trace_log_tx_start(2);
				/**********/
        node_t * n = _lookup(Self, s, k);
        if (n != NULL) {
            READ_OBJ(n);
            v = GET_VALUE(n);
            VFY(n);

            if (TXCOMMIT(Self)) {
                assert(v==k+100);
                return v;
            }
            total_cf++;
        } else {
            if (TXCOMMIT(Self)) {
                return 0;
            }
            total_cf++;
        }
    }
    return v;
}

#ifdef __USE_TX_HANDLER__
#define REGISTER_PRE_ABORT(_f,_a) _ctl_register_pre_abort_handler(Self,(_f),(_a))

void __free_node__(Thread *Self, void *args) {
#ifdef _DO_FREE_
	free(args);
#endif
}

#endif

// puts even if already present
// returns 0 if key was not present
// returns 1 if key was already present
int kv_put (Thread * Self, void * s, setkey_t k, setval_t v) {
#ifndef __USE_TX_HANDLER__
    node_t * nn = _new_node(Self);
#endif
    //vfy_assert(v==k+100);
    //TraceEvent(Self->UniqID, 0, _tl_kv_put, NULL, k, v);

    for (;;) {
        TXSTART (Self, 0);
/**********/
				trace_log_tx_start(0);
				/**********/

#ifdef __USE_TX_HANDLER__
    		node_t * nn = _new_node(Self);
				REGISTER_PRE_ABORT(__free_node__, nn);		
#endif

        node_t * ex = _insert (Self, s, k, v, nn);
        if (ex != NULL) {
            if (TXCOMMIT(Self)) {
                _release_node (Self, nn);
                return 1;
            }
            total_cf++;

        } else {
            if (TXCOMMIT(Self)) {
                return 0;
            }
            total_cf++;

        }
    }
}

#ifdef __USE_TX_HANDLER__
#define REGISTER_POS_COMMIT(_f,_a) _ctl_register_pos_commit_handler(Self,(_f),(_a))
#endif


// Remove key k on set s
int kv_delete(Thread * Self, void *s, setkey_t k) {
    node_t * n = NULL;
    //TraceEvent(Self->UniqID, 0, _tl_kv_delete, NULL, k, 0);
    for (;;) {
        TXSTART (Self, 0);
/**********/
				trace_log_tx_start(1);
				/**********/

        n = _lookup(Self, s, k);
        if (n != NULL) {
            n = _delete(Self, s, n);
#ifdef __USE_TX_HANDLER__
#ifdef _DO_FREE_
						REGISTER_POS_COMMIT(_release_node, n);
#endif
#endif
        }
        if (TXCOMMIT(Self)) {
            break;
        }
        total_cf++;

    }
#ifndef __USE_TX_HANDLER__
    if (n != NULL)
        _release_node(Self, n);
#endif
    return (n != NULL);
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Diagnostic section
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


// Compute the BH (BlackHeight) and validate the tree.
//
// This function recursively verifies that the given binary subtree satisfies
// three of the red black properties. It checks that every red node has only
// black children. It makes sure that each node is either red or black. And it
// checks that every path has the same count of black nodes from root to leaf.
// It returns the blackheight of the given subtree; this allows blackheights to
// be computed recursively and compared for left and right siblings for
// mismatches. It does not check for every nil node being black, because there
// is only one sentinel nil node. The return value of this function is the
// black height of the subtree rooted at the node ``root'', or zero if the
// subtree is not red-black.
//


/*
// Return the left most node in the tree
static node_t * _first_node (Thread * Self, set_t * s) {
    node_t * p;
    READ_OBJ(s);
    p = GET_ROOT(s);
    READ_OBJ(p);
    if (p != NULL) {
        while (GET_LEFT(p) != NULL) {
            p = GET_LEFT(p);
            READ_OBJ(p);
        }
    }
    return p;
}//_first_node
 
 
static int _verify_redblack (Thread * Self, node_t * root, int depth) {
    int  height_left, height_right;
 
    if (root == NULL)
        return 1;
 
    READ_OBJ(root);
 
    setkey_t kkk =GET_KEY(root);
    setval_t vvv = GET_VALUE(root);
    if(!TXVALID(Self))
        return -1;
    vfy_assert(kkk+100==vvv);
    height_left  = _verify_redblack(Self, GET_LEFT(root), depth+1);
    if(!TXVALID(Self))
        return -1;
    height_right = _verify_redblack(Self, GET_RIGHT(root), depth+1);
    if(!TXVALID(Self))
        return -1;
    if (height_left == 0 || height_right == 0)
        return 0;
    if (height_left != height_right) {
        assert(0);
        printf (" Imbalance @depth=%d : %d %d\n", depth, height_left, height_right);
        if (0)
            return 0;
    }
 
    if (GET_LEFT(root) != NULL && GET_PARENT(GET_LEFT(root)) != root) {
        if(!TXVALID(Self))
            return -1;
        assert(0);
        printf (" lineage\n");
    }
    if (GET_RIGHT(root) != NULL && GET_PARENT(GET_RIGHT(root)) != root) {
        if(!TXVALID(Self))
            return -1;
        assert(0);
        printf (" lineage\n");
    }
 
    // Red-Black alternation
    if (GET_COLOUR(root) == RED) {
        if (GET_LEFT(root) != NULL && GET_COLOUR(GET_LEFT(root)) != BLACK) {
            if(!TXVALID(Self))
                return -1;
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        if (GET_RIGHT(root) != NULL && GET_COLOUR(GET_RIGHT(root)) != BLACK) {
            if(!TXVALID(Self))
                return -1;
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        return height_left;
    }
    if (GET_COLOUR(root) != BLACK) {
        if(!TXVALID(Self))
            return -1;
        printf ("VERIFY %d\n", __LINE__);
        return 0;
    }
    return height_left + 1;
}
 
// Verify or validate the RB tree.
 
int kv_verify (Thread * Self, void * s, int Verbose) {
    int vfy=-1;
    for (;;) {
        TXSTART (Self, 0);
        READ_OBJ(s);
        node_t * root = GET_ROOT(s);
        if(!TXVALID(Self))
            continue;
        if (root == NULL) {
            if(TXCOMMIT(Self)) {
                return 1;
            } else {
                total_cf++;
 
                continue;
            }
        }
 
        if (Verbose) {
            if(!TXVALID(Self))
                continue;
            printf ("Integrity check: ");
        }
 
        if (0) {
            if(!TXVALID(Self))
                continue;
            assert(0);
            printf ("  root=%lX: key=%ld color=%ld\n", (long)root, (long)GET_KEY(root), (long)GET_COLOUR(root));
        }
 
        READ_OBJ(root);
        if (GET_PARENT(root) != NULL) {
            if(!TXVALID(Self))
                continue;
            assert(0);
            printf ("  (WARNING) root %lX parent=%lX\n", (long)root, (long)GET_PARENT(root));
            if(TXCOMMIT(Self)) {
                return -1;
            } else {
                total_cf++;
                continue;
            }
        }
 
        if (GET_COLOUR(root) != BLACK) {
            if(!TXVALID(Self))
                continue;
            assert(0);
            printf ("  (WARNING) root %lX color=%lX\n", (long)root, (long)GET_COLOUR(root));
        }
        if(!TXVALID(Self))
            continue;
 
        // Weak check of binary-tree property
        int ctr = 0;
        node_t * its = _first_node(Self, s);
        while (its != NULL) {
            if(!TXVALID(Self))
                break;
            ctr ++;
            READ_OBJ(its);
            node_t * child = GET_LEFT(its);
            READ_OBJ(child);
            if(!TXVALID(Self))
                continue;
            if (child != NULL && GET_PARENT(child) != its) {
                if(!TXVALID(Self))
                    continue;
                assert(0);
                printf ("Bad parent\n");
            }
            if(!TXVALID(Self))
                continue;
            child = GET_RIGHT(its);
            if(!TXVALID(Self))
                continue;
 
            READ_OBJ(child);
            if (child != NULL && GET_PARENT(child) != its) {
                if(!TXVALID(Self))
                    continue;
                assert(0);
                printf ("Bad parent\n");
            }
            if(!TXVALID(Self))
                continue;
            node_t * nxt = _successor (Self, its);
            if(!TXVALID(Self))
                continue;
            if (nxt == NULL)
                break;
            READ_OBJ(nxt);
            if (GET_KEY(its) >= GET_KEY(nxt)) {
                if(!TXVALID(Self))
                    continue;
                assert(0);
                printf ("Key order %lX (%ld %ld) %lX (%ld %ld)\n",
                        (long)its, (long)GET_KEY(its), (long)GET_VALUE(its), (long)nxt, (long)GET_KEY(nxt), (long)GET_VALUE(nxt));
                return -3;
            }
            its = nxt;
        }
        if(!TXVALID(Self))
            continue;
 
        vfy = _verify_redblack (Self, root, 0);
        if(!TXVALID(Self))
            continue;
        if (Verbose) {
            printf (" Nodes=%d Depth=%d\n", ctr, vfy);
        }
        if (TXCOMMIT(Self)) {
            break;
        }
        total_cf++;
    }
    printf("total commit failed %d\n", total_cf);
 
    return vfy;
}
 
*/

static int _verify_redblack_non_stm (Thread * Self, node_t * root, int depth) {
    int  height_left, height_right;

    if (root == NULL)
        return 1;

    setkey_t kkk =root->k;
    setval_t vvv = root->v;
    assert(kkk+100==vvv);
    height_left  = _verify_redblack_non_stm(Self, root->l, depth+1);
    height_right = _verify_redblack_non_stm(Self, root->r, depth+1);
    if (height_left == 0 || height_right == 0) {
        return 0;
    }
    if (height_left != height_right) {
        assert(0);
    }

    if (root->l != NULL && root->l->p != root) {
        assert(0);
    }
    if (root->r != NULL && root->r->p != root) {
        assert(0);
    }

    // Red-Black alternation
    if (root->c == RED) {
        if (root->l != NULL && root->l->c != BLACK) {
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        if (root->r != NULL && root->r->c != BLACK) {
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        return height_left;
    }
    if (root->c != BLACK) {
        printf ("VERIFY %d\n", __LINE__);
        return 0;
    }
    return height_left + 1;
}

// Verify or validate the RB tree.

int kv_verify (Thread * Self, void * s, int Verbose) {
    int vfy=-1;
    node_t * root = ((set_t*)s)->root;
    if (root == NULL) {
        return 1;
    }

    if (Verbose) {
        printf ("Integrity check: ");
    }

    if (root->p != NULL) {
        assert(0);
    }

    if (root->c != BLACK) {
        assert(0);
    }

    // Weak check of binary-tree property
    int ctr = 0;
    node_t * its = _first_node_non_stm(Self, s);
    while (its != NULL) {
        ctr ++;

        node_t * child = its->l;
        if (child != NULL && child->p != its) {
            assert(0);
        }

        child = its->r;

        if (child != NULL && child->p != its) {
            assert(0);
        }
        node_t * nxt = _successor_non_stm (Self, its);
        if (nxt == NULL)
            break;
        if (its->k >= nxt->k) {
            assert(0);
        }
        its = nxt;
    }

    vfy = _verify_redblack_non_stm (Self, root, 0);
    return vfy;
}













