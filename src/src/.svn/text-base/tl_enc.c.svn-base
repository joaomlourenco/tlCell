#ifdef USE_TX_HANDLER
#include "tl_handler.c"
#endif

//static inline void RecordStoreENC (Log * k, intptr_t volatile * addr, vwLock volatile * Lock, intptr_t cv) {
static inline int RecordStoreENC(Thread * const Self,
				 intptr_t const volatile *const addr,
				 vwLock const cv, unsigned int const txid)
{
	Log *k = &GET_TX(Self)->wrSet;
	ASSERT_DEBUG(addr != NULL);
	ASSERT_DEBUG(IS_VERSION(cv) || OwnerOf(cv) == Self);
	ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(addr))) == Self);
	ASSERT_DEBUG(IS_VERSION(cv) || LDLOCK(PSLOCK(addr)) == cv);

	// NOTE: if we run in consistent mode, addr may be an invalid pointer and
	// the copy to undo log will SEGFAULT. In this case when undoing log
	// we must restore only the lock (not the value)

	ASSERT(!IS_LOG_FULL(k));

	AVPair *p = k->Put;
	k->Put++;
	p->rdv = cv;
#ifdef ENABLE_NESTING

	p->TxId = txid;
#endif

	ASSERT_DEBUG(p->Held == 0);
	p->Addr = LINE_BASE(addr);
	p->ObjSize = 0;

	// This may segfault.... It must be caught by fault handler with a longjump
	LINE_COPY(p->ObjValu, LINE_BASE(addr));

	// On ENC: Held=1 means value is on the undo log. (On undo value will be restored)
	p->Held = 1;
	return 1;
}	//RecordStoreENC

//size in bytes
static inline int RecordStoreObjectENC(Thread * const Self,
				       intptr_t const volatile *const addr,
				       unsigned int size, vwLock const cv,
				       unsigned int const txid)
{
	Log *k = &GET_TX(Self)->wrSet;
	ASSERT_DEBUG(size > 0 && size <= MAX_OBJ_SIZE * sizeof(intptr_t));
	ASSERT_DEBUG(addr != NULL);
	ASSERT_DEBUG(IS_VERSION(cv) || OwnerOf(cv) == Self);
	ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(addr))) == Self);
	ASSERT_DEBUG(IS_VERSION(cv) || LDLOCK(PSLOCK(addr)) == cv);

	// NOTE: if we run in consistent mode, addr may be an invalid pointer and
	// the copy to undo log will SEGFAULT. In this case when undoing log
	// we must restore only the lock (not the value)

	ASSERT(!IS_LOG_FULL(k));

	AVPair *p = k->Put;
	k->Put++;
	p->rdv = cv;
#ifdef ENABLE_NESTING

	p->TxId = txid;
#endif

	ASSERT_DEBUG(p->Held == 0);
	p->Addr = (intptr_t *) addr;
	p->ObjSize = size;

	// This may segfault.... It must be caught by fault handler with a longjump
	//////    memcpy(p->ObjValu, (void*)addr, size);
	BYTES_COPY(p->ObjValu, (void *)addr, (int)size);

	// On ENC: Held=1 means value is on the undo log. (On undo value will be restored)
	p->Held = 1;
	return 1;
}	//RecordStoreObjectENC



//Restore old address values and release locks to old version number
static inline void UndoLocksAndValuesENC(Thread * const Self, Tx * tx)
{
	AVPair *p;

	ASSERT_DEBUG(tx != NULL && tx->Mode == TTXN);
	ASSERT_DEBUG(tx->upperTx == NULL);	//ensure this is the main tx

	for (p = tx->wrSet.Put - 1; p >= tx->wrSet.List; p--)
	{
		ASSERT_DEBUG(p->Addr != NULL);
		ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(p->Addr))) == Self);
		ASSERT_DEBUG((LDLOCK(PSLOCK(p->Addr)) ==
			      (ADDR_2_VWLOCK(p))) == IS_VERSION(p->rdv));

		//TraceEvent(Self->UniqID, 20, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
#ifdef ENABLE_TRACE
		{
			int res[] = { 20, (int)p->Addr, 0, 0 };
			TraceEvent(Self->UniqID, _tl_txabort, res);
		}
