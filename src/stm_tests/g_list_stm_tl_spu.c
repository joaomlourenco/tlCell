#include <stdio.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <stdint.h>
#include <g_list_stm_tl_spu.h>
#include "tl_spu.h"

#include <sys/time.h>
#include <time.h>

//#include <spu_timer.h>


#include <ea.h>
#define mmap_func       mmap_ea
#define mmap_fail       MAP_FAILED

//variable pointer to List Structure.
unsigned long long pointerToData;


//variables that i need


//listStructure Pointer

controlStruct ctrStruct __attribute__((aligned(128))); //myStruct SPE_ALIGN_FULL;
set_t mySet __attribute__((aligned(128)));
//node_t root __attribute__((aligned(128)));
thread_stats myStats __attribute__((aligned(128)));
info infoStruct __attribute__((aligned(128)));
int can_stop;
uint64_t start,end, total_spe_time;
int id;
node_t tmpNode;
node_t nextNode;
node_t previousNode;
//spu-gcc -g -o list_stm_tl_spu -I./ -L./ g_list_stm_tl_spu.c libtl_spu.a


//#define MYRAND() rand_r(&(run_stats[thread_id].rand_seed))
#define MYRAND() rand();


#define GET_TIME() ({\
	unsigned long long _ull_time;\
	struct timeval ts;\
	gettimeofday(&ts, NULL);\
	_ull_time = (ts.tv_sec * 1.e6) + ts.tv_usec;\
	_ull_time;\
})


node_t * new_node() {
    node_t *new;
    new = (node_t *) malloc (sizeof(node_t));
    //memset (new, 0xFF, sizeof(node_t));
    return new;
}


node_t * lookup(setkey_t k) {
	int tag = 1, tag_mask = 1<<tag;
	node_t next;

	//mfc_get(&next,mySet.root,sizeof(node_t),tag,0,0);
	TxLoad((volatile *)&tmpNode,mySet.root, sizeof(node_t));
	node_t tmp = tmpNode;
	//check root
	if (tmpNode.k == k) { //found it
		if ( (int) tmpNode.v != ( (int) tmpNode.k + 100)) {
			printf("error: value != k+100\n");
			return NULL;
		}
		printf("#lookup# -> Found it, returning..\n");
		return &tmpNode;
	}
	while (tmpNode.n != 0) {

		if ( (int) tmpNode.k == (int )k) { //found it
			if ( (int)tmpNode.v != ( ((int) tmp.k) + 100)) {
				printf("error: value != k+100\n");
				return 0;
			}
			printf("#lookup# -> found the node\n");
			return &tmpNode;
		} else { //fetch next.
			//void TxLoad(volatile *destination, long long int address, int size)
			if(tmpNode.n==0) return 0;
			printf("#Lookup# fetching next->p=%llx \n", tmpNode.n);
			TxLoad((volatile *)&tmpNode, tmpNode.n, sizeof(node_t));
		}
	}
}

node_t * searchNode(setkey_t k) {

	int tag = 1, tag_mask = 1 << tag;
	node_t next;
	unsigned long long tmpNext;

	TxLoad((volatile *) &tmpNode, mySet.root, sizeof(node_t));
	node_t tmp = tmpNode;
	if (tmpNode.k == k) { //found it
		if ((int) tmpNode.v != ((int) tmpNode.k + 100)) {
			printf("error: value != k+100\n");
			return NULL;
		}
		printf("#lookup# -> Found it, returning..\n");
		return &tmpNode;
	}
	while ((int) tmpNode.n != (int) 0) {

			if ( (int) tmpNode.k == (int )k) { //found it
				if ( (int)tmpNode.v != ( ((int) tmp.k) + 100)) {
					printf("error: value != k+100\n");
					return 0;
				}
				printf("#lookup# -> found the node\n");
				return &tmpNode;
			} else { //fetch next.
				//void TxLoad(volatile *destination, long long int address, int size)
				if(tmpNode.n==0) return 0;
				TxLoad((volatile*) tmpNext, tmpNode.n, sizeof(unsigned long long));
				printf("#Lookup# fetching next->p=%llx \n", tmpNode.n);
				TxLoad((volatile *)&tmpNode, tmpNode.n, sizeof(node_t));
			}
		}

}


