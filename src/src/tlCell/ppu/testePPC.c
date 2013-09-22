#include <sched.h>
#include <libspe2.h>
#include <pthread.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/wait.h>
//#include <malloc_align.h>
#include <string.h>
#include <malloc_align.h>
#include <assert.h>
//#include "tl_ppu.h"
#include "tl.h"

#define testVector
//#define testInt

extern spe_program_handle_t spuCode;
#ifdef testInt
int dataToSend __attribute__((aligned(128)));
#endif

#ifdef testDouble
double dataToSendDouble __attribute__((aligned(128)));
#endif
//volatile int dataToSend __attribute__((aligned(128)));
int i=0;
int nrThread = 10;

//intptr_t *dboard __attribute__((aligned(128)));
#ifdef testVector
int boardSize=10;
int dboard[10] __attribute__((aligned(16)));



void startBoard(int size) {
	int *dboardp2 = (int *) malloc(sizeof(int) * size);
	int *temp = dboardp2;
	int z;
	boardSize = size;
	for (z = 0; z < size; z++) {
		dboard[z] = 10;
		dboardp2 = &dboard[z];
		dboardp2++;
		//printf("pointer is %p \n", &dboard[z]);
	}
	dboardp2 = temp;
	printf("dboarpd2 = %p", dboardp2);
}

void board_init(int size, int value) {
    //dboard=(intptr_t*) malloc(sizeof(intptr_t)*size);
    int z=0;
    for(z=0;z<size;z++) {
        dboard[z]=value;
        //printf("Pointer for dboard[%d] = %p \n", z, &dboard[z]);
    }
}


void board_set(Thread * Self, int size, int val) {
    int i=0;
    for(i=0;i<size;i++) {
        TXSTV(dboard[i],val);
    }
}

int board_check(Thread * Self, int size, int val) {
    int i=0;
    for(i=0;i<size;i++) {
        int v=TXLDV(dboard[i]);
        //printf("v=%d val=%d\n",v,val);
        assert(v==val);
        if(v!=val)
            return 0;
    }
    return 1;
}

void *ppu_pthread_function2(void *arg) {
  //TxStartSPE(spuCode,&dataToSend);
	TxStartSPE(spuCode,&dboard);
	//TxStartSPEBlocking(spuCode,&dboard);
  pthread_exit(NULL);
}
#endif

#ifdef testInt
void *ppu_pthread_function3(void *arg) {
	TxStartSPE(spuCode,&dataToSend);
	//TxStartSPE(spuCode,dataToSend);
  pthread_exit(NULL);
}
#endif

#ifdef testDouble
void *ppu_pthread_function4(void *arg) {
  //TxStartSPE(spuCode,&dataToSend);
	TxStartSPE(spuCode,&dataToSendDouble);
  pthread_exit(NULL);
}
#endif

#ifdef testInt

/*int testInt(int nrThreads) {

	pthread_t pthread[nrThreads];
	dataToSend = 10;
	int i;
	for (i = 0; i < nrThreads; i++) {
		pthread_create(&pthread[i], NULL, &ppu_pthread_function3, NULL);
	}
	//i = 0;
	//printf("Issuing a TxStartSpe. Memory alloced at position=%p\n", &dataToSend);
	for (i = 0; i < nrThreads; i++) {
		//Lets paralelize the computation.
		pthread_join(pthread[nrThreads], NULL);
	}

	return 1;
}*/
#endif

#ifdef testDouble
int testDouble(int nrThreads) {
	pthread_t pthread[nrThreads];
	dataToSendDouble = 10.0;

	int i;
	for (i = 0; i < nrThreads; i++) {
		pthread_create(&pthread[i], NULL, &ppu_pthread_function4, NULL);
	}

	//printf("Issuing a TxStartSpe. Memory alloced at position=%p\n", &dataToSend);
	for (i = 0; i < nrThreads; i++) {
		//Lets paralelize the computation.
		pthread_join(pthread[nrThreads], NULL);
	}
	return 1;
}
#endif

/*Will start blocking SPE transaction (non-threaded) and provoke collision
 * PPE transaction must abort since SPE transaction modified var.
 * set #define testInt in spuCode.c
 */

int main(int argc, char *argv[]){
	/* this is the handle which will be returned by "spe_context_create."  */
	//spe_context_ptr_t speid;

	/* this variable is the SPU entry point address which is initially set to the d*/
	//unsigned int entry = SPE_DEFAULT_ENTRY;

	/* this variable is used to return data regarding an abnormal return from the S*/
	//spe_stop_info_t stop_info;



	if (argc != 2) {
		printf("syntax: testePPC - nrThreads \n");
		exit(1);
	}
	if (atoi(argv[1]) < 0) {
		printf("Cant have negative Threads\n");
		exit(1);
	}
	nrThread=atoi(argv[1]);
	pthread_t pthread[nrThread];



#ifdef testInt
	dataToSend = 10;
	//testBlockingVar();
#endif
#ifdef testDouble
	dataToSendDouble = 10.0;
#endif
#ifdef testVector
	//initializing whole set with value=0
	board_init(boardSize,0);
#endif

	for(i=0;i<nrThread;i++){
#ifdef testInt
		pthread_create(&pthread[i], NULL, &ppu_pthread_function3, NULL);
		//testBlockingVar();
#endif
#ifdef testDouble
		pthread_create(&pthread[i], NULL, &ppu_pthread_function4, NULL);
#endif
#ifdef testVector
		pthread_create(&pthread[i], NULL, &ppu_pthread_function2, NULL);
#endif
	}

	//printf("Issuing a TxStartSpe. Memory alloced at position=%p\n", &dataToSend);
	for (i=0; i < nrThread; i++) {
		//Lets paralelize the computation.
		pthread_join(pthread[i], NULL);
	}
#ifdef testInt
	printf("TxStartSPE returned successufuly - Received data = %d\n", dataToSend);
#endif
#ifdef testDouble
	printf("TxStartSPE returned successufuly - Received data = %f\n", dataToSendDouble);
#endif
#ifdef testVector
	//printf("TxStartSPE returned successufuly - Received dboard = %d\n", dboard[0]);
	printf("TxStartSPE returned successufuly - Received dboard =[ %d,", dboard[0]);
	for (i = 1; i < boardSize; i++) {
		printf("%d,", dboard[i]);
	}
	printf("]\n");


#endif
	return 0;
}









