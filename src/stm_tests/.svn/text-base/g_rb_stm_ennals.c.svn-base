#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "portable_defns.h"
#include "gc.h"
#include "stm.h"
#include "g_rb_stm_ennals.h"


//------------------
unsigned int total_cf=0;



// x must be already open for writing
// x must have a parent
static stm_blk* _new_node(ptst_t *ptst) {
    stm_blk * newb;
    newb = new_stm_blk__(ptst, MEMORY);
    return newb;
}


static void _release_node(ptst_t *ptst, stm_blk * b) {
    free_stm_blk__(ptst, MEMORY, b);
}

// x must be already open for writing
// x must have a parent
static void _left_rotate(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk *xb, node_t *x) {
    stm_blk *rb, *rlb, *pb;
    node_t *r, *rl, *p;

    rb = GET_RIGHT(x);
    pb = GET_PARENT(x);

    r = WRITE_OBJ(rb);
    p = WRITE_OBJ(pb);
    rlb = GET_LEFT(r);

    x = WRITE_OBJ(xb);
    SET_RIGHT(x, rlb);
    if ( rlb != NULL ) {
        rl = WRITE_OBJ(rlb);
        SET_PARENT(rl, xb);
    }

    SET_PARENT(r, pb);
    if(pb == NULL) {
        set_t * s = (set_t*) WRITE_OBJ(sb);
        SET_ROOT(s, rb);
    } else if ( GET_LEFT(p) == xb ) {
        SET_LEFT(p, rb);
    } else {
        SET_RIGHT(p, rb);
    }
    SET_LEFT(r, xb);
    SET_PARENT(x, rb);
}//left_rotate

// x must be already open for writing
// x must have a parent
static void _right_rotate(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk *xb, node_t *x) {
    stm_blk *lb, *lrb, *pb;
    node_t *l, *lr, *p;

    lb = GET_LEFT(x);
    pb = GET_PARENT(x);

    l = WRITE_OBJ(lb);
    p = WRITE_OBJ(pb);
    lrb = GET_RIGHT(l);

    x = WRITE_OBJ(xb);
    SET_LEFT(x, lrb);
    if (lrb != NULL ) {
        lr = WRITE_OBJ(lrb);
        SET_PARENT(lr, xb);
    }

    SET_PARENT(l, pb);
    if(pb == NULL) {
        set_t * s = (set_t*) WRITE_OBJ(sb);
        SET_ROOT(s, lb);
    } else if ( xb == GET_RIGHT(p) ) {
        SET_RIGHT(p, lb);
    } else {
        SET_LEFT(p, lb);
    }
    SET_RIGHT(l, xb);
    SET_PARENT(x, lb);
}//right_rotate


