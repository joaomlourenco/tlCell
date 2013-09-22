#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <stdint.h>
#include <assert.h>
//#include <spu_intrinsics.h>
//#include <libspe2.h>
#include <spu_mfcio.h>
#include <setjmp.h>
#include "tl_spu.h"

//#include <cache-api.h>
//#include "util.h"
//#include "data.h"

#define DEBUGON
#define MULTIBUFFER

/* Alignment macros */
#define SPE_ALIGNMENT 16
#define SPE_ALIGNMENT_FULL 128
#define SPE_ALIGN __attribute__((aligned(16)))
#define SPE_ALIGN_FULL __attribute__((aligned(128)))
#define ROUND_UP_ALIGN(value, alignment)\
 (((value) + ((alignment) - 1))&(~((alignment)-1)))
/*End of alignment macros*/


/*working on SPU MMap function.*/
#define __ea                    /* ignored */
#define mmap_func       mmap
#define mmap_fail       MAP_FAILED

#ifdef USE_CACHE
/*BEGGINING OF CACHE DEFINITION*/
#define CACHE_NAME              tlCache
#define CACHED_TYPE             intptr_t
#define CACHELINE_LOG2SIZE      10
#define CACHE_LOG2NWAY          2
#define CACHE_LOG2NSETS         5
#define CACHE_TYPE              1
#include <cache-api.h>
#define LOAD(ea)                cache_rd (tlCache, (unsigned)(ea))
#define STORE(ea, val)          cache_wr (tlCache, (unsigned)(ea), (val))
#define CACHE_FLUSH()           cache_flush (tlCache)
#define CACHE_PR_STATS()        cache_pr_stats (tlCache)
/*END OF CACHE DEFINITION*/
#endif



//volatile control_block cb __attribute__ ((aligned (128)));
unsigned int tag_id;
int txActive=0; //control variable. Should change it!
//xpto varStruct;

/*VARIABLES - Control Block*/
volatile controlStruct_t theStruct SPE_ALIGN_FULL;
//static cachelog *TxLog  SPE_ALIGN_FULL;
volatile Tx *txStruct SPE_ALIGN_FULL;
unsigned long long remoteWrSet;
unsigned long long remoteRdSet;

#ifdef MULTIBUFFER
volatile Log rdSetBuffer SPE_ALIGN_FULL;
volatile Log wrSetBuffer SPE_ALIGN_FULL;
volatile AVPair *wrPuffPut;	// Insert position - cursor
volatile AVPair *wrBuffList __attribute__((aligned(128)));
unsigned long long currentRemoteWrSet;
unsigned long long currentRemoteRdSet;

unsigned long int buffCount;
#endif

int countRdBufferTransfered =0; //REMEMBER MAX__LIST_SIZE
int countWrBufferTransfered =0;  //REMEMBER MAX__LIST_SIZE


/*Auxiliar Functions*/

static inline AVPair *MakeList(int const sz){
	int CachePad=64;
	//ASSERT_DEBUG(sz > 0);
	// Use CachePad to reduce the odds of false-sharing.
	AVPair *ap = (AVPair *) malloc((sizeof(*ap) * sz) + CachePad);
	//ASSERT(ap != NULL);
	memset(ap, 0, sizeof(*ap) * sz);
	int i;
	for (i = 0; i < sz; i++)
	{
		(ap + i)->Owner = theStruct.Owner;
	}
	return ap;
}


#ifdef MULTIBUFFER
//Returns 1 if one of the buffer is full
int WrBufferFull(){

	if(txStruct->wrSet.CurrentList == MAX_LOCAL_LIST_SIZE) return 1;
	else return 0;
	
}

int RdBufferFull() {

	if(txStruct->rdSet.CurrentList == MAX_LOCAL_LIST_SIZE) return 1;
	else return 0;
}

int DumpWrSet(){
//unsigned long long remoteWrSet = theStruct.wrSet;AVPair *tmpList = txStruct->wrSet.List;

	while(txStruct->wrSet.List < txStruct->wrSet.Put){
		mfc_put(txStruct->wrSet.List, currentRemoteWrSet, sizeof(AVPair) , tag_id, 0, 0);
		txStruct->wrSet.List++;
		currentRemoteWrSet+=176; //176 = sizeof(AVPAIR) in PPE

	}
	txStruct->wrSet.List = tmpList;
	countWrBufferTransfered++;
	txStruct->wrSet.List = MakeList(MAX_LOCAL_LIST_SIZE);
	txStruct->wrSet.Put = txStruct->wrSet.List;
	txStruct->wrSet.CurrentList=0;
	return 1;
}

