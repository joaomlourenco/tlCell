#ifndef CLOCK_H_
#define CLOCK_H_

#include <inttypes.h>
#include "tl.h"

//#define _USE_RDTSC_

// =====================>  Global Version-clock management

#define _GCLOCK  GClock[32]

#define _GVCONFIGURATION 4

#if !defined(_GVCONFIGURATION)
#define _GVCONFIGURATION 4
#endif

#if _GVCONFIGURATION == 1
#define _GVFLAVOR "GV1"
#define GVGenerateWV GVGenerateWV_GV1
#endif

#if _GVCONFIGURATION == 4
#define _GVFLAVOR "GV4"
#ifndef _USE_RDTSC_
#define GVGenerateWV GVGenerateWV_GV4
#else
#define GVGenerateWV GVGenerateWV_RDTSC
#endif
#endif

#if _GVCONFIGURATION == 1000
#define _GVFLAVOR "GV1000"
#define GVGenerateWV GVGenerateWV_GV1000
#endif

extern volatile vwLock GClock[64];

#if _GVCONFIGURATION != 1 && _GVCONFIGURATION != 4 && _GVCONFIGURATION != 1000
#error only GV1, GV4 and GV1000 are enabled
#endif

#if _GVCONFIGURATION == 1000
#warning GV1000 is NOT SAFE
#warning GV1000 is NOT SAFE
#warning GV1000 is NOT SAFE
#warning GV1000 is NOT SAFE
#warning GV1000 is NOT SAFE
#endif

// --------------------------------------------------------------
extern void GVInit();

extern inline vwLock GVRead();
extern inline vwLock GVGenerateWV_GV1(Thread * Self);
extern inline vwLock GVGenerateWV_GV4(Thread * Self);
extern inline vwLock GVGenerateWV_GV1000(Thread * Self);
#ifdef _USE_RDTSC_
extern inline vwLock GVGenerateWV_RDTSC(Thread * Self);
#endif

#endif /*CLOCK_H_ */