static void _fix_after_insertion(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk *xb, node_t *x) {
    stm_blk *pb, *gpb, *yb, *lub;
    node_t  *p, *gp, *y;

    SET_COLOUR(x, RED);
    // rebalance tree
    set_t * s = (set_t*)READ_OBJ(sb);
    while (xb != NULL && GET_PARENT(x) != NULL) {
        pb = GET_PARENT(x);
        p = READ_OBJ(pb);
        if (IS_BLACK(p)) // case 2 - parent is black
            break;

        gpb = GET_PARENT(p);
        gp = READ_OBJ(gpb);
        lub = GET_LEFT(gp);

        if (pb == lub) {
            // parent is red, p=GET_LEFT(g)
            yb = GET_RIGHT(gp); // y (uncle)
            y  = READ_OBJ(yb);
            if (IS_RED(y)) {
                // case 3 - parent is red, uncle is red (p = GET_LEFT(gp))
                p = WRITE_OBJ(pb);
                y = WRITE_OBJ(yb);
                gp = WRITE_OBJ(gpb);
                SET_COLOUR(p, BLACK);
                SET_COLOUR(y, BLACK);
                SET_COLOUR(gp, RED);
                xb = gpb;
                x  = gp;
            } else {
                // parent is red, uncle is black (p = GET_LEFT(gp))
                if ( xb == GET_RIGHT(p) ) {
                    // case 4 - parent is red, uncle is black, x = GET_RIGHT(p), p = GET_LEFT(gp)
                    xb = pb;
                    x  = WRITE_OBJ(pb);
                    _left_rotate(ptst, tx, sb, xb, x);
                    pb=GET_PARENT(x);
                }
                // case 5 - parent is red, uncle is black, x = GET_LEFT(p), p = GET_LEFT(gp)
                p  = WRITE_OBJ(pb);
                gpb = GET_PARENT(p);
                gp  = WRITE_OBJ(gpb);
                SET_COLOUR(p, BLACK);
                SET_COLOUR(gp, RED);
                if (gp != NULL) {
                    _right_rotate(ptst, tx, sb, gpb, gp);
                }
            }
        } else {
            // parent is red, p = GET_RIGHT(gp)
            yb = lub;
            y  = READ_OBJ(yb);
            if (IS_RED(y)) {
                // case 3 - parent is red, uncle is red (p = GET_RIGHT(gp))
                p = WRITE_OBJ(pb);
                y = WRITE_OBJ(yb);
                gp = WRITE_OBJ(gpb);
                SET_COLOUR(p, BLACK);
                SET_COLOUR(y, BLACK);
                SET_COLOUR(gp, RED);
                xb = gpb;
                x  = gp;
            } else {
                // parent is red, uncle is black (p = GET_RIGHT(gp))
                if ( xb == GET_LEFT(p) ) {
                    // case 4 - parent is red, uncle is black, x = GET_LEFT(p), p = GET_RIGHT(gp)
                    xb = pb;
                    x  = WRITE_OBJ(pb);
                    _right_rotate(ptst, tx, sb, xb, x);
                    pb = GET_PARENT(x);
                }
                // case 5 - parent is red, uncle is black, x = GET_RIGHT(p), p = GET_RIGHT(gp)
                p  = WRITE_OBJ(pb);
                gpb = GET_PARENT(p);
                gp  = WRITE_OBJ(gpb);
                SET_COLOUR(p, BLACK);
                SET_COLOUR(gp, RED);
                if(gp != NULL) {
                    _left_rotate(ptst, tx, sb, gpb, gp);
                }
            }
        }
    }

    s = (set_t*)READ_OBJ(sb);
    stm_blk * rob = GET_ROOT(s);
    node_t * ro = READ_OBJ(rob);
    if (IS_RED(ro)) {
        ro = WRITE_OBJ(rob);
        SET_COLOUR(ro,BLACK);
    }
}

