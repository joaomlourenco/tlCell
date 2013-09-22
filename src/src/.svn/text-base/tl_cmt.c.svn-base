#include <config.h>


#define CPU_CELL

#ifdef CPU_CELL
//#include..
#endif
#ifdef USE_TX_HANDLER
#include "tl_handler.c"
#endif

//static inline void RecordStoreCMT (Log * const k, intptr_t volatile const * const addr, intptr_t const Valu, vwLock volatile * Lock) {
static inline int RecordStoreCMT(Thread * const Self,
				 intptr_t volatile *const addr,
				 intptr_t const Valu)
{
	// As an optimization we could squash multiple stores to the same location.
	// Maintain FIFO order to avoid WAW hazards.
	Log *k = &GET_TX(Self)->wrSet;
	ASSERT(!IS_LOG_FULL(k));

	ASSERT_DEBUG(addr != NULL);
	AVPair *p = k->Put;
	k->Put++;
	p->Addr = addr;
	//LINE_COPY(p->ObjValu, &Valu);
	WORD_COPY(p->ObjValu[0], Valu);

	p->Held = 0;
	p->rdv = LOCKBIT;	// use either 0 or LOCKBIT

	return 1;
}	//RecordStoreCMT

//Restore locks to old version number
static inline void UndoLocksCMT(Thread * const Self)
{
	AVPair *p;

	ASSERT_DEBUG(GET_TX(Self) != NULL && GET_TX(Self)->Mode == TTXN);
	ASSERT_DEBUG(GET_TX(Self)->upperTx == NULL);	//ensure this the main tx

	for (p = GET_TX(Self)->wrSet.List; p != GET_TX(Self)->wrSet.Put;
	     p++)
	{
		ASSERT_DEBUG(p->Addr != NULL);
		if (p->Held == 0)
		{
			ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(p->Addr))) !=
				     Self);
			ASSERT_DEBUG(LDLOCK(PSLOCK(p->Addr)) !=
				     ADDR_2_VWLOCK(p));

			continue;
		}
		ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(p->Addr))) == Self);
		ASSERT_DEBUG(LDLOCK(PSLOCK(p->Addr)) == ADDR_2_VWLOCK(p));
		ASSERT_DEBUG(IS_VERSION(p->rdv));
		p->Held = 0;
		STLOCK(PSLOCK(p->Addr), p->rdv);
		p->Addr = NULL;	// diagnostic hygiene
	}

	GET_TX(Self)->wrSet.Put = GET_TX(Self)->wrSet.List;
}	//UndoLocksCMT

// Transfer the data in the log its ultimate location and
// then mark the log as empty.
static inline void RedoValuesCMT(Thread * const Self)
{
	ASSERT_DEBUG(GET_TX(Self) != NULL && GET_TX(Self)->Mode == TTXN);
	ASSERT_DEBUG(GET_TX(Self)->upperTx == NULL);	//ensure this the main tx

	AVPair *p;
	Log *k = &GET_TX(Self)->wrSet;
	for (p = k->List; p != k->Put; p++)
	{
		ASSERT_DEBUG(p->Addr != NULL);
		//LINE_COPY(p->Addr,p->ObjValu);
		WORD_COPY(*(p->Addr), p->ObjValu[0]);

	}
	// Note: WriteBackF explicitly avoids resetting the "Put" list.
	// k->Put = k->List ;
}	//RedoValuesCMT