int DumpRdSet() {

	//unsigned long long remoteRdSet = theStruct.rdSet;
	AVPair *tmpList = txStruct->rdSet.List;
	while(txStruct->rdSet.List < txStruct->rdSet.Put){
		mfc_put(txStruct->rdSet.List, currentRemoteRdSet, sizeof(AVPair), tag_id, 0, 0);
		txStruct->rdSet.List++;
		currentRemoteRdSet+=176; //176 = sizeof(AVPAIR) in PPE
	}
	txStruct->rdSet.List = tmpList;
	txStruct->rdSet.List = MakeList(MAX_LOCAL_LIST_SIZE);
	txStruct->rdSet.Put = txStruct->rdSet.List;
	txStruct->rdSet.CurrentList=0;
	countRdBufferTransfered++;
	return 1;
}
#endif



void printWrSet(){
	Log *const wr = &txStruct->wrSet;

	AVPair *p;
	int i=0;
					for (p = wr->Put - 1; p >= wr->List; p--)
					{
						printf("i=%d --- p->Addr= %llx , p->ObjValu[0]=%d  \n", i, p->Addr , p->ObjValu[0]);
						i++;
					}
}

void printRdSet(){
	Log *const wr = &txStruct->rdSet;

	AVPair *p;
	int i=0;

					for (p = wr->Put - 1; p >= wr->List; p--)
					{
						printf("i=%d --- p->Addr= %llx , p->ObjValu[0]=%d  \n", i, p->Addr , p->ObjValu[0]);
						i++;
					}
}


/*LIBRARY ALLOWED FUNCTIONS
 * Returns the pointer to User Structure.
 * Initializes TxStruct.
 * Must receive argp from main(...,argp,...);
 * */
unsigned long long TxStart(long long int argp){

	int tag_id;
	/* Reserve a tag for application usage */
	  if ((tag_id = mfc_tag_reserve()) == MFC_TAG_INVALID) {
	    printf("ERROR: unable to reserve a tag\n");
	    return 1;
	  }
	  int tag = 1, tag_mask = 1<<tag;
	  mfc_get(&theStruct, argp, sizeof(theStruct), tag, 0, 0);
	  mfc_write_tag_mask(tag_mask);
	  mfc_read_tag_status_all();
	  remoteWrSet = theStruct.wrSet;
	  remoteRdSet = theStruct.rdSet;
//#ifdef MULTIBUFFER
	  currentRemoteWrSet=remoteWrSet;
	  currentRemoteRdSet=remoteRdSet;
//#endif

	 txActive=1;
	 txStruct = (Tx *) malloc(sizeof(Tx));
	 txStruct->wrSet.List = MakeList(MAX_LOCAL_LIST_SIZE);
	 txStruct->wrSet.Put = txStruct->wrSet.List;
	 txStruct->rdSet.List = MakeList(MAX_LOCAL_LIST_SIZE);
	 txStruct->rdSet.Put = txStruct->rdSet.List;
	 txStruct->rdSet.CurrentList=0;
	 txStruct->wrSet.CurrentList=0;
	 txStruct->lowerTx = NULL;
	 txStruct->upperTx = NULL;
	 txStruct->Mode = TIDLE;


	 return theStruct.userStruct;
}

/*Loads a value from main memory (address) to a LS destination.
 * Might be already
 *
 * */
