#ifndef TRACELOG_H_
#define TRACELOG_H_

#if defined LIB_COMPILATION && defined HAVE_CONFIG_H
#  include <config.h>
#endif

#include <inttypes.h>

#define MAX_INT_ARGS 4
#define MAX_STRING_SIZE (MAX_INT_ARGS*sizeof(int))


// ----------------------------


typedef enum _TraceEventName
{ _tl_txstart, _tl_txabort, _tl_txcommit, _tl_sigsegf, _tl_rscoherent,
		_tl_txload, _tl_txvfy, _tl_txstore, _tl_txsterilize,
		_tl_LK_ldlock, _tl_LK_stlock, _tl_LK_caslock,
		_tl_LK_ldaddr, _tl_LK_staddr, _TRACE_SEPARATOR = 50,
#ifdef USER_TRACE_EVENTID_HEADER
#include USER_TRACE_EVENTID_HEADER
#endif
} TraceEventName;

struct event_string
{
	int id;
	const char *name;
};

union event_arg
{
	int int_arg[MAX_INT_ARGS];
	char string[MAX_STRING_SIZE];
};

typedef void (*print_args) (union event_arg args, FILE * out);

// this is the default print function
void print_int_args(union event_arg args, FILE * out);
void print_string_args(union event_arg args, FILE * out);

struct event_printer
{
	int id;
	print_args f;
};


#ifdef ENABLE_TRACE

extern inline void TraceEvent(int thread_id, enum _TraceEventName type,
		       int args[MAX_INT_ARGS]);
extern inline void TraceEventStringArg(int thread_id, enum _TraceEventName type,
				char string[MAX_STRING_SIZE]);
extern void PrintTrace();
extern void TraceInit();

#endif

#endif