// release the locks after a successfull commit and update lock version
// used on CMT-commit
static inline void ReleaseLocksCMT(Thread * const Self, vwLock wv)
{
	ASSERT_DEBUG(GET_TX(Self) != NULL && GET_TX(Self)->Mode == TTXN);
	ASSERT_DEBUG(IS_VERSION(wv));
	AVPair *p;
	for (p = GET_TX(Self)->wrSet.List; p != GET_TX(Self)->wrSet.Put;
	     p++)
	{
		ASSERT_DEBUG(p->Addr != NULL);

		vwLock volatile *LockFor = PSLOCK(p->Addr);

		if (p->Held == 0)
		{
			ASSERT_DEBUG(LDLOCK(LockFor) != ADDR_2_VWLOCK(p));
			continue;
		}
		ASSERT_DEBUG(OwnerOf(LDLOCK(LockFor)) == Self);
		ASSERT_DEBUG(p->Held == 1);
		p->Held = 0;

		ASSERT_DEBUG(LDLOCK(LockFor) == ADDR_2_VWLOCK(p));
		ASSERT_DEBUG(IS_VERSION(p->rdv));
		ASSERT_DEBUG(wv > p->rdv);
		//        TraceEvent(Self->UniqID, 55, _tl_txcommit, p->Addr, (intptr_t)LockFor,(intptr_t)wv);
		STLOCK(LockFor, wv);
		//        TraceEvent(Self->UniqID, 56, _tl_txcommit, p->Addr, (intptr_t)LockFor,(intptr_t)*LockFor);

		p->Addr = NULL;	// diagnostic hygiene
	}
	GET_TX(Self)->wrSet.Put = GET_TX(Self)->wrSet.List;
}	//ReleaseLocksCMT