void TxLoad(volatile *destination,  long long int address, int size){ //size?
/*
 * Search local cache for the address. If not present in cache, DMA transfer from main memory and update Read-Set
 * LOAD(ea);
 * 1)Check write-set for address.
 * 		1.1)If present -> return value in writeset
 * 		1.2)If not present -> mfcget
 * 			1.2.1)Check if mfcget.rv<=txStruct.rv
 * 				1.2.1.1)if true - add value to readset and return value.
 * 				1.2.1.2)if false - abort tx? inconsistent state. Other transaction has written in a variable we wish to read.
 * */
	if(txActive!=1){
		printf("No transaction started - Must start a transaction to issue a TxLoad\n");
		return 0;
	}

//#ifdef MULTIBUFFER //swap buffers and dump
	if(RdBufferFull()) {

		printf("Swaping buffers RdSet\n");
		DumpRdSet(); //mfcPut

	}

//#endif

	long long int tmp=address;
	//check in wrSet for address
	Log *const wr = &txStruct->wrSet;
	//vwLock volatile *LockFor = PSLOCK(address); (atencao a linha de baixo, é necessário ser endereco de 64bits)
	volatile vwLock *LockFor = ((theStruct.lockTab) + (PSLOCKOFFSET(tmp)));
	intptr_t msk = FILTERBITS(tmp);
	//bloomfilter finds it?
	if ((wr->BloomFilter & msk) == msk)
			{
				AVPair *p;
				//lets double check since bloomfilter gives false positives.
				for (p = wr->Put - 1; p >= wr->List; p--)
				{
					//ASSERT_DEBUG(p->Addr != NULL); //hum.. aqui como vai ser?
					if(p->Addr == NULL){
					  printf("ERROR :(TxStore) p->Addr = null\n");	
					  return NULL ;
					}
					//found it?

					if (p->Addr == address)
					{
						//ASSERT_DEBUG(LockFor == PSLOCK(p->Addr));
						/*CBEA version*/
						//ASSERT_DEBUG(LockFor = (theStruct.lockTab + PSLOCKOFFSET(address));
						if(LockFor != (theStruct.lockTab + PSLOCKOFFSET(address))) return NULL;

						//TraceEvent(Self->UniqID, 20, _tl_txload, addr, (intptr_t)0,(intptr_t)p->ObjValu[0]);

						*destination = p->ObjValu[0];
						//printf("#Internal Library SPE -> Found in wrSet#\n");
						return;
						//return p->ObjValu[0];
					}
				}


				//SEARCH IT IN MAIN MEMORY
				if (countWrBufferTransfered > 0) {

					/*printf("#tl_spu - TxLoad#remoteSearchMainMemory\n");
					printf("Remote Put = %llx\n", remoteWrSet + countWrBufferTransfered * MAX_LOCAL_LIST_SIZE * 176);
					printf("Remote Put (-176) = %llx\n", remoteWrSet + (countWrBufferTransfered * MAX_LOCAL_LIST_SIZE * 176) -176);
					printf("currentRemoteWrSet = %llx \n",currentRemoteWrSet );
					printf("currentRemoteWrSet (-176) = %llx \n",currentRemoteWrSet -176 );*/

					int tag = 1, tag_mask = 1 << tag;
					//AVPair *tmpList = txStruct->List;
					unsigned long long tmpList = remoteWrSet;
					unsigned long long tmpPut = currentRemoteWrSet; //remoteWrSet * countWrBufferTransfered * MAX_LOCAL_LIST_SIZE * 176;
					tmpPut-=176; // -1 for last entry in WriteSet.
					//offset of transfered buffer.
					//AVPair *tmpPut = txStruct->List * countWrBufferTransfered * MAX_LOCAL_LIST_SIZE * 176;
					AVPair tmp;
					//while (tmpList < tmpPut) {
					while(tmpPut>tmpList){

						/*printf("getting remote wrSet entry -> %llx\n", tmpPut);
						printf("tmpList= -> %llx\n", tmpList);*/

						mfc_get(&tmp, tmpPut, sizeof(AVPair), tag_id, 0, 0);
						mfc_write_tag_mask(tag_mask);
						mfc_read_tag_status_all();
						if (tmp.Addr == address) {

							printf("remoteSearch found the entry.. returning.. value=%d\n", tmp.ObjValu[0]);

							*destination = tmp.ObjValu[0];
							return;
						}
						tmpPut-=176;
					}
				}

			}




	//address not found in wrSet. Lets load from main memory.
	//Is variable Locked?
	vwLock rdv2 = LDLOCK(LockFor); //isto tem que ser mfc_get. OU NAO! discuss...
	//#ifdef DEBUGON
	//printf("IS_VERSION(rdv2) = %d  and rdv2<=theStruct.rv <=> %d <= %d  \n" , IS_VERSION(rdv2) , rdv2 , theStruct.rv );
	//#endif
	if ( IS_VERSION(rdv2) /*&& (((int) rdv2) <= ((int)theStruct.rv))*/) {
		int tag = 1, tag_mask = 1<<tag;
		mfc_get(destination, (unsigned long long) address, size, tag_id, 0, 0);

		mfc_write_tag_mask(tag_mask);
		mfc_read_tag_status_all();

		Log *k = &txStruct->rdSet;
		//ASSERT_DEBUG(!IS_LOG_FULL(k));

		k->Put->Addr = address;
		k->Put++;
		k->CurrentList++;

	}
	else{//abort situation.


		printf("Aborting Transaction.. Inconsistency detected\n");
		TxAbort();
		return;

	}

	return;

}