/*int kv_put(setkey_t k, setval_t v) {

	_node_t n = root;
	_node_t tmp = n;
	//check root

	if (n.k == k) { //found it
		if (n.v != (n.k + 100)) {
			printf("error: value != k+100\n");
			return 0;
		}
		return n;
	}

	while (tmp.n != null) {

		if (tmp.k == k) { //found it
			if (tmp.v != (tmp.k + 100)) {
				printf("error: value != k+100\n");
				return 0;
			}
			return tmp;
		} else { //fetch next.
			//void TxLoad(volatile *destination, long long int address, int size)
			TxLoad(&tmp, tmp.n, sizeof(_node_t));
		}
	}
	
	return 1;
}*/

int kv_delete(setkey_t k) {
	//node_t *node = lookup(k);
	node_t previous;
	node_t next;
	//unsigned long long CurrentNodePointer;


	node_t n;
	printf("Transfering root..kv_get..\n");
	TxLoad((volatile *) &n, mySet.root, sizeof(node_t));
	printf("root->k = %llx , root->v = %llx\n", n.k, n.v);
	printf(" root->n = %llx , root->p = %llx\n", n.n, n.p);
	node_t tmp = n;
	//check root
	if ((int) n.k == (int) k) { //found it
		if ((int) n.v != (int) (n.k + 100)) {
			printf("#kv_get#  error: found it but value != k+100. %d!=%d\n",
					(int) tmp.k, (int) k);
			return 0;
		}
		//TxLoad((volatile *) &previous, tmp.p, sizeof(node_t));
		if (tmp.n == 0) { //there is only the root (does this happen?)
			//previous.n = NULL;
			printf("##kv_delete## TxStore\n");
			TxStore((unsigned long long volatile) previous.n, (intptr_t) NULL);
		} else {
			printf("##kv_delete##Txstoring..\n");
			TxLoad((volatile *) &next, tmp.n, sizeof(node_t));
			//TxStore((unsigned long long volatile) previous.n, tmp.n);
			TxStore((unsigned long long volatile) next.p, tmp.p);
			//TxStore(setROOT);
			printf("##kv_delete##End of Txstoring..\n");
		}
	}
	while (tmp.n != 0) {

		//printf("#kv_get# (int) tmp->k = %d , tmp->v = %d\n", (int)tmp.k , (int) tmp.v);
		//printf("#kv_get#  tmp->n = %llx , tmp->p = %llx\n", tmp.n , tmp.p);
		if ((int) tmp.k == (int) k) { //found it
			printf("#KV_DELETE -> found it, deleting it#");
			TxLoad((volatile *) &previous, tmp.p, sizeof(node_t));
			TxLoad((volatile *) &next, tmp.n, sizeof(node_t));
			TxStore((unsigned long long volatile) previous.n, tmp.n);
			TxStore((unsigned long long volatile) next.p, tmp.p);
			printf("#KV_DELETE -> done... returning#");
			return 0;

	
		} else { //fetch next.
			//void TxLoad(volatile *destination, long long int address, int size)
			printf("#kv_delete# fetching next\n");
			if (tmp.n == 0)
				return -1;
			TxLoad((volatile *) &tmp, tmp.n, sizeof(node_t));
		}
	}///////////////////////////////////////////////////


	//printf("##kv_delete## Txloading previous node-> %llx \n", node->p);
	//TxLoad(&previous,node.p, sizeof(node_t));


}