// --------------------------------
// CMT
// --------------------------------
static inline int TryFastUpdate(Thread * const Self)
{
	//printf("TryFastUpdate - 1\n");
	ASSERT_DEBUG(GET_TX(Self) != NULL && GET_TX(Self)->Mode == TTXN);
	Log *const wr = &GET_TX(Self)->wrSet;
	Log *const rd = &GET_TX(Self)->rdSet;
	//printf("TryFastUpdate - 2\n");
	// Optionally optimization -- pre-validate the read-set.
	// Consider: Call ReadSetCoherent() before grabbing write-locks.
	// Validate that the set of values we've fetched from pure READ objects
	// remain coherent.  This avoids the situation where a doomed transaction
	// grabs write locks and impedes or causes other potentially successful
	// transactions to spin or abort.
	// A smarter tactic might be to only call ReadSetCoherent() when
	// Self->Retries > NN.

	// Consider: if the write-set is long or Self->Retries is high we
	// could run a pre-pass and sort the write-locks by LockFor address.
	// We could either use a separate LockRecord list (sorted) or
	// link the write-set entries via SortedNext.
	AVPair *p;

	if (!ReadSetCoherent(Self, 0))
	{
		//printf("TryFastUpdate - ReadSetNotCoherent \n");
		return 0;
	}
	//printf("TryFastUpdate - 3\n");
	//    TraceTxCommit(Self->UniqID, 9);

	// Lock-acquisition phase ...
	int ctr = 1000;	// Spin budget - TUNABLE
	for (p = wr->List; p != wr->Put; p++)
	{

		ASSERT_DEBUG(p->Addr != NULL);
		ASSERT_DEBUG(p->Held == 0);
		ASSERT_DEBUG(p->Owner == Self);
		vwLock volatile *LockFor = PSLOCK(p->Addr);
		// Consider prefetching only when Self->Retries == 0
		PrefetchW((void *)LockFor);
		vwLock cv = LDLOCK(LockFor);

		if (OwnerOf(cv) == Self)
		{
			// THIS CANT BE DONE - KEEP IN MIND THAT SEVERAL ADDRESSES
			// CAN MAP TO THE SAME LOCK POSITION and we havent verified if
			// someone hasn't changed it between now and a prior read
			// Example: first ST is a write only ST, second ST is a R/W ST.
			// Both addreses map to the same position in locktab.
			// The 2nd ST does not verify if some other TX wrote to the 2nd addr
			// between the 1st TX LD and 1st TX Commit

			// Already locked by an earlier iteration.
			//continue ;
		}
		// Check is the variable is write only or read/write
		AVPair *r = FindFirstAddr(rd, p->Addr);
		//TraceEvent(Self->UniqID, 10, _tl_txcommit, p->Addr, (intptr_t)0, (intptr_t)cv);
#ifdef ENABLE_TRACE
		{
			int res[] = { 10, (int)p->Addr, (int)cv, 0 };
			TraceEvent(Self->UniqID, _tl_txcommit, res);
		}
#endif
		if (r != NULL)
		{
			// READ-WRITE stripe

			if (IS_VERSION(cv) && cv <= Self->rv
			    && CAS_VWLOCK(LockFor, cv,
					  (vwLock) ADDR_2_VWLOCK(p)) == cv)
			{
				p->rdv = cv;
				p->Held = 1;
				//                TraceEvent(Self->UniqID, 11, _tl_txcommit, p->Addr, (intptr_t)0, (intptr_t)cv);
				continue;
			}

			if (IS_LOCKED(cv) && OwnerOf(cv) == Self
			    && GetAVPairOf(cv)->rdv <= Self->rv)
			{
				// this addr was already locked or some other address maps to this lock
				//p->rdv  = GetAVPairOf(cv)->rdv;
				p->rdv = 0;	//optional
				p->Held = 0;
				//                TraceEvent(Self->UniqID, 12, _tl_txcommit, p->Addr, (intptr_t)0, (intptr_t)cv);
				continue;
			}
			// The stripe is either locked or the previously observed
			// read-version changed.  We must abort.  Spinning makes little sense.
			// In theory we could spin if the read-version is the same but
			// the lock is held in the faint hope that the owner might
			// abort and revert the lock.
			//printf("Issuing UndoLocksCMT (locked or rv changed \n");
			UndoLocksCMT(Self);
			return 0;
		} else
		{
			// WRITE-ONLY stripe
			// Note that we already have a fresh copy of *LockFor in cv.
			// If we find a write-set element locked then we can either
			// spin or try to find something useful to do, such as :
			// A. Validate the read-set by calling ReadSetCoherent()
			//    We can abort earlier if the transaction is doomed.
			// B. optimistically proceed to the next element in the write-set.
			//    Skip the current locked element and advance to the
			//    next write-set element, later retrying the skipped elements.
			//            TraceEvent(Self->UniqID, 15, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);
			for (;;)
			{
				cv = LDLOCK(LockFor);
				//                TraceEvent(Self->UniqID, 16, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);
				if (IS_VERSION(cv)
				    && CAS_VWLOCK(LockFor, cv,
						  (vwLock)
						  ADDR_2_VWLOCK(p)) == cv)
				{
					p->rdv = cv;	// Save LockWord so we can restore or increment
					p->Held = 1;
					//                    TraceEvent(Self->UniqID, 17, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);
					break;
				}
				if (IS_LOCKED(cv) && OwnerOf(cv) == Self)
				{
					// Save LockWord so we can restore or increment
					//p->rdv  = GetAVPairOf(cv)->rdv;
					p->rdv = 0;	//optional
					p->Held = 0;
					//                    TraceEvent(Self->UniqID, 18, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);
					break;
				}
				//                TraceEvent(Self->UniqID, 19, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);
				if (--ctr < 0)
				{
					//printf("Issuing UndoLocksCMT because ctr<0\n");
					UndoLocksCMT(Self);
					return 0;
				}

				PAUSE();
			}
		}
		//        TraceEvent(Self->UniqID, 20, _tl_txcommit, p->Addr, (intptr_t)LockFor, (intptr_t)cv);

	}

	//    TraceTxCommit(Self->UniqID, 22);
	//        for (p = rd->List ; p != rd->Put ; p++) {
	//            TraceEvent(Self->UniqID, 25, _tl_txcommit, p->Addr, 0,(intptr_t)0);
	//        }
	//        for (p = wr->List ; p != wr->Put ; p++) {
	//            TraceEvent(Self->UniqID, 26, _tl_txcommit, p->Addr, p->Held,(intptr_t)p->rdv);
	//        }


	// We now hold all the locks for RW and W objects.
	// Next we validate that the values we've fetched from pure READ objects
	// remain coherent.
	//
	// If GVGenerateWV() is implemented as a simplistic atomic fetch-and-add then
	// we can optimize by skipping read-set validation in the common-case.
	// Namely,
	//   if (Self->rv != (wv-2) && !ReadSetCoherent(Self)) { ... abort ... }
	// That is, we could elide read-set validation for pure READ objects if
	// there were no intervening write txns between the fetch of _GCLOCK into
	// Self->rv in TxStart() and the increment of _GCLOCK in GVGenerateWV().

	if (!ReadSetCoherent(Self, 0))
	{
		// The read-set is inconsistent.
		// The transaction is spoiled as the read-set is stale.
		// The candidate results produced by the txn and held in
		// the write-set are a function of the read-set, and thus invalid.
		//printf("readSet inconsistent (!ReadSetCoherent) so.. issuing UndoLocksCMT\n");
		UndoLocksCMT(Self);

		return 0;
	}
#ifdef USE_TX_HANDLER
	if (!_ctl_call_prepare_commit_handlers(Self))
	{
		UndoLocksCMT(Self);
		return 0;
	}
#endif


	//TraceEvent(Self->UniqID, 30, _tl_txcommit, NULL, (intptr_t)0, (intptr_t)0);
#ifdef ENABLE_TRACE
	{
		int res[] = { 30, (int)NULL, 0, 0 };
		TraceEvent(Self->UniqID, _tl_txcommit, res);
	}
#endif
	// We're now committed - this txn is successful.
	// Write-back the deferred stores to their ultimate object locations.
	//printf("We're now committed - this txn is successful. RDOCaluesCMT\n");
	RedoValuesCMT(Self);

	TraceTxCommit(Self->UniqID, 40);

	////    // Flush values before incrementing the clock - done by the CAS
	////    MEMBARSTST() ;

	// MUST BE INCREMENTED AFTER VALUES HAVE BEEN COPIED ACROSS!!! Otherwise the improved TxLoad algorithm doesn't work.
	vwLock wv = GVGenerateWV(Self);

	//TraceEvent(Self->UniqID, 50, _tl_txcommit, NULL, (intptr_t)0, (intptr_t)wv);
#ifdef ENABLE_TRACE
	{
		int res[] = { 10, (int)NULL, (int)wv, 0 };
		TraceEvent(Self->UniqID, _tl_txcommit, res);
	}
#endif
	// Release all the held write-locks, incrementing the version
	//printf("Releasing locks. RealeaseLocksCMT\n");
	ReleaseLocksCMT(Self, wv);
	//printf("Locks Released. RealeaseLocksCMT\n");
	TraceTxCommit(Self->UniqID, 60);

	// Ensure that all the prior STs have drained before starting the next
	// txn.  We want to avoid the scenario where STs from "this" txn
	// languish in the write-buffer and inadvertently satisfy LDs in
	// a subsequent txn via look-aside into the write-buffer.
	MEMBARSTLD();

	//printf("returning from FastUpdate\n");
	return 1;	// return success indication
}	//TryFastUpdate