#endif
		if (p->Held)
		{
			//            TraceEvent(Self->UniqID, 30, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
			p->Held = 0;

			if (p->ObjSize == 0)
			{
				// word undo copy
				//                TraceEvent(Self->UniqID, 40, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
				LINE_COPY(LINE_BASE(p->Addr), p->ObjValu);

			} else
			{
				// object undo copy
				//                TraceEvent(Self->UniqID, 50, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
				ASSERT_DEBUG(p->ObjSize > 0
					     && p->ObjSize <=
					     MAX_OBJ_SIZE *
					     sizeof(intptr_t));
				BYTES_COPY((void *)p->Addr, p->ObjValu,
					   p->ObjSize);
			}
			//        } else {
			//            // Segfault occured during copy to undo log
			//            //            TraceEvent(Self->UniqID, 60, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
			//
			//            if(p->ObjSize!=0) {
			//                // object undo copy
			//                //                TraceEvent(Self->UniqID, 70, _tl_txabort, p->Addr, (intptr_t)0,(intptr_t)0);
			//            }

		}
		p->ObjSize = -1;

	}

	MEMBARSTST();

	// This is to avoid the situation where:
	// TX1 opens variable A for read
	// TX2 opens variable A for write
	// TX2 does a write in place
	// TX1 reads the dirty variable A
	// TX2 aborts (restoring the original version number)
	// TX1 commits - it does not detect dirty read
	vwLock wv = 0;
	if (tx->wrSet.Put != tx->wrSet.List)
	{
		wv = GVGenerateWV(Self);
	}

	for (p = tx->wrSet.Put - 1; p >= tx->wrSet.List; p--)
	{
		ASSERT_DEBUG(wv != 0);

		//TraceEvent(Self->UniqID, 80, _tl_txabort, p->Addr, (intptr_t)PSLOCK(p->Addr),(intptr_t)p->rdv);
#ifdef ENABLE_TRACE
		{
			int res[] =
				{ 80, (int)p->Addr, (int)PSLOCK(p->Addr),
				    (int)p->rdv };
			TraceEvent(Self->UniqID, _tl_txabort, res);
		}
#endif
		vwLock volatile *LockFor = PSLOCK(p->Addr);
		vwLock LockVal = LDLOCK(LockFor);
		if (LockVal == ADDR_2_VWLOCK(p))
		{
			ASSERT_DEBUG(IS_VERSION(p->rdv));
			STLOCK(LockFor, wv);

		} else
		{
			ASSERT_DEBUG(IS_LOCKED(p->rdv));
		}

		p->Addr = NULL;	// diagnostic hygiene
	}

	MEMBARSTLD();	// ?????????????


}	//UndoLocksAndValuesENC

#ifdef ENABLE_NESTING
//Restore old address values and release locks to old version number (for subtx only)
static inline void UndoValuesENC(Thread * const Self, Tx * tx)
{
	AVPair *p;
	//    ASSERT_DEBUG(GET_TX(Self)!=NULL && GET_TX(Self)->Mode == TTXN);
	//    ASSERT_DEBUG(GET_TX(Self)->upperTx!=NULL);//ensure this is a subtx
	ASSERT_DEBUG(tx != NULL && tx->Mode == TTXN);
	ASSERT_DEBUG(tx->upperTx != NULL);	//ensure this is the main tx

	for (p = tx->wrSet.Put - 1; p >= tx->wrSet.List; p--)
	{
		ASSERT_DEBUG(p->Addr != NULL);
		ASSERT_DEBUG(p->Held == 1);
		ASSERT_DEBUG(OwnerOf(LDLOCK(PSLOCK(p->Addr))) == Self);
		p->Held = 0;
		LINE_COPY(LINE_BASE(p->Addr), p->ObjValu);

		//        ASSERT_DEBUG (LDLOCK(PSLOCK(p->Addr)) == ADDR_2_VWLOCK(p)) ;
		AVPair volatile *avp =
			GetAVPairOf(LDLOCK(PSLOCK(p->Addr)));
		if (avp->TxId == tx->TxId)
		{
			// the lock was acquired by this Tx - undo value and unlocklock
			// WARNING: THIS MAKES THE ALGORITHM NOT 2 PHASE LOCKING!!!
			ASSERT_DEBUG(IS_VERSION(p->rdv));
			MEMBARSTST();
			STLOCK(PSLOCK(p->Addr), p->rdv);
		} else
		{
			// this variable was already locked by parent Tx - do not unlock
			ASSERT_DEBUG(!IS_VERSION(p->rdv));
		}

		p->Addr = NULL;	// diagnostic hygiene
	}

	MEMBARSTLD();	// This may be necessary in case we unlocked some address ?????

}	//UndoValuesENC
#endif