setval_t kv_get(setkey_t k){
	
	node_t n;
	printf("Transfering root..kv_get..\n");
	TxLoad((volatile *)&n,mySet.root,sizeof(node_t));

	//printf("Transfered root.... \n");
	printf("root->k = %llx , root->v = %llx\n", n.k , n.v);
	//printf("(int) root->k = %d , root->v = %d\n", (int)n.k , (int) n.v);
	printf(" root->n = %llx , root->p = %llx\n", n.n , n.p);
	//printf("(intptr_t) root->k = %d , root->v = %d\n", (intptr_t)n.k , (intptr_t) n.v);
	//printf("(double) root->k = %d , root->v = %d\n", (double)n.k , (double) n.v);
	//printf("(unsigned long int) root->k = %ul , root->v = %ul\n", (unsigned long int)n.k , (unsigned long int) n.v);
	//printf("(unsigned int) root->k = %u , root->v = %u\n", (unsigned int)n.k , (unsigned int) n.v);
	node_t tmp = n;
	//check root
	if ( (int) n.k == (int) k) { //found it
		if ( (int)n.v != (int) (n.k + 100)) {
			printf("#kv_get#  error: found it but value != k+100. %d!=%d\n", (int) tmp.k, (int) k);
			return 0;
		}
		return (setval_t)(int) n.v;
	}
	while(tmp.n != 0){
		
		//printf("#kv_get# (int) tmp->k = %d , tmp->v = %d\n", (int)tmp.k , (int) tmp.v);
		//printf("#kv_get#  tmp->n = %llx , tmp->p = %llx\n", tmp.n , tmp.p);
		if ( (int) tmp.k== (int) k){ //found it
			if( ((int) tmp.v) != ((int) tmp.k + 100)) {
				printf(" #kv_get#  error: value (iteration) != k+100   (%d!=%d)\n", (int) tmp.k, (int) k);
				return 0;
			}
			printf("#kv_get# returning (int) val->%d", (int) tmp.v);

			return (setval_t) (int) tmp.v;
		}
		else{ //fetch next.
			//void TxLoad(volatile *destination, long long int address, int size)
			printf("#kv_get# fetching next\n");
			if(tmp.n==0) return 0;
			TxLoad((volatile *)&tmp, tmp.n,sizeof(node_t));
		}
	}
}

int kv_put(setkey_t k, setval_t v) {
	return 0;
}