int TxCommitCMT(Thread * const Self)
{
	TraceTxCommit(Self->UniqID, 0);
	//printf("TxCommitCMT - 1\n");
	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "Trying to commit but not within an active transaction\n");

#ifdef ENABLE_NESTING

	if (GET_TX(Self)->upperTx != NULL)
	{
		// Nested TX
		// Optionally we could call ReadSetCoherent

		AppendList(&GET_TX(Self)->upperTx->rdSet,
			   &GET_TX(Self)->rdSet, MAX_LIST_SIZE);
		AppendList(&GET_TX(Self)->upperTx->wrSet,
			   &GET_TX(Self)->wrSet, MAX_LIST_SIZE);
#ifndef DISABLE_LOCAL_VARS

		AppendList(&GET_TX(Self)->upperTx->LocalUndo,
			   &GET_TX(Self)->LocalUndo, Self->LocalUndoSize);
#endif

		CleanupTx(Self);
		//        TraceTxCommit(Self->UniqID, 900);
		return 1;
	}
#endif

#if (! defined DISABLE_TXSTERILIZE_EXT)
	// Fast-path: Optional optimization for pure-readers
    //printf("TxCommitCMT - 2\n");
	if (GET_TX(Self)->wrSet.Put == GET_TX(Self)->wrSet.List)
	{
		// the read-set is already known to be coherent.

#ifdef USE_TX_HANDLER
		if (!_ctl_call_prepare_commit_handlers(Self))
		{
			STATS_INC(_tl_num_cmt_aborts);
			AbortBackOffTx(Self);
			return 0;
		}
#endif

		STATS_INC(_tl_num_ok_commits);
#ifdef USE_TX_HANDLER
		_ctl_call_pos_commit_handlers(Self);
#endif
		CleanupTx(Self);
		//        TraceTxCommit(Self->UniqID,910);
		return 1;
	}