// release the locks after a successfull commit and update lock version
// used on ENC-commit
static inline void ReleaseLocksENC(Thread * const Self, vwLock wv)
{
	ASSERT_DEBUG(GET_TX(Self) != NULL && GET_TX(Self)->Mode == TTXN);
	ASSERT_DEBUG(IS_VERSION(wv));
	AVPair *p;
	for (p = GET_TX(Self)->wrSet.List; p != GET_TX(Self)->wrSet.Put;
	     p++)
	{
		ASSERT_DEBUG(p->Addr != NULL);

		vwLock volatile *LockFor = PSLOCK(p->Addr);
		vwLock LockVal = LDLOCK(LockFor);

		ASSERT_DEBUG(p->Held == 1);
		p->Held = 0;

		if (LockVal != ADDR_2_VWLOCK(p))
		{
			// happens with either lock colision or duplicate write to the same address
			// we can't assert the owner of the lock is Self, because this TX may have
			// released it on a previous iteration and it may have already been taken by another TX
			continue;
		}
		ASSERT_DEBUG(OwnerOf(LockVal) == Self);
		ASSERT_DEBUG(LockVal == ADDR_2_VWLOCK(p));
		ASSERT_DEBUG(IS_VERSION(p->rdv));
		ASSERT_DEBUG(wv > p->rdv);
		//        TraceEvent(Self->UniqID, 55, _tl_txcommit, p->Addr, (intptr_t)LockFor,(intptr_t)wv);
		STLOCK(LockFor, wv);
		//        TraceEvent(Self->UniqID, 56, _tl_txcommit, p->Addr, (intptr_t)LockFor,(intptr_t)*LockFor);

		p->Addr = NULL;	// diagnostic hygiene
		p->ObjSize = -1;

	}
	GET_TX(Self)->wrSet.Put = GET_TX(Self)->wrSet.List;

}	//ReleaseLocksENC





// ----------------------------------------------
// ENC
// ----------------------------------------------

int TxCommitENC(Thread * const Self)
{

	TraceTxCommit(Self->UniqID, 0);

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "Trying to commit but not within an active transaction\n");

	MEMBARSTLD();

#ifdef ENABLE_NESTING

	if (GET_TX(Self)->upperTx != NULL)
	{
		// Nested TX
		// Optionally we could call ReadSetCoherent
		AppendList(&GET_TX(Self)->upperTx->rdSet,
			   &GET_TX(Self)->rdSet, MAX_LIST_SIZE);
		AppendListWRENC(GET_TX(Self),
				&GET_TX(Self)->upperTx->wrSet,
				&GET_TX(Self)->wrSet, MAX_LIST_SIZE);
#ifndef DISABLE_LOCAL_VARS

		AppendList(&GET_TX(Self)->upperTx->LocalUndo,
			   &GET_TX(Self)->LocalUndo, Self->LocalUndoSize);
#endif

		CleanupTx(Self);
		TraceTxCommit(Self->UniqID, 900);
		return 1;
	}
#endif

	//        TraceTxCommit(Self->UniqID,20);
	//    AVPair * p;
	//    Log * const wr = &GET_TX(Self)->wrSet ;
	//    Log * const rd = &GET_TX(Self)->rdSet ;
	//    for (p = rd->List ; p != rd->Put ; p++) {
	//        TraceEvent(Self->UniqID, 25, _tl_txcommit, p->Addr, 0,(intptr_t)0);
	//    }
	//    for (p = wr->List ; p != wr->Put ; p++) {
	//        TraceEvent(Self->UniqID, 26, _tl_txcommit, p->Addr, p->Held,(intptr_t)p->rdv);
	//    }

	//#if (! (defined OBJ_STM && defined NO_VFY)) && (! defined DISABLE_TXSTERILIZE_EXT)