static void _fix_after_deletion(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk *xb, node_t *x) {
    stm_blk *pb, *plb, *sibb, *siblb, *sibrb;
    node_t *p, *sib, *sibl, *sibr;
    set_t *s;

    while (GET_PARENT(x)!=NULL && IS_BLACK(x)) {
        pb = GET_PARENT(x);
        p  = WRITE_OBJ(pb);
        plb = GET_LEFT(p);
        if ( xb == plb ) {
            sibb = GET_RIGHT(p);
            sib  = WRITE_OBJ(sibb);
            if (IS_RED(sib)) {
                SET_COLOUR(sib, BLACK);
                SET_COLOUR(p, RED);
                _left_rotate(ptst, tx, sb, pb, p);
	            pb = GET_PARENT(x);
                p=WRITE_OBJ(pb);
                sibb = GET_RIGHT(p);
                sib  = WRITE_OBJ(sibb);
            }

            siblb = GET_LEFT(sib);
            sibl  = READ_OBJ(siblb);
            sibrb = GET_RIGHT(sib);
            sibr  = READ_OBJ(sibrb);
            if (IS_BLACK(sibl) && IS_BLACK(sibr)) {
                SET_COLOUR(sib, RED);
                xb = GET_PARENT(x);
                x  = WRITE_OBJ(xb);
            } else {
                if (IS_BLACK(sibr)) {
                    sibl = WRITE_OBJ(siblb);
                    SET_COLOUR(sibl, BLACK);
                    SET_COLOUR(sib,RED);
                    _right_rotate(ptst, tx, sb, sibb, sib);
                    pb = GET_PARENT(x);
                    p  = WRITE_OBJ(pb);
                    sibb = GET_RIGHT(p);
                }

                sib  = WRITE_OBJ(sibb);
                SET_COLOUR(sib, GET_COLOUR(p));
                p  = WRITE_OBJ(pb);
                SET_COLOUR(p, BLACK);
                sibrb = GET_RIGHT(sib);
                sibr  = WRITE_OBJ(sibrb);
                SET_COLOUR(sibr, BLACK);
                _left_rotate(ptst, tx, sb, pb, p);
                s = (set_t*)READ_OBJ(sb);
                xb = GET_ROOT(s);
                x  = WRITE_OBJ(xb);
                break;
            }
        } else  { // inverse
            sibb = plb;
            sib  = WRITE_OBJ(sibb);
            if (IS_RED(sib)) {
                SET_COLOUR(sib, BLACK);
                SET_COLOUR(p, RED);
                _right_rotate(ptst, tx, sb, pb, p);
                pb = GET_PARENT(x);
                p=WRITE_OBJ(pb);
                sibb = GET_LEFT(p);
                sib  = WRITE_OBJ(sibb);
            }

            siblb = GET_LEFT(sib);
            sibl  = READ_OBJ(siblb);
            sibrb = GET_RIGHT(sib);
            sibr  = READ_OBJ(sibrb);
            if (IS_BLACK(sibl) && IS_BLACK(sibr)) {
                SET_COLOUR(sib, RED);
                xb = GET_PARENT(x);
                x  = WRITE_OBJ(xb);
            } else {
                if (IS_BLACK(sibl)) {
                    sibr = WRITE_OBJ(sibrb);
                    SET_COLOUR(sibr, BLACK);
                    SET_COLOUR(sib, RED);
                    _left_rotate(ptst, tx, sb, sibb, sib);
                    pb = GET_PARENT(x);
                    p  = WRITE_OBJ(pb);
                    sibb = GET_LEFT(p);
                }

                sib = WRITE_OBJ(sibb);
                SET_COLOUR(sib, GET_COLOUR(p));
                p  = WRITE_OBJ(pb);
                SET_COLOUR(p, BLACK);
                siblb = GET_LEFT(sib);
                sibl = WRITE_OBJ(siblb);
                SET_COLOUR(sibl, BLACK);
                _right_rotate(ptst, tx, sb, pb, p);
                s = (set_t*)READ_OBJ(sb);
                xb = GET_ROOT(s);
                x  = WRITE_OBJ(xb);
                break;
            }
        }
    }

    if(IS_RED(x)) {
        SET_COLOUR(x, BLACK);
    }
}//fix_after_deletion


// Return the left most node in the tree
static stm_blk * _first_node_non_stm (ptst_t *ptst, stm_tx *tx, stm_blk *sb) {
    stm_blk *pb;
    node_t *p;

    set_t * s = (set_t*) init_stm_blk__(ptst, MEMORY, sb);
    pb = GET_ROOT(s);

    if(pb != NULL) {
        p = init_stm_blk__(ptst, MEMORY, pb);
        while ( GET_LEFT(p) != NULL ) {
            pb = GET_LEFT(p);
            p = init_stm_blk__(ptst, MEMORY, pb);
        }
    }
    return pb;
}//_first_node_non_stm