#endif
	//printf("TxCommitCMT - 3\n");
	if (TryFastUpdate(Self))
	{
		//#ifdef USE_TX_HANDLER
		//        if (!_ctl_call_prepare_commit_handlers(GET_HANDLERS(Self),GET_TX_ENV(Self))) {
		//              STATS_INC(_tl_num_cmt_aborts);
		//              AbortBackOffTx(Self) ;
		//              return 0 ;
		//        }
		//#endif
		// commit successfull
		STATS_INC(_tl_num_ok_commits);

#ifdef USE_TX_HANDLER
		_ctl_call_pos_commit_handlers(Self);
#endif
		//printf("TxCommitCMT - 4\n");
		CleanupTx(Self);
		//printf("TxCommitCMT - 4- cleaned up TX already! success commiting (return 1).\n");
		//        TraceTxCommit(Self->UniqID,920);

		return 1;
	}
	//    //    TraceTxCommit(Self->UniqID,930);
	STATS_INC(_tl_num_cmt_aborts);
	//printf("TxCommitCMT - Transaction is going to AbortBackOffTx. returning 0 (non successufll)\n");
	AbortBackOffTx(Self);
	return 0;
}	//TxCommitCMT

// Internal function to do the actual abort
static inline int AbortInternalCMT(Thread * const Self, Tx * tx)
{
	TraceTxAbort(Self->UniqID, 0);
	ASSERT_DEBUG(GET_TX(Self) != NULL);
	ASSERT_DEBUG(tx != NULL && tx->Mode == TTXN);


	STATS_INC(_tl_num_total_aborts);
	// Clean up after an abort.
	// Restore any modified locals
#ifndef DISABLE_LOCAL_VARS

	if (tx->LocalUndo.Put != tx->LocalUndo.List)
	{
		UndoLOCAL(&tx->LocalUndo);
	}
#endif

	// Done by txstart - TxReset(Self) ;
	tx->Mode = TABORTED;

	return 0;

}	//TxAbortInternalCMT

