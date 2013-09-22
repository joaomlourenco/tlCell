#include "gvclock.h"
#include "x86_sync_defns.h"
#include "logger.h"
#include <stdio.h>
#include <assert.h>

#ifdef _USE_RDTSC_
#include "rdtsc.h"
#endif

// =====================>  Global Version-clock management

// We use GClock[32] as the global counter.  It must be the sole occupant
// of its cache line to avoid false sharing.  Even so, accesses to
// GCLock will cause significant cache coherence & communication costs
// as it is multi-read multi-write.
volatile vwLock GClock[64] __attribute__ ((aligned(8)));

// -----------------------------------------------------------------------------

inline void GVInit()
{
	STLOCK(&_GCLOCK, 0ULL);
	ASSERT_DEBUG(LDLOCK(&_GCLOCK) == 0ULL);
}

inline vwLock GVRead()
{
#ifndef _USE_RDTSC_
	vwLock clock_value = LDLOCK(&_GCLOCK);
	return clock_value;
#else
        return rdtsc();
#endif
}

// GVGenerateWV():
//
// Conceptually, we'd like to fetch-and-add _GCLOCK.  In practice, however,
// that naive approach, while safe and correct, results in CAS contention
// and SMP cache coherency-related performance penalties.  As such, we
// use either various schemes (GV4,GV5 or GV6) to reduce traffic on _GCLOCK.
//
// Global Version-Clock invariant:
// I1: The generated WV must be > any previously observed (read) RV

// Simple fetch-and-add
inline vwLock GVGenerateWV_GV1(Thread * Self)
{
	vwLock old = AdjustClock(&_GCLOCK, 2);
	vwLock wv = old + 2;

	ASSERT_DEBUG(IS_VERSION(wv));
	ASSERT_MSG(wv != 0, "GV:OVERFLOW\n");
	//    if (wv == 0)
	//        TRACE ("GV:OVERFLOW\n") ;
	ASSERT_DEBUG(wv > Self->rv);
	return wv;
}

// Regarding GV4:
// The GV4 form of GVGenerateWV() does not have a CAS retry loop.
// If the CAS fails then we have 2 writers that are racing, trying to bump
// the global clock.  One increment succeeded and one failed.  Because the 2 writers
// hold locks at the time we bump, we know that their write-sets don't intersect.
// If the write-set of one thread intersects the read-set of the other then we
// know that one will subsequently fail validation (either because the lock
// associated with the read-set entry is held by the other thread, or
// because the other thread already made the update and dropped the lock,
// installing the new version #).  In this particular case it's safe if
// two threads call GVGenerateWV() concurrently and they both generate the same
// (duplicate) WV.  That is, if we have writers that concurrently try to increment
// the clock-version and then we allow them both to use the same wv.  The failing
// thread can "borrow" the wv of the successful thread.
inline vwLock GVGenerateWV_GV4(Thread * Self)
{
	//    vwLock gv = _GCLOCK ;
	vwLock gv = LDLOCK(&_GCLOCK);
	vwLock wv = gv + 2;
	vwLock k = CAS_VWLOCK(&_GCLOCK, gv, wv);
	if (k != gv)
		wv = k;
	ASSERT_DEBUG(IS_VERSION(wv));
	ASSERT_MSG(wv != 0, "GV:OVERFLOW\n");
	//    if (wv == 0)
	//        TRACE ("GV:OVERFLOW\n") ;
	ASSERT_DEBUG(wv > Self->rv);
	return wv;
}

inline vwLock GVGenerateWV_GV1000(Thread * Self)
{
	_GCLOCK += 2;
	return _GCLOCK;
}
#ifdef _USE_RDTSC_
inline vwLock GVGenerateWV_RDTSC(Thread * Self) {
  return rdtsc();
}
#endif