// Return the given node's successor node---the node which has the
// next key in the the left to right ordering. If the node has
// no successor, a null pointer is returned rather than a pointer to
// the nil node.
static stm_blk * _successor(ptst_t *ptst, stm_tx *tx, stm_blk * tb) {
    stm_blk *chb, *pb, *plb;
    node_t *t, *ch, *p;

    if(tb == NULL) {
        return NULL;
    } else {
        t = READ_OBJ(tb);
        if(GET_RIGHT(t) != NULL) {
            pb = GET_RIGHT(t);

            p = READ_OBJ(pb);
            while ( (plb=GET_LEFT(p)) != NULL ) {

                pb = plb;
                p = READ_OBJ(pb);
            }

            return pb;
        } else {//GET_RIGHT(t) == NULLL
            pb = GET_PARENT(t);
            p = READ_OBJ(pb);
            chb = tb;
            ch = t;

            while(pb != NULL && chb == GET_RIGHT(p)) {
                chb = pb;
                ch = p;
                pb = GET_PARENT(p);
                p = READ_OBJ(pb);
            }

            return pb;
        }
    }
}

static stm_blk * _successor_non_stm(ptst_t *ptst, stm_tx *tx, stm_blk * tb) {
    stm_blk *chb, *pb, *plb;
    node_t *t, *ch, *p;

    if(tb == NULL) {
        return NULL;
    } else {
        t = init_stm_blk__(ptst, MEMORY, tb);
        if(GET_RIGHT(t) != NULL) {
            pb = GET_RIGHT(t);
            p = init_stm_blk__(ptst, MEMORY, pb);
            while ( (plb=GET_LEFT(p)) != NULL ) {
                pb = plb;
                p = init_stm_blk__(ptst, MEMORY, pb);
            }
            return pb;
        } else {//GET_RIGHT(t) == NULLL
            pb = GET_PARENT(t);
            p = init_stm_blk__(ptst, MEMORY, pb);
            chb = tb;
            ch = t;

            while(pb != NULL && chb == GET_RIGHT(p)) {
                chb = pb;
                ch = p;
                pb = GET_PARENT(p);
                p = init_stm_blk__(ptst, MEMORY, pb);
            }
            return pb;
        }
    }
}



static stm_blk * _lookup(ptst_t *ptst, stm_tx *tx, stm_blk *sb, setkey_t k) {
    stm_blk *pb;
    node_t  *p;

    set_t * s = (set_t*) READ_OBJ(sb);
    pb = GET_ROOT(s);
    while ( pb != NULL ) {
        p = READ_OBJ(pb);
        //assert(GET_KEY(p)+100==GET_VALUE(p));
        int cmp = k - GET_KEY(p);
        if (cmp == 0) {

            return pb;
        }
        pb = (cmp < 0) ? GET_LEFT(p) : GET_RIGHT(p);

    }
    return NULL;
}


static stm_blk * _insert(ptst_t *ptst, stm_tx *tx, stm_blk *sb, setkey_t k, setval_t v, stm_blk * newb) {
    stm_blk *xb, *pb;
    node_t  *x, *p, *new;
    int cmp = 0;

    set_t * s = (set_t*) READ_OBJ(sb);
    xb = GET_ROOT(s);
    pb = NULL;

    if(xb == NULL) {
        //insert at the root
        new = WRITE_OBJ(newb);
        SET_KEY(new, k);
        SET_COLOUR(new, BLACK);
        SET_VALUE(new, v);
        SET_LEFT(new, NULL);
        SET_RIGHT(new, NULL);
        SET_PARENT(new, NULL);

        s = (set_t*) WRITE_OBJ(sb);
        SET_ROOT(s, newb);
        return NULL;
    }

    while ( xb != NULL )// find place to insert
    {
        x = READ_OBJ(xb);
        //        assert(GET_KEY(x)+100==GET_VALUE(x));

        cmp = k - GET_KEY(x);
        if (cmp == 0) {

            break;
        } else if (cmp < 0) {
            pb = xb;
            xb = GET_LEFT(x);

        } else { // cmp > 0
            pb = xb;
            xb = GET_RIGHT(x);

        }
    }


//    if(!validate_stm_tx(ptst,tx)) {
//        return NULL;
//    }
//    assert((xb == NULL) == (cmp != 0));

    x = WRITE_OBJ(xb);// update leaf ptr x/xb
    if (cmp == 0) {
        SET_VALUE(x, v);
        return xb;
    }

    // new insertion.
    new = WRITE_OBJ(newb);
    SET_PARENT(new, pb);
    SET_LEFT(new, NULL);
    SET_RIGHT(new, NULL);
    SET_KEY(new, k);
    SET_VALUE(new, v);
    //SET_COLOUR(new, BLACK); // fixAfterInsertion() will set RED

    p = WRITE_OBJ(pb);
    if (cmp < 0) {
        SET_LEFT(p, newb);
    } else {
        SET_RIGHT(p, newb);
    }

    _fix_after_insertion(ptst, tx, sb, newb, new);
    return NULL;
}