#if (1 && !( (defined OBJ_STM && defined NO_VFY) || (defined DISABLE_TXSTERILIZE_EXT)))
	// Fast-path: Optional optimization for pure-readers
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

	if (!ReadSetCoherent(Self, 0))
	{
		STATS_INC(_tl_num_cmt_aborts);
		TraceTxCommit(Self->UniqID, 910);
		AbortBackOffTx(Self);
		return 0;
	}
#ifdef USE_TX_HANDLER
	if (!_ctl_call_prepare_commit_handlers(Self))
	{
		STATS_INC(_tl_num_cmt_aborts);
		AbortBackOffTx(Self);
		return 0;
	}
#endif

	TraceTxCommit(Self->UniqID, 30);

	vwLock wv = GVGenerateWV(Self);

	ReleaseLocksENC(Self, wv);
	MEMBARSTLD();	//Do we need a MEMBAR ????

	STATS_INC(_tl_num_ok_commits);

#ifdef USE_TX_HANDLER
	_ctl_call_pos_commit_handlers(Self);
#endif

	CleanupTx(Self);

	TraceTxCommit(Self->UniqID, 920);

	return 1;
}	//TxCommitENC

// Internal function to do the actual abort
static inline int AbortInternalENC(Thread * const Self, Tx * tx)
{
	TraceTxAbort(Self->UniqID, 0);
	ASSERT_DEBUG(GET_TX(Self) != NULL);
	ASSERT_DEBUG(tx != NULL && tx->Mode == TTXN);

	STATS_INC(_tl_num_total_aborts);

#ifdef ENABLE_NESTING

	if (tx->upperTx == NULL)
	{
		// if this is the main tx undo the values and unlock everything
		UndoLocksAndValuesENC(Self, tx);
	} else
	{
		// if this is a sub tx undo only the values
		UndoValuesENC(Self, tx);
	}
#else
	UndoLocksAndValuesENC(Self, tx);
#endif

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

}	//TxAbortInternalENC


// Load transactional value
intptr_t TxLoadENC(Thread * const Self, intptr_t volatile *addr)
{
	intptr_t Valu;

	//TraceEvent(Self->UniqID, 0, _tl_txload, addr, (intptr_t)0,(intptr_t)0);
	TraceTxLoad(Self->UniqID, 0, addr);

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxLoad must be called within an active transaction\n");


	STATS_INC(_tl_num_loads);

	vwLock volatile *LockFor = PSLOCK(addr);
	//    vwLock rdv1 = LDLOCK(LockFor);
	//    MEMBARLDLD() ; // order lock and value read
	Valu = *(addr);
	MEMBARLDLD();
	vwLock rdv2 = LDLOCK(LockFor);

	//    if ((IS_VERSION(rdv1) && rdv1 <= Self->rv && rdv1 == rdv2) || OwnerOf(rdv1)==Self) {
	if ((IS_VERSION(rdv2) && rdv2 <= Self->rv)
	    || OwnerOf(rdv2) == Self)
	{
		// We got here means:
		// Either the lock was already acquired by this thread OR:
		// 1) lock hasn't changed before and after the actual read
		// 2) and lock is a version (it's not locked)
		// 3) version number is <= RV
		ASSERT_DEBUG(IS_VERSION(rdv2)
			     || (OwnerOf(rdv2) == Self
				 && IS_VERSION(GetAVPairOf(rdv2)->rdv)
				 && GetAVPairOf(rdv2)->rdv <= Self->rv));
		ASSERT_DEBUG(IS_LOCKED(rdv2) || rdv2 <= Self->rv);

		if (!Self->IsRO)
		{
			//        if(!IS_LOG_FULL(&GET_TX(Self)->rdSet)) {
			ASSERT(!IS_LOG_FULL(&GET_TX(Self)->rdSet));
			TrackLoad(Self, addr);
		}
		//        TraceEvent(Self->UniqID, 910, _tl_txload, addr, (intptr_t)rdv2,(intptr_t)Valu);

		return Valu;
	}

	//ASSERT_DEBUG(rdv1 > Self->rv || IS_LOCKED(rdv1) || rdv1!=rdv2);
	//ASSERT_DEBUG(rdv2 > Self->rv || IS_LOCKED(rdv2));

	STATS_INC(_tl_num_ld_aborts);

	//    TraceEvent(Self->UniqID, 915, _tl_txload, addr, (intptr_t)0,(intptr_t)0);
	AbortBackOffTx(Self);
	return 0;

}	//TxLoadENC