intptr_t TxLoadCMT(Thread * const Self, intptr_t volatile *addr)
{
	intptr_t Valu;

	//TraceEvent(Self->UniqID, 0, _tl_txload, addr, (intptr_t)0,(intptr_t)0);
	TraceTxLoad(Self->UniqID, 0, addr);

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxLoad must be called within an active transaction\n");

	STATS_INC(_tl_num_loads);

	vwLock volatile *LockFor = PSLOCK(addr);

	// Preserve the illusion of processor consistency in run-ahead mode.
	// Look-aside: check the wrSet for RAW hazards.
	// This is optional, but it improves the quality and fidelity
	// of the wrset and rdset compiled during speculative mode.
	// Consider using a Bloom filter on the addresses in wrSet to give us
	// a statistically fast out if the address doesn't appear in the set.
	Tx *ti;

#ifdef ENABLE_NESTING

	for (ti = GET_TX(Self); ti != NULL; ti = ti->upperTx)
#else

	ti = GET_TX(Self);
#endif

	{
		Log *const wr = &ti->wrSet;
		intptr_t msk = FILTERBITS(addr);
		if ((wr->BloomFilter & msk) == msk)
		{
			AVPair *p;
			for (p = wr->Put - 1; p >= wr->List; p--)
			{
				ASSERT_DEBUG(p->Addr != NULL);
				if (p->Addr == addr)
				{
					ASSERT_DEBUG(LockFor ==
						     PSLOCK(p->Addr));

					//                    TraceEvent(Self->UniqID, 20, _tl_txload, addr, (intptr_t)0,(intptr_t)p->ObjValu[0]);
					return p->ObjValu[0];
				}
			}
		}
	}

	/*COMMENT BY ANDRE (CBEA related comment):
	 * If Self->rv is modified to be set upon TxLoad(), be aware that on CBEA the SPE's depend on this rv, which is sent upon a TxStart();
	 */
	// TODO-FIXME:
	// Currently we set Self->rv in TxStart().
	// We might be better served to defer reading Self->rv
	// until the 1st transactional load.
	// if (Self->rv == 0) Self->rv = _GCLOCK ;

	//    vwLock rdv1 = LDLOCK(LockFor);
	//    MEMBARLDLD() ; // order lock and value read
	//rjfd
	//Valu = *(addr) ;
	//rjfd
	MEMBARLDLD();
	vwLock rdv2 = LDLOCK(LockFor);

	//    if (IS_VERSION(rdv1) && rdv1 <= Self->rv && rdv1 == rdv2) {
	if (IS_VERSION(rdv2) && rdv2 <= Self->rv)
	{
		//rjfd
		Valu = *(addr);
		//rjfd

		// We got here means:
		// 1) lock hasn't changed before and after the actual read
		// 2) lock is a version (it's not locked)
		// 3) version number is <= RV
		if (!Self->IsRO)
		{

			//        if(!IS_LOG_FULL(&GET_TX(Self)->rdSet)) {
			ASSERT(!IS_LOG_FULL(&GET_TX(Self)->rdSet));
			TrackLoad(Self, addr);
		}
		//TraceEvent(Self->UniqID, 40, _tl_txload, addr, (intptr_t)rdv2,(intptr_t)Valu);
		return Valu;
		//        } else {
		//            ASSERT(!ReadSetCoherent(Self,0));
		//        }

	}
	// The location is either currently locked or has been
	// updated since this txn started.  In the later case if
	// the read-set is otherwise empty we could simply re-load
	// Self->rv = _GCLOCK and try again.  If the location is
	// locked it's fairly likely that the owner will release
	// the lock by writing a versioned write-lock value that
	// is > Self->rv, so spinning provides little profit.

	STATS_INC(_tl_num_ld_aborts);

	AbortBackOffTx(Self);
	return 0;
}	//TxLoadCMT

void TxStoreCMT(Thread * const Self, intptr_t volatile *addr,
		intptr_t valu)
{
	TraceTxStore(Self->UniqID, 0, addr, valu);

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxStore must be called within an active transaction\n");

	ASSERT_MSG_DEBUG(!(Self->IsRO), "%d - Storing on a RO Tx\n",
			 Self->UniqID);

	STATS_INC(_tl_num_stores);

	// CONSIDER: spin briefly (bounded) while the object is locked,
	// periodically calling ReadSetCoherent(Self).
	//vwLock rdv ;
	//rdv = LDLOCK(LockFor) ;

	// Convert a redundant "idempotent" store to a tracked load.
	// This helps minimize the wrSet size and reduces false+ aborts.
	// Conceptually, "a = x" is equivalent to "if (a != x) a = x"
	// This is entirely optional
	Log *wr = &GET_TX(Self)->wrSet;
	MEMBARLDLD();
	wr->BloomFilter |= FILTERBITS(addr);

	if (!RecordStoreCMT(Self, addr, valu))
	{
		AbortBackOffTx(Self);
		return;
	}
	//TraceTxStore(Self->UniqID, 900, addr,valu);

	return;
}	// TxStoreCMT