static stm_blk * _delete(ptst_t *ptst, stm_tx *tx, stm_blk *sb, stm_blk * xb) {
    stm_blk *sucb, *lb, *rb, *pb, *repb;
    node_t *suc, *p, *rep, *x;

    x = READ_OBJ(xb);
    lb = GET_LEFT(x);
    rb = GET_RIGHT(x);

    if(lb != NULL && rb != NULL) {
        // x has 2 children
        sucb = _successor(ptst, tx, xb);
        suc= READ_OBJ(sucb);
        x = WRITE_OBJ(xb);

        SET_KEY(x, GET_KEY(suc));
        SET_VALUE(x, GET_VALUE(suc));

        xb = sucb;
        x = suc;
        lb = GET_LEFT(x);
        rb = GET_RIGHT(x);
    }

//    if(!validate_stm_tx(ptst,tx)) {
//        return NULL;
//    }
//    assert(lb == NULL || rb == NULL);

    repb = (lb != NULL) ? lb : rb;
    rep = WRITE_OBJ(repb);

    if(repb != NULL) {
        pb = GET_PARENT(x);
        p = WRITE_OBJ(pb);
        SET_PARENT (rep, pb);
        if (p == NULL) {
            set_t *s = (set_t*)WRITE_OBJ(sb);
            SET_ROOT(s, repb);
        } else if (xb == GET_LEFT(p)) {
            SET_LEFT(p, repb);
        } else {
            SET_RIGHT(p, repb);
        }

        // Null out links so they are OK to use by fixAfterDeletion.
        x = WRITE_OBJ(xb);
        SET_LEFT(x, NULL);
        SET_RIGHT(x, NULL);
        SET_PARENT(x, NULL);

        // Fix replacement
        if (IS_BLACK(x)) {
            _fix_after_deletion(ptst, tx, sb, repb, rep);
        }

    } else if (GET_PARENT(x) == NULL) { // return if we are the only node.
        set_t *s = (set_t*)WRITE_OBJ(sb);
        SET_ROOT(s, NULL);
    } else { //  No children. Use self as phantom replacement and unlink.
        if (IS_BLACK(x)) {
            x = WRITE_OBJ(xb);
            _fix_after_deletion(ptst, tx, sb, xb, x);
        }

        pb = GET_PARENT(x);
        if (pb != NULL) {
            p = WRITE_OBJ(pb);
            if (xb == GET_LEFT(p)) {
                SET_LEFT(p, NULL);
            } else if (xb == GET_RIGHT(p)) {
                SET_RIGHT(p, NULL);
            }
            x = WRITE_OBJ(xb);
            SET_PARENT(x, NULL);
        }
    }
    return xb;
}

void kv_init(void) {
    //node_t *null;
    ptst_t  *ptst = NULL;

    //-----------------------
    //memset(&shared, 0, sizeof(shared));
    _init_ptst_subsystem();
    _init_gc_subsystem();

    //-----------------------

    ptst = critical_enter();
    _init_stm_subsystem(0);
    MEMORY = new_stm(ptst, (int)sizeof(node_t));
    critical_exit(ptst);



}//kv_init