void TxStoreENC(Thread * const Self, intptr_t volatile *addr,
		intptr_t valu)
{
	TraceTxStore(Self->UniqID, 0, addr, valu);

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxStore must be called within an active transaction\n");

	ASSERT_MSG_DEBUG(!(Self->IsRO), "%d - Storing on a RO Tx\n",
			 Self->UniqID);

	STATS_INC(_tl_num_stores);

	vwLock volatile *LockFor = PSLOCK(addr);
	vwLock LockVal = LDLOCK(LockFor);
	if (IS_VERSION(LockVal))
	{
		//lock and create undo copy
		if (LockVal > Self->rv)
		{
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 910, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 910, addr, 0);
			AbortBackOffTx(Self);
			return;
		}

		Log *wr = &GET_TX(Self)->wrSet;
		ASSERT_DEBUG(wr->Put != NULL);

		// before acquiring the lock we must make sure that there is enough room in the log
		// otherwise we may reach a situation where we have a log but no lock or vice versa
		ASSERT_DEBUG(!IS_LOG_FULL(wr));

		//acquire lock
		vwLock ret =
			CAS_VWLOCK(LockFor, LockVal,
				   (vwLock) ADDR_2_VWLOCK(wr->Put));
		if (ret != LockVal)
		{
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 940, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 940, addr, 0);
			AbortBackOffTx(Self);
			return;
		}
		// create undo copy - do it after CAS so we get a locked value
		// keep in mind there is an interval where the locktab ptr is pointing to an empty log
		//        RecordStoreENC (wr, addr, LockFor, LockVal);
		ASSERT_DEBUG(!IS_LOG_FULL(wr));
#ifdef ENABLE_NESTING

		RecordStoreENC(Self, addr, LockVal, GET_TX(Self)->TxId);
#else

		RecordStoreENC(Self, addr, LockVal, 0);
#endif

		*addr = valu;
	} else
	{
		//it's a lock
		if (OwnerOf(LockVal) != Self)
		{
			//someone else owns the lock
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 960, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 960, addr, 0);
			AbortBackOffTx(Self);
			return;
		} else
		{
			//lock was already ours
			AVPair volatile *avp = GetAVPairOf(LockVal);
#ifdef ENABLE_NESTING

			if (avp->TxId != GET_TX(Self)->TxId)
			{
				//lock was acquired by either a parentTx or a subTx -> create undo copy
				if (!RecordStoreENC
				    (Self, addr, LockVal,
				     GET_TX(Self)->TxId))
				{
					//TraceEvent(Self->UniqID, 950, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
					TraceTxStore(Self->UniqID, 950,
						     addr, 0);
					AbortBackOffTx(Self);
					return;
				}
			}
#endif

			ASSERT_DEBUG(avp->rdv <= Self->rv);

			if (avp->Addr != addr)
			{
				//There was a lock colision with another address. Create undo copy but don't lock
#ifdef ENABLE_NESTING
				RecordStoreENC(Self, addr, LockVal,
					       GET_TX(Self)->TxId);
#else

				RecordStoreENC(Self, addr, LockVal, 0);
#endif

			}
			*addr = valu;
		}
	}
	//TraceEvent(Self->UniqID, 970, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
	TraceTxStore(Self->UniqID, 970, addr, 0);

	return;
}	// TxStoreENC

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------