//eaddr_t mmap_ea(eaddr_t start, size_t length, int prot, int flags, int fd, off_t offset)
// puts even if already present
// returns 0 if key was not present
// returns 1 if key was already present
/*inline int kv_put(void * sb, setkey_t k, setval_t v) {
	node_t * prev;
	node_t * next;
	node_t * tmp;

	node_t new = malloc(sizeof(node_t));
	//TxStore(&root.n, NULL);
	//TxStore(&root.p, NULL);
	//TxStore(&root.k, k);
	//TxStore(&root.v, v);
	root.n=NULL;
	root.p=NULL;
	root.k=k;
	root.v=v;
	//WRITE_OBJ(new, sizeof(node_t));

	//READ_OBJ(s);
	node_t * ptr = root; //isto ja foi feito TxLoad? hum.....
	//VFY(s);

	if (ptr == NULL) {
		//list is empty
		//WRITE_OBJ(s, sizeof(set_t));
		root = new;
		return NULL;
	}

	//TraceEvent(Self->UniqID, 10, _tl_kv_put, NULL, 0, 0);

	//--------- WTF?
	/*prev=GET_PREV(ptr);
	 VFY(ptr);
	 vfy_assert(prev==NULL);
	//---------

	while (ptr.n != NULL && ptr.k < key) {
		//        setkey_t kkk = GET_KEY(ptr);
		//        setval_t vvv = GET_VALUE(ptr);
		//        VFY(ptr);
		//        vfy_assert(kkk+100==vvv);


		//ptr=GET_NEXT(ptr);
		TxLoad(&ptr, ptr.n, sizeof(_node_t));
		//VFY(ptr_tmp);
		//READ_OBJ(ptr);
	}
	//VFY(ptr);

	//TraceEvent(Self->UniqID, 20, _tl_kv_put, NULL, 0, 0);
	//WRITE_OBJ(ptr, sizeof(node_t));

	//either list has only 1 node or the insertion will be on the first node
	if (key < ptr.k) {
		//TraceEvent(Self->UniqID, 30, _tl_kv_put, NULL, 0, 0);
		//ins before
		prev = ptr.p;
		if (ptr == root) {
			//TraceEvent(Self->UniqID, 40, _tl_kv_put, NULL, 0, 0);
			//VFY(s);
			//vfy_assert(prev==NULL);
			//WRITE_OBJ(s, sizeof(set_t));
			root = new;
			//SET_ROOT(s, new);
			root.p=NULL;
			//SET_PREV(new, NULL);

		} else {
			//TraceEvent(Self->UniqID, 50, _tl_kv_put, NULL, 0, 0);
			//VFY(s);
			//vfy_assert(prev != NULL);
			//WRITE_OBJ(prev, sizeof(node_t));
			TxStore(&prev.n, NULL);
			prev.n=new;
			new.p=prev;
			//SET_NEXT(prev, new);
			//SET_PREV(new, prev);
		}
		new.n=ptr;
		ptr.p=new;
		//SET_NEXT(new, ptr);
		//SET_PREV(ptr, new);

		//SET_KEY(new, key);
		//SET_VALUE(new, val);

		//        vfy_assert(GET_PREV(new)==NULL || GET_KEY(GET_PREV(new))<key);
		//        vfy_assert(GET_NEXT(new)!=NULL && GET_KEY(GET_NEXT(new))>key);

		return NULL;
	} else if (key > ptr.k) {
		//TraceEvent(Self->UniqID, 60, _tl_kv_put, NULL, 0, 0);
		// ins after
		new.p=ptr;
		node_t tmp = malloc(sizeof(node_t));
		TxLoad(&tmp, ptr.n, sizeof(_node_t));
		new.n=tmp;

		//SET_PREV(new, ptr);
		//SET_NEXT(new, GET_NEXT(ptr));
		if (ptr.n != NULL) {
			//TraceEvent(Self->UniqID, 70, _tl_kv_put, NULL, 0, 0);
			//next = GET_NEXT(ptr);
			TxLoad(&tmp, ptr.n, sizeof(_node_t));
			ptr.n=

			WRITE_OBJ(next, sizeof(node_t));
			SET_PREV(next, new);
		}
		SET_NEXT(ptr, new);

		SET_KEY(new, key);
		SET_VALUE(new, val);

		//        vfy_assert(GET_PREV(new)!=NULL && GET_KEY(GET_PREV(new))<key);
		//        vfy_assert(GET_NEXT(new)==NULL || GET_KEY(GET_NEXT(new))>key);

		return NULL;
	} else {
		//TraceEvent(Self->UniqID, 80, _tl_kv_put, NULL, 0, 0);
		//replace key and value
		setkey_t ptrkey = GET_KEY(ptr);
		vfy_assert(key == ptrkey);
		SET_KEY(ptr, key);//unnecessary...
		SET_VALUE(ptr, val);

		//        vfy_assert(prev==NULL || GET_KEY(prev)<key);
		//        vfy_assert(next==NULL || GET_KEY(next)>key);

		return new;
	}
}*/


