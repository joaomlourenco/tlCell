#ifndef X86_SYNC_DEFNS_H_
#define X86_SYNC_DEFNS_H_

#if defined LIB_COMPILATION && defined HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>

#define CAS32_internal(_a, _o, _n)                         \
	({ __typeof__(_o) __o = _o;			   \
		__asm__ __volatile__(			   \
			"lock cmpxchg %3,%1"		   \
			: "=a" (__o), "=m" (*(volatile unsigned int *)(_a)) \
			:  "0" (__o), "r" (_n) );			\
		__o;                                                    \
	})

// ONLY WORKS WITH x86_64
#define CAS64_internal(_a, _o, _n)                               \
	({ __typeof__(_o) __o = _o;				 \
		__asm__ __volatile__(				 \
			"lock cmpxchg %3,%1"			 \
			: "=a" (__o), "=m" (*(volatile unsigned long long *)(_a)) \
			:  "0" (__o), "r" (_n) );			\
		__o;							\
	})

/*
   //equiv to atomic{ ret_val=*_a ; *_a=_n ; }
   #define FAS32_internal(_a, _n)                             \
   ({ __typeof__(_n) __o;                                     \
   __asm__ __volatile__(                                           \
   "lock xchg %0,%1"                                       \
   : "=r" (__o), "=m" (*(volatile unsigned int *)(_a))     \
   :  "0" (_n) );                                                  \
   __o;                                                    \
   })

   // ONLY WORKS WITH x86_64
   //equiv to atomic{ ret_val=*_a ; *_a=_n ; }
   #define FAS64_internal(_a, _n)                                   \
   ({ __typeof__(_n) __o;                                           \
   __asm__ __volatile__(                                                 \
   "lock xchg %0,%1"                                             \
   : "=r" (__o), "=m" (*(volatile unsigned long long *)(_a))     \
   :  "0" (_n) );                                                        \
   __o;                                                          \
   })
 */

// ONLY WORKS WITH x86_32
#define CAS8B_internal(_a, _o, _n)                               \
  ({ __typeof__(_o) __o = _o;					 \
	  __typeof__(_n) __n = (_n);				 \
	  __typeof__(_n) _nh = (__n >> 32);			 \
	  __typeof__(_n) _nl = (__n);				 \
	  __asm__ __volatile__(					 \
		  "movl %3, %%ecx;"				 \
		  "movl %4, %%ebx;"				 \
		  "lock cmpxchg8b %1"				 \
		  : "=A" (__o), "=m" (*(volatile unsigned long long *)(_a)) \
		  : "0" (__o), "m" (_nh), "m" (_nl)			\
		  : "ebx", "ecx" );					\
	  __o;                                                          \
  })

// _n = LOAD64(_a) <=> n <= *_a
#define LOAD_64_sse(_a)					\
  ({	__typeof__(_a) __a = (_a);                      \
	  _u64 __n;					\
	  __asm__ __volatile__(				\
		  "movl %1, %%eax;"			\
		  "movlps (%%eax), %%xmm0;"		\
		  "movlps %%xmm0, %0"			\
		  : "=m" (__n)				\
		  : "m" ((volatile unsigned long long *)(__a))	\
		  : "%xmm0", "%eax" );				\
	  __n;})

// STORE64(_a, _n) <=> *_a <= _n
#define STORE_64_sse(_a, _n)				 \
  ({	__typeof__(_a) __a = (_a);                       \
	  _u64 __n = (_n);				 \
	  __asm__ __volatile__(				 \
		  "movlps %1, %%xmm0;"			 \
		  "movl %0, %%eax;"			 \
		  "movlps %%xmm0, (%%eax)"		 \
		  : "=m" ((volatile unsigned long long *)(__a)) \
		  : "m" (__n)                                   \
		  : "%xmm0", "%eax" );                          \
  })

// Indirect stringification.  Doing two levels allows the parameter to be a
// macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
// converts to "bar".

#define __stringify_1(x)        #x
#define __stringify(x)          __stringify_1(x)

//#define __powerpc64__
#ifdef __powerpc64__
#define __SUBARCH_HAS_LWSYNC
#endif

#ifdef __SUBARCH_HAS_LWSYNC
#    define LWSYNC	lwsync
#else
#    define LWSYNC	sync
#endif

#define CONFIG_SMP
#ifdef CONFIG_SMP
#define ISYNC_ON_SMP    "\n\tisync\n"
#define LWSYNC_ON_SMP   __stringify(LWSYNC) "\n"
#else
#define ISYNC_ON_SMP
#define LWSYNC_ON_SMP
#endif

static __inline__ unsigned long __cmpxchg_u64(volatile unsigned long long *p, unsigned long old, unsigned long new)
{
        unsigned long prev;

        __asm__ __volatile__ (

        		LWSYNC_ON_SMP
        		"1:     ldarx   %0,0,%2         # __cmpxchg_u64\n\
        		cmpd    0,%0,%3\n\
        		bne-    2f\n\
        		stdcx.  %4,0,%2\n\
        		bne-    1b"
        		ISYNC_ON_SMP
        		"\n\
        		2:"
        		: "=&r" (prev), "+m" (*p)
        		: "r" (p), "r" (old), "r" (new)
        		: "cc", "memory");

        return prev;
}