// Open Object for Read
int TxOpenReadENC(Thread * const Self, void volatile *addr)
{

	//TraceEvent(Self->UniqID, 0, _tl_txload, addr, (intptr_t)0,(intptr_t)0);
	TraceTxLoad(Self->UniqID, 0, addr);
	if (addr == NULL)
		return 0;

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxOpenRead must be called within an active transaction\n");


	STATS_INC(_tl_num_loads);

#if (defined OBJ_STM && defined NO_VFY)

	if (!IS_LOG_FULL(&GET_TX(Self)->rdSet))
	{
		TrackLoad(Self, addr);

		//TraceEvent(Self->UniqID, 910, _tl_txload, addr, (intptr_t)0,(intptr_t)rdv1);

		return 1;
	} else
	{
		ASSERT(!ReadSetCoherent(Self, 0));
	}
#else
	if (Self->IsRO)
	{
		return 1;
	}

	ASSERT(!IS_LOG_FULL(&GET_TX(Self)->rdSet));
	TrackLoad(Self, addr);

	//TraceEvent(Self->UniqID, 910, _tl_txload, addr, (intptr_t)0,(intptr_t)rdv1);

	return 1;
#endif


	STATS_INC(_tl_num_ld_aborts);

	//    TraceEvent(Self->UniqID, 915, _tl_txload, addr, (intptr_t)0,(intptr_t)0);
	AbortBackOffTx(Self);
	return 0;

}	//TxOpenReadENC

// Open Object for Write
//size in bytes
int TxOpenWriteENC(Thread * const Self, void volatile *addr,
		   unsigned int size)
{
	TraceTxStore(Self->UniqID, 0, addr, (intptr_t) size);

	if (addr == NULL)
		return 0;

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxOpenWrite must be called within an active transaction\n");

	ASSERT_MSG_DEBUG(!(Self->IsRO), "%d - Storing on a RO Tx\n",
			 Self->UniqID);

	STATS_INC(_tl_num_stores);

	vwLock volatile *LockFor = PSLOCK(addr);
	vwLock LockVal = LDLOCK(LockFor);
	if (IS_VERSION(LockVal))
	{
		// lock and create undo copy

		if (LockVal > Self->rv)
		{
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 910, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 910, addr, 0);
			AbortBackOffTx(Self);
			return 0;
		}
#if (1 && defined OBJ_STM && defined NO_VFY)
		// Validate read set before write - avoid acquiring locks when read set is not coherent
		if (!ReadSetCoherent(Self, 0))
		{
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 930, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 930, addr, 0);
			AbortBackOffTx(Self);
			return 0;
		}
#endif

		Log *wr = &GET_TX(Self)->wrSet;
		ASSERT_DEBUG(wr->Put != NULL);

		// before acquiring the lock we must make sure that there is enough room in the log
		// otherwise we may reach a situation where we have a log but no lock or vice versa
		ASSERT_DEBUG(!IS_LOG_FULL(wr));

		//acquire lock
		vwLock ret =
			CAS_VWLOCK(LockFor, LockVal,
				   (vwLock) ADDR_2_VWLOCK(wr->Put));
		if (ret != LockVal)
		{
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 940, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 940, addr, 0);
			AbortBackOffTx(Self);
			return 0;
		}
		// create undo copy - do it after CAS so we get a locked value
		// keep in mind there is an interval where the locktab ptr is pointing to an empty log
		ASSERT_DEBUG(!IS_LOG_FULL(wr));
#ifdef ENABLE_NESTING

		RecordStoreObjectENC(Self, addr, size, LockVal,
				     GET_TX(Self)->TxId);
#else

		RecordStoreObjectENC(Self, addr, size, LockVal, 0);
