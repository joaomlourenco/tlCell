#include <config.h>
#include "trace_log.c"

#ifdef ENABLE_TRACE

#define TraceTxStart(_tid, _st, _rv) {\
	int __res___[] = {(int)(_st), (int)(_rv), 0, 0}; \
	TraceEvent(_tid, _tl_txstart, __res___);}
#define TraceTxCommit(_tid, _st) {\
	int __res___[] = {(int)(_st), 0, 0, 0}; \
	TraceEvent(_tid, _tl_txcommit, __res___);}
#define TraceTxAbort(_tid, _st)  {\
	int __res___[] = {(int)(_st), 0, 0, 0}; \
	TraceEvent(_tid, _tl_txabort, __res___);}
#define TraceTxLoad(_tid, _st, _addr)  {\
	int __res___[] = {(int)(_st), (int)(_addr), 0, 0}; \
	TraceEvent(_tid, _tl_txload, __res___);}
#define TraceTxStore(_tid, _st, _addr, _val)  {\
	int __res___[] = {(int)(_st), (int)(_addr), (int)(_val), 0}; \
	TraceEvent(_tid, _tl_txstore, __res___);}
#define TraceTxSterilize(_tid, _st, _addr)  {\
	int __res___[] = {(int)(_st), (int)(_addr), 0, 0}; \
	TraceEvent(_tid, _tl_txsterilize, __res___);}
#define TraceSIGSEGF(_tid, _st, _addr)  {\
	int __res___[] = {(int)(_st), (int)(_addr), 0, 0}; \
	TraceEvent(_tid, _tl_sigsegf, __res___);}
#define TraceRSCoherent(_tid, _st, _addr, _val)  {\
	int __res___[] = {(int)(_st), (int)(_addr), (int)(_val), 0}; \
	TraceEvent(_tid, _tl_txstart, __res___);}

#else

#define TraceEvent(...)
#define TraceTxStart(...)
#define TraceTxCommit(...)
#define TraceTxAbort(...)
#define TraceTxLoad(...)
#define TraceTxStore(...)
#define TraceTxSterilize(...)
#define TraceSIGSEGF(...)
#define TraceRSCoherent(...)
#define PrintTrace(...)
#define TraceInit(...)


#endif