void routine() {
	//THREAD_INFO * Self;
	//Self = NEW_THREAD();


	//long thread_id = (long) args;
	//printf ("thread_id = %d\n",thread_id);

	//printf("%d: worker_thr\n", Self->UniqID);

	//    int worker_successfull_commits=0;
	//    int worker_failed_commits=0;

	int num_puts = 0;
	int num_deletes = 0;
	int num_gets = 0;

	//no synch.
	/*while (!can_start)
		sched_yield();*/

	unsigned long long test_time = 0;
	unsigned long long test_start_time = 0;
	unsigned long long test_end_time = 0;

	unsigned long long harness_op_time = 0;
	unsigned long long start_harness_op_time, stop_harness_op_time;

	unsigned long long stm_op_time = 0;
	unsigned long long start_stm_op_time, stop_stm_op_time;

	test_start_time = GET_TIME();
	test_start_time = GET_TIME();
	test_start_time = GET_TIME();

	printf("##SPE Routine## Starting SPE execution\n");
	//printf("####Timer START!!!!!###");
	spu_write_decrementer(0xFFFFFFFF); // initialization
	uint32_t t1 = spu_read_decrementer();
	//uint32_t t2 = spu_read_decrementer();
	//printf("Value : %d\n\n", t1 - t2);
	//printf("####END OF Timer TEST###");
	//while (!can_stop) {
		        int i;
		        for(i=0;i<10;i++){
		        //while(t1 - spu_read_decrementer() < myInfo.time){
		        //while((spu_clock_read() - start) < myInfo.time) {
		start_harness_op_time = GET_TIME();
		int rnd1 = MYRAND();
		int rnd2 = MYRAND();

		//printf("rnd %12d %12d\n",rnd1,rnd2);
		int pct = (int) (100.0 * (rnd1 / (RAND_MAX + 1.0)));
		setkey_t key = (long) ((double) infoStruct.key_range * (rnd2 / (RAND_MAX + 1.0)));
		setval_t val = key + 100;
		//        int key = i;
		//        int val = i;

		assert(pct >= 0 && pct < 100);
		assert(key >= 0 && key < infoStruct.key_range);

		start_stm_op_time = stop_harness_op_time = GET_TIME();
		if (pct < infoStruct.pct_put) {
			printf("#SPE routine# -> kv_put : Key->%d, val:%d \n", key, val);
			kv_put(key, val);
			num_puts++;
		} else if (pct < infoStruct.pct_put + infoStruct.pct_del) {
			printf("#SPE routine# -> kv_del : Key->%d \n", key);
			kv_delete(key);
			num_deletes++;
		} else if (pct < infoStruct.pct_put + infoStruct.pct_del + infoStruct.pct_get) {
			printf("#SPE routine# -> kv_get : Key->%d\n", key);
			val = kv_get(key);

			//assert(val == 0 || val == key + 100);
			/*if( (int) val== 0 || (int) val!= (int)key + 100){
				printf("ERROR:val!= 0 || val!=key + 100  val=%d key=%d \n", (int) val, (int) key);
				return;
			}*/
			num_gets++;
		} else {
			printf("ERROR!\n");
			return;
			//assert(0);
		}

		//stop_stm_op_time = GET_TIME();

		//stm_op_time += (stop_stm_op_time - start_stm_op_time);
		//harness_op_time += (stop_harness_op_time - start_harness_op_time);
		//sched_yield();
	}

	//test_end_time = GET_TIME();
	//test_time = test_end_time - test_start_time;

	//END_THREAD(Self);


	//Have to mfc_put this run_stats.
    myStats.num_puts = num_puts;
	myStats.num_deletes = num_deletes;
	myStats.num_gets = num_gets;
    myStats.test_time = test_time;
	//run_stats.stm_op_time = stm_op_time;
	//run_stats.harness_op_time = harness_op_time;

	return;
}


int verifySet() {

	node_t n;
	long long unsigned null = 0;
	printf("#VERIFY SET#Transfering root....\n");
	TxLoad((volatile *) &n, mySet.root, sizeof(node_t));
	//printf("Transfered root.... \n");
	printf("root->k = %llx , root->v = %llx\n", n.k , n.v);
	printf("(int) root->k = %d , root->v = %d\n", (int)n.k , (int) n.v);
	//printf(" root->n = %llx , root->p = %llx\n", n.n , n.p);
	//printf("(intptr_t) root->k = %d , root->v = %d\n", (intptr_t)n.k , (intptr_t) n.v);
	//printf("(double) root->k = %d , root->v = %d\n", (double)n.k , (double) n.v);
	//printf("(unsigned long int) root->k = %ul , root->v = %ul\n", (unsigned long int)n.k , (unsigned long int) n.v);
	//printf("(unsigned int) root->k = %u , root->v = %u\n", (unsigned int)n.k , (unsigned int) n.v);
	node_t tmp = n;
	int i=1;
	while ( ((int) tmp.n) != 0) {
		printf("Next-> %d \n", (int) tmp.n);
		printf("fetching next\n");
		printf("NODE(%d) (int) tmp->k = %d , tmp->v = %d\n", i, (int) tmp.k, (int) tmp.v);
		printf("NODE(%d) tmp->n = %llx , tmp->p = %llx\n", i, tmp.n, tmp.p);
		//void TxLoad(volatile *destination, long long int address, int size)
		if(tmp.n==0) break;
		TxLoad((volatile *)&tmp, tmp.n, sizeof(node_t));
		i++;

	}

	return 0;
}