#endif

	} else
	{
		//it's a lock
		if (OwnerOf(LockVal) != Self)
		{
			//someone else owns the lock
			STATS_INC(_tl_num_st_aborts);
			//TraceEvent(Self->UniqID, 960, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
			TraceTxStore(Self->UniqID, 960, addr, 0);
			AbortBackOffTx(Self);
			return 0;
		} else
		{
			//lock was already ours
			AVPair volatile *avp = GetAVPairOf(LockVal);
#ifdef ENABLE_NESTING

			if (avp->TxId != GET_TX(Self)->TxId)
			{
				//lock was acquired by either a parentTx or a subTx -> create undo copy
				if (!RecordStoreObjectENC
				    (Self, addr, size, LockVal,
				     GET_TX(Self)->TxId))
				{
					AbortBackOffTx(Self);
					//TraceEvent(Self->UniqID, 950, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
					TraceTxStore(Self->UniqID, 950,
						     addr, 0);
					return 0;
				}
			}
#endif

			ASSERT_DEBUG(avp->rdv <= Self->rv);

			if (avp->Addr != addr)
			{
				//There was a lock colision with another address. Create undo copy but don't lock.
#ifdef ENABLE_NESTING
				RecordStoreObjectENC(Self, addr, size,
						     LockVal,
						     GET_TX(Self)->TxId);
#else

				RecordStoreObjectENC(Self, addr, size,
						     LockVal, 0);
#endif

			}

		}
	}
	//TraceEvent(Self->UniqID, 970, _tl_txstore, addr, (intptr_t)0,(intptr_t)0);
	TraceTxStore(Self->UniqID, 970, addr, 0);

	return 1;
}	// TxOpenWriteENC

int TxVerifyAddrENC(Thread * const Self, void volatile *addr)
{
#ifdef ENABLE_TRACE
	//TraceEvent(Self->UniqID, 0, _tl_txvfy, addr, (intptr_t)0,(intptr_t)0);
	{
		int res[] = { 0, (int)addr, 0, 0 };
		TraceEvent(Self->UniqID, _tl_txvfy, res);
	}
#endif
	if (addr == NULL)
		return 0;

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxVerify must be called within an active transaction\n");

	STATS_INC(_tl_num_vfys);

	MEMBARLDLD();
	vwLock volatile *LockFor = PSLOCK(addr);
	vwLock rdv1 = LDLOCK(LockFor);

	if ((IS_VERSION(rdv1) && rdv1 <= Self->rv)
	    || OwnerOf(rdv1) == Self)
	{
		ASSERT_DEBUG(IS_VERSION(rdv1)
			     || (OwnerOf(rdv1) == Self
				 && IS_VERSION(GetAVPairOf(rdv1)->rdv)));
		//        TraceEvent(Self->UniqID, 910, _tl_txvfy, addr, (intptr_t)0,(intptr_t)rdv1);
		return 1;
	}

	STATS_INC(_tl_num_vfy_aborts);

	//    TraceEvent(Self->UniqID, 915, _tl_txvfy, addr, (intptr_t)0,(intptr_t)0);
	AbortBackOffTx(Self);
	return 0;
}


int TxVerifyLogAddrENC(Thread * const Self, void volatile *addr)
{
	//TraceEvent(Self->UniqID, 0, _tl_txvfy, addr, (intptr_t)0,(intptr_t)0);
#ifdef ENABLE_TRACE
	{
		int res[] = { 0, (int)addr, 0, 0 };
		TraceEvent(Self->UniqID, _tl_txvfy, res);
	}
#endif
	if (addr == NULL)
		return 0;

	ASSERT_MSG_DEBUG(GET_TX(Self) != NULL
			 && GET_TX(Self)->Mode == TTXN,
			 "TxVerify must be called within an active transaction\n");

	STATS_INC(_tl_num_vfys);

	MEMBARLDLD();
	vwLock volatile *LockFor = PSLOCK(addr);
	vwLock rdv1 = LDLOCK(LockFor);

	if ((IS_VERSION(rdv1) && rdv1 <= Self->rv)
	    || OwnerOf(rdv1) == Self)
	{
		ASSERT_DEBUG(IS_VERSION(rdv1)
			     || (OwnerOf(rdv1) == Self
				 && IS_VERSION(GetAVPairOf(rdv1)->rdv)));
		//        TraceEvent(Self->UniqID, 910, _tl_txvfy, addr, (intptr_t)0,(intptr_t)rdv1);

		ASSERT(!IS_LOG_FULL(&GET_TX(Self)->rdSet));
		TrackLoad(Self, addr);

		return 1;

	}

	STATS_INC(_tl_num_vfy_aborts);

	//    TraceEvent(Self->UniqID, 915, _tl_txvfy, addr, (intptr_t)0,(intptr_t)0);
	AbortBackOffTx(Self);
	return 0;
}