#define WORD_COPY(d,s) do {\
	d=s;\
}while(0)



void TxStore(unsigned long long volatile address, intptr_t value) { //size?


	if (txActive != 1) {
		printf("No transaction started - Must start a transaction to issue a TxLoad\n");
		return;
	}

	if(WrBufferFull()){
		printf("Swaping buffers WrSet\n");
		DumpWrSet();
	}

	Log *wr = &txStruct->wrSet;
	//MEMBARLDLD(); /We dont need memory barriers
	wr->BloomFilter |= FILTERBITS(address);

	Log *k = &txStruct->wrSet;
	//ASSERT(!IS_LOG_FULL(k));
	//ASSERT_DEBUG(address != NULL);
	if(address == NULL) return; // return NULL;
	AVPair *p = k->Put;
	k->Put++;
	k->CurrentList++;
	p->Addr = address;
	//LINE_COPY(p->ObjValu, &Valu);
	WORD_COPY(p->ObjValu[0], value); //Esta é a linha original
	//WORD_COPY(p->ObjValu[0], theStruct.pointerToObjValu);
	p->Held = 0;
	p->rdv = LOCKBIT; // use either 0 or LOCKBIT

	//printf("######Leaving TxStore(TL_SPU)#############\n");
	printf(" "); //THE MYSTERIOUS PRINTF!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	return;
}
void TxCommit(){
/*
 * Flush wrSet and rdSet to main memory.
 * */

	int totalValue, totalValue1;
	if ((tag_id = mfc_tag_reserve()) == MFC_TAG_INVALID) {
		printf("ERROR: unable to reserve a tag\n");
		return NULL;
	}
	int tag_mask = 1 << tag_id;

	while(txStruct->wrSet.List < txStruct->wrSet.Put){
		//printf("Transfering AVPair to memory position = %llx and from SPE memory position = %p\n", theStruct.wrSet , txStruct->wrSet.List);
#ifdef MULTIBUFFER
		mfc_put(txStruct->wrSet.List, currentRemoteWrSet, sizeof(AVPair) , tag_id, 0, 0);
		currentRemoteWrSet+=176;
#endif
#ifndef MULTIBUFFER
		mfc_put(txStruct->wrSet.List, remoteWrSet, sizeof(AVPair) , tag_id, 0, 0);
		remoteWrSet+=176;
#endif
		txStruct->wrSet.List++;
		 //176 = sizeof(AVPAIR) in PPE

	}
	if(countWrBufferTransfered==0){
		mfc_put(&(txStruct->wrSet.CurrentList), theStruct.CurrentListwrSet, sizeof(int), tag_id, 0, 0);

	}
	else{
		totalValue1 = txStruct->wrSet.CurrentList + countWrBufferTransfered*MAX_LOCAL_LIST_SIZE;
		mfc_put( &totalValue1, theStruct.CurrentListwrSet, sizeof(int), tag_id, 0, 0 );
	}
	mfc_write_tag_mask(tag_mask);
	mfc_read_tag_status_all();

	while(txStruct->rdSet.List < txStruct->rdSet.Put){
#ifdef MULTIBUFFER
		mfc_put(txStruct->rdSet.List, currentRemoteRdSet, sizeof(AVPair) , tag_id, 0, 0);
		currentRemoteRdSet+=176;
#endif
#ifndef MULTIBUFFER
		mfc_put(txStruct->rdSet.List, remoteRdSet, sizeof(AVPair), tag_id, 0, 0);
		remoteRdSet+=176; //176 = sizeof(AVPAIR) in PPE
#endif
		txStruct->rdSet.List++;
	}
	//printf("Trying to set size of rdset which is =%d\n" , txStruct->rdSet.CurrentList );
	if(countRdBufferTransfered==0){

		mfc_put(&(txStruct->rdSet.CurrentList), theStruct.CurrentListrdSet, sizeof(int), tag_id, 0, 0);
	}
	else {
		totalValue = txStruct->rdSet.CurrentList + countRdBufferTransfered*MAX_LOCAL_LIST_SIZE;
		mfc_put( &totalValue, theStruct.CurrentListrdSet, sizeof(int), tag_id, 0, 0 );
	}
	mfc_write_tag_mask(tag_mask);
	mfc_read_tag_status_all();

	txActive=0;


}

void TxAbort(){
/*
 * Clear cache.
 * free() the struct
 * */

	free(txStruct);
	txActive=0;
	return;

}


