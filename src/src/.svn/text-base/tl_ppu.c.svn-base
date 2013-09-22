#include <libspe2.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>


void TxStartSPE(extern spe_program_handle_t spuCode) {

	/* we allocate one control block, to correspond to one SPE */
		 //control_block cb[1] __attribute__ ((aligned (128)));

		/* this is the pointer to the SPE code, to be used at thread creation time */
		 //extern spe_program_handle_t spuCode;

		/* before the thread is created, we create a group environment to contain it. */
		/* this "gid" is the handle for that group.                                   */
		 spe_gid_t gid;

		/* this is the handle which will be returned by "spe_create_thread."  */
		 speid_t speids[1];

		/* this variable is used to return data regarding an abnormal return from the SPE*/
		 int status[1];

		/* here is the variable to hold the address returned by the malloc() call. */
		 int *data;


	data = (int *) malloc(128);
	/* Create an SPE group */
	gid = spe_create_group(SCHED_OTHER, 0, 1);
	if (gid == NULL) {
		fprintf(stderr, "Failed spe_create_group(errno=%d)\n", errno);
		return -1;
	}
	if (spe_group_max(gid) < 1) {
		fprintf(stderr, "System doesn't have a working SPE.  I'm leaving.\n");
		return -1;
	}

	/* load the address into the control block */
	//cb[0].addr = (unsigned int) data;

	/* allocate the SPE task */
	speids[0] = spe_create_thread(gid, &spuCode, (unsigned long long *) &cb[0], NULL, -1, 0);
	if (speids[0] == NULL) {
		fprintf(stderr, "FAILED: spe_create_thread(num=%d, errno=%d)\n", 0, errno);
		exit(3);
	}
}