//ANDRE: Notice, changed first argument of cmpxchg from (volatile unsigned int *p) -> (volatile unsigned long int *p)
#define PPC405_ERR77(ra,rb)
static __inline__ unsigned long __cmpxchg_u32(volatile unsigned long int *p, unsigned long old, unsigned long new)
{
        unsigned int prev;

        __asm__ __volatile__ (
        LWSYNC_ON_SMP
"1:     lwarx   %0,0,%2         # __cmpxchg_u32\n\
        cmpw    0,%0,%3\n\
        bne-    2f\n"
        PPC405_ERR77(0,%2)
"	stwcx.  %4,0,%2\n\
        bne-    1b"
        ISYNC_ON_SMP
        "\n\
2:"
   	: "=&r" (prev), "+m" (*p)
        : "r" (p), "r" (old), "r" (new)
        : "cc", "memory");

        return prev;
}


//#define X86_32__32bitversionclock
//#define X86_32__64bitversionclock
//#define X86_64
#define CPU_CELL

#ifdef CPU_X86
#  ifdef SUPPORT_SSE	// x86, sse
#    define CASIO CAS32_internal
#    define CASPO CAS32_internal
#    define __LDLOCK(a)    LOAD_64_sse(a)
#    define __STLOCK(a,v) STORE_64_sse(a,v)
#    define __CAS_VWLOCK CAS8B_internal
#    define IS_LOCK64_OK(x) (((HIGH_32BITS & x) == 0ULL) && IS_LOCKED(x))
#    define RDTICK RDTICK_X86_32
typedef unsigned long long vwLock;
#    define MB()  __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")
#    define WMB() __asm__ __volatile__ ("" : : : "memory")
#    define RMB() MB()
#  else // x86
#    define CASIO CAS32_internal
#    define CASPO CAS32_internal
#    define __LDLOCK(a)    (*(a))
#    define __STLOCK(l,n) do{*(l) = n ;}while(0)
#    define __CAS_VWLOCK CAS32_internal
#    define IS_LOCK64_OK(x) (1)
#    define RDTICK RDTICK_X86_32
typedef unsigned int vwLock;

#    define MB()  __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")
#    define WMB() __asm__ __volatile__ ("" : : : "memory")
#    define RMB() MB()
#  endif // sse
#endif // x86

#ifdef CPU_X86_64	// x86_64
#  define CASIO CAS32_internal
#  define CASPO CAS64_internal
#  define __LDLOCK(a)    (*(a))
#  define __STLOCK(l,n) do{*(l) = n ;}while(0)
#  define __CAS_VWLOCK CAS64_internal
#  define IS_LOCK64_OK(x) (1)
#  define RDTICK RDTICK_X86_64
typedef unsigned long long vwLock;	// (Version,LOCKBIT)

#  define MB()    asm volatile("mfence":::"memory")
#  define WMB()   asm volatile("" ::: "memory")
#  define RMB()   asm volatile("lfence":::"memory")

#endif




#ifdef CPU_CELL
//#define CASIO(a,b,c) 1
#define CASIO(a,b,c) __cmpxchg_u32((intptr_t *)a,b,c)
#define CASPO(a,b,c) __cmpxchg_u64((intptr_t *)a,b,c)
//#define CASPO(a,b,c) 1
#define __LDLOCK(a) (*(a))
#define __STLOCK(l,n) do{*(l) = n ;} while(0)
#define __CAS_VWLOCK(a,b,c) __cmpxchg_u64((volatile unsigned long long *)a,b,c)
#define IS_LOCK64_OK(x) (1)
#define RDTICK rdtsc
typedef unsigned long long vwLock;	// (Version,LOCKBIT)
#define MB() //in order execution=) no need for memory Barriers
#define WMB()//in order execution=)
#define RMB()//in order execution=)
#endif




#define MEMBARSTLD() MB()
#define MEMBARSTST() WMB()
#define MEMBARLDLD() RMB()

//#define PrefetchW(x) (0)
#define PrefetchW(x) __builtin_prefetch(x, 1, 3);

#define RDTICK_X86_32()							\
	({ unsigned long long __t; __asm__ __volatile__ ("rdtsc" : "=A" (__t)); __t; })

#define RDTICK_X86_64()							\
	({ unsigned long long __t, __tl, __th; __asm__ __volatile__ ("rdtsc" : "=a" (__tl), "=d" (__th)); __t=(__th<<32)|__tl; __t; })

#define RDTICK_CELL 1

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int result=0;
  unsigned long int upper, lower,tmp;
  __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0           \n"
                "\tmftb    %1           \n"
                "\tmftbu   %2           \n"
                "\tcmpw    %2,%0        \n"
                "\tbne     0b         \n"
                : "=r"(upper),"=r"(lower),"=r"(tmp)
                );
  result = upper;
  result = result<<32;
  result = result|lower;

  return(result);
 }

typedef uint8_t _u8;
typedef uint16_t _u16;
typedef uint32_t _u32;
typedef uint64_t _u64;

#endif /* X86_SYNC_DEFNS_H_ */