int main(unsigned long long spe_id, unsigned long long argp, unsigned long long envp){



	printf("###CellHarnessSPE### main()\n");
	printf("###CellHarnessSPE### argp-> %llx\n", argp);
	unsigned long long pointerToData = (unsigned long long) TxStart(argp);
	printf("Received arguments PointerToData  -> %llx\n", pointerToData);
	int tag = 1, tag_mask = 1<<tag;
	mfc_get(&ctrStruct,pointerToData,sizeof(ctrStruct),tag,0,0);
	mfc_write_tag_mask(tag_mask);
	mfc_read_tag_status_all();
	printf("###CellHarnessSPE###Received arguments remoteThreadStats  -> %llx\n", ctrStruct.remoteThreadStats);
	printf("###CellHarnessSPE###Received arguments infoPointer  ->  %llx\n", ctrStruct.infoPointer);


	printf("#Testin remote malloc _ea#");
	 //__ea eaAddress = __malloc_ea64(sizeof(ctrStruct));
	 //printf("__ea = % llx\n", __ea);
	 printf("#END OF Testin remote malloc _ea#");

	mfc_get(&infoStruct,ctrStruct.infoPointer,sizeof(infoStruct),tag,0,0);
	mfc_write_tag_mask(tag_mask);
	mfc_read_tag_status_all();
	printf("###CellHarnessSPE###Received arguments pointerToList  ->  %llx\n", infoStruct.pointerToList);
	printf("##CellHarnessSPE## Size of mySet -> %d  \n", sizeof(mySet));
	printf("##CellHarnessSPE## Size of node_t -> %d  \n", sizeof(node_t));
	printf("##CellHarnessSPE## Size of info -> %d  \n", sizeof(info));
	mfc_get(&mySet,infoStruct.pointerToList,sizeof(mySet),tag,0,0);
	mfc_write_tag_mask(tag_mask);
	mfc_read_tag_status_all();

	printf("###CellHarnessSPE###Received arguments myset.root  ->  %llx\n", mySet.root);
	printf("Received pct_put=%d , pct_del=%d, pct_get=%d\n", infoStruct.pct_put, infoStruct.pct_del, infoStruct.pct_get);
	printf("Key Range = %d\n", infoStruct.key_range);
	//END OF INFO TRANSFER




	//unsigned spu_clock_slih (unsigned event_mask);
	/* use library FLIH and SLIH */
	//spu_slih_register (MFC_DECREMENTER_EVENT, spu_clock_slih);
	/* alloc timer for profiling */
	//id = spu_timer_alloc (14318, my_prof_handler);
	/* start clock before timer */
	//spu_clock_start ();
	/* profile the following block */
	//spu_timer_start (id);

	/* measure total time for work() */

	//start = spu_clock_read ();

	routine();
	//verifySet();
	//end = spu_clock_read();
	//total_spe_time = end - start;
	//time_working += (spu_clock_read () – start);

	/* done profiling */
	//spu_timer_stop (id);
	//spu_timer_free (id);
	/* more work */
	//spu_clock_stop ();






	//mfc_put();


	//printf("Received arguments ctrStruct  -> %ull \n", pointerToData.ctrStruct);


	printf("###END OF CellHarnessSPE####\n");


	//mfc_get(&root,mySet.root,sizeof(_node_t),tag,0,0);
	//mfc_write_tag_mask(tag_mask);
	//mfc_read_tag_status_all();
	//read to start computation. No need to transfer ThreadStats. we have local copy, then we will transfer.
	

	return 0;
}