void *kv_create(void) {
    ptst_t  *ptst = NULL;
    stm_blk  *msetb;
    set_t  *mset;
    //    node_t *root;

    ptst = critical_enter();

    msetb = new_stm_blk__(ptst, MEMORY);
    mset = (set_t*)init_stm_blk__(ptst, MEMORY, msetb);
    SET_ROOT(mset, NULL);

    critical_exit(ptst);

    return msetb;
}//kv_create

// Search for key k on set s
setval_t kv_get(void *sb, setkey_t k) {
    ptst_t  *ptst = NULL;
    stm_tx  *tx = NULL;
    node_t  *n;
    stm_blk * nb;
    setval_t v;

    ptst = critical_enter();

    for (;;) {
        new_stm_tx(tx, ptst, MEMORY);
        nb = _lookup(ptst, tx, sb, k);
        if(nb != NULL) {
            n = READ_OBJ(nb);
            v = GET_VALUE(n);
            if(commit_stm_tx(ptst, tx)) {
                assert(v==k+100);
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

    return v;
}

// puts even if already present
// returns 0 if key was not present
// returns 1 if key was already present
int kv_put(void * sb, setkey_t k, setval_t v) {
    ptst_t  *ptst = NULL;
    stm_tx  *tx = NULL;

    ptst = critical_enter();

    stm_blk * newb = _new_node(ptst);

    for (;;) {
        new_stm_tx(tx, ptst, MEMORY);
        stm_blk * exb = _insert(ptst, tx, sb, k, v, newb);
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

    critical_exit(ptst);
}

// Remove key k on set s
int kv_delete(void *sb, setkey_t k) {
    ptst_t  *ptst = NULL;
    stm_tx  *tx = NULL;

    stm_blk * nb = NULL;

    ptst = critical_enter();

    for(;;) {
        new_stm_tx(tx, ptst, MEMORY);

        nb = _lookup(ptst, tx, sb, k);
        if (nb != NULL) {
            nb = _delete(ptst, tx, sb, nb);
        }
        if(commit_stm_tx(ptst, tx)) {
            break;
        }
        total_cf++;

    }
    if (nb != NULL)
        _release_node(ptst, nb);

    critical_exit(ptst);

    return (nb != NULL);
}// kv_delete



static int _verify_redblack (ptst_t *ptst, stm_tx *tx, stm_blk *rootb, int depth) {
    int  height_left, height_right;
    node_t * root, *rootl, *rootr;
    stm_blk *rootlb, *rootrb;

    if (rootb == NULL)
        return 1;

    root = init_stm_blk__(ptst, MEMORY, rootb);

    assert(GET_KEY(root)+100==GET_VALUE(root));

    height_left  = _verify_redblack(ptst, tx, GET_LEFT(root), depth+1);
    height_right = _verify_redblack(ptst, tx, GET_RIGHT(root), depth+1);
    if (height_left == 0 || height_right == 0)
        return 0;

    if (height_left != height_right) {
        printf (" Imbalance @depth=%d : %d %d\n", depth, height_left, height_right);
        if (0)
            return 0;
    }

    rootlb = GET_LEFT(root);
    rootl = init_stm_blk__(ptst, MEMORY, rootlb);
    rootrb = GET_RIGHT(root);
    rootr = init_stm_blk__(ptst, MEMORY, rootrb);

    if (rootl != NULL && GET_PARENT(rootl) != rootb) {
        printf (" lineage\n");
    }
    if (rootr != NULL && GET_PARENT(rootr) != rootb) {
        printf (" lineage\n");
    }

    // Red-Black alternation
    if (GET_COLOUR(root) == RED) {
        if (rootl != NULL && GET_COLOUR(rootl) != BLACK) {
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        if (rootr != NULL && GET_COLOUR(rootr) != BLACK) {
            printf ("VERIFY %d\n", __LINE__);
            return 0;
        }
        return height_left;
    }
    if (GET_COLOUR(root) != BLACK) {
        printf ("VERIFY %d\n", __LINE__);
        return 0;
    }
    return height_left + 1;
}
// Verify or validate the RB tree.

int kv_verify (void * sb, int Verbose) {
    ptst_t  *ptst = NULL;
    stm_tx  *tx = NULL;
    stm_blk *rootb = NULL;
    node_t  *root = NULL;
    set_t * s = NULL;
    int vfy = -1;

    ptst = critical_enter();

    s= (set_t*)init_stm_blk__(ptst, MEMORY, sb);
    rootb = GET_ROOT(s);

    if (rootb == NULL) {
        return 1;
    }

    if (Verbose) {
        printf ("Integrity check: ");
    }

    root = init_stm_blk__(ptst, MEMORY, rootb);
    if (0) {
        printf ("  root=%lX: key=%lu color=%ld\n", (long)rootb, (long)GET_KEY(root), (long)GET_COLOUR(root));
    }

    if (GET_PARENT(root) != NULL) {
        printf ("  (WARNING) root %lX parent=%lX\n", (long)rootb, (long)GET_PARENT(root));
        return -1;
    }
    if (GET_COLOUR(root) != BLACK) {
        printf ("  (WARNING) root %lX color=%lX\n", (long)rootb, (long)GET_COLOUR(root));
    }

    // Weak check of binary-tree property
    int ctr = 1;
    stm_blk * itsb = _first_node_non_stm(ptst, tx, sb);
    node_t * its = init_stm_blk__(ptst, MEMORY, itsb);
    stm_blk * childb = NULL;
    node_t * child = NULL;
    stm_blk * nxtb = NULL;
    node_t * nxt = NULL;

    while (itsb != NULL) {
        childb = GET_LEFT(its);
        child = init_stm_blk__(ptst, MEMORY, childb);
        if (childb != NULL && GET_PARENT(child) != itsb) {
            printf ("Bad parent\n");
        }
        childb = GET_RIGHT(its);
        child = init_stm_blk__(ptst, MEMORY, childb);
        if (childb != NULL && GET_PARENT(child) != itsb) {
            printf ("Bad parent\n");
        }

        nxtb = _successor_non_stm(ptst, tx, itsb);
        nxt = init_stm_blk__ (ptst, MEMORY, nxtb);
        if (nxtb == NULL)
            break;

        if (GET_KEY(its) >= GET_KEY(nxt)) {
            printf ("Key order %lX (%lu %ld) %lX (%lu %ld)\n",
                    (long)itsb, (long)GET_KEY(its), (long)GET_VALUE(its), (long)nxtb, (long)GET_KEY(nxt), (long)GET_VALUE(nxt));
            return -3;
        }

        itsb = nxtb;
        its = nxt;

        ctr ++;
    }

    vfy = _verify_redblack (ptst, tx, rootb, 0);
    if (Verbose) {
        printf (" Nodes=%d Depth=%d\n", ctr, vfy);
    }

    critical_exit(ptst);
    return vfy;
}


extern void kv_print (void * sb) {
    printf("tree\n");
    ptst_t  *ptst = NULL;
    stm_tx  *tx = NULL;
    stm_blk *rootb = NULL;
    node_t  *root = NULL;
    set_t * s = NULL;

    ptst = critical_enter();

    s= (set_t*)init_stm_blk__(ptst, MEMORY, sb);
    rootb = GET_ROOT(s);

    if (rootb == NULL) {
        return;
    }

    root = init_stm_blk__(ptst, MEMORY, rootb);

    int ctr = 0;
    stm_blk * itsb = _first_node_non_stm(ptst, tx, sb);
    node_t * its = init_stm_blk__(ptst, MEMORY, itsb);

    while (itsb != NULL) {
        if(ctr >= 890) {
            printf ("ctr:%d, %ld, %ld --- \n", ctr, (long)GET_KEY(its), (long)GET_VALUE(its));
        }
        itsb = _successor_non_stm (ptst, tx, itsb);
        its = init_stm_blk__ (ptst, MEMORY, itsb);
        ctr ++;
    }
    critical_exit(ptst);
}
