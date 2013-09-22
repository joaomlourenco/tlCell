#include "trace_log.h"
#include <config.h>

#ifdef ENABLE_TRACE

#define TRACE_MAX_THREADS 100

#define MAX_EVENTS 4000




typedef struct _TraceLogEvent
{
	long event_ts;
	int thread_id;
	TraceEventName type;
	union event_arg args;
}
TraceLogEvent;

extern struct event_string TraceEventString[];
extern struct event_printer ArgsPrinter[];

static volatile TraceLogEvent GlobalTraceLog[MAX_EVENTS];
static volatile int lock_var = 0;
static volatile long global_event_clock = 0;
// -----------------------------------------------------------
static inline void spin_lock(int volatile *const lock)
{
	for (;;)
	{
		while (*lock == 1)
		{
			sched_yield();
		}
		if (CASIO(lock, 0, 1) == 0)
		{
			break;
		}
	}
	assert(*lock == 1);

}

static inline int spin_trylock(int volatile *const lock)
{
	if (*lock == 0 && CASIO(lock, 0, 1) == 0)
	{
		assert(*lock == 1);
		return 1;
	}

	return 0;
}


static inline void spin_unlock(int volatile *const lock)
{
	assert(*lock == 1);
	*lock = 0;
}

static inline void CAS_SPIN_SET(uintptr_t * const addr,
				uintptr_t const val)
{
	uintptr_t old_val;
	do
	{
		old_val = *addr;
	} while (old_val != CASPO(addr, old_val, val));
}

static inline long IncClock()
{
	long v;
	for (v = global_event_clock;
	     CASPO(&global_event_clock, v, v + 1) != v;
	     v = global_event_clock)
		;

	assert(v + 1 != 0);
	return v;
}

// --------------------------------------------

// -----------------------------------------------------------


inline void TraceEvent(int thread_id, enum _TraceEventName type,
		       int args[MAX_INT_ARGS])
{
	long my_clock = IncClock();
	long pos = my_clock % MAX_EVENTS;

	GlobalTraceLog[pos].event_ts = my_clock;
	GlobalTraceLog[pos].thread_id = thread_id;
	GlobalTraceLog[pos].type = type;
	int i;
	for (i = 0; i < MAX_INT_ARGS; i++)
	{
		GlobalTraceLog[pos].args.int_arg[i] = args[i];
	}
}

inline void TraceEventStringArg(int thread_id, enum _TraceEventName type,
				char string[MAX_STRING_SIZE])
{
	long my_clock = IncClock();
	long pos = my_clock % MAX_EVENTS;

	GlobalTraceLog[pos].event_ts = my_clock;
	GlobalTraceLog[pos].thread_id = thread_id;
	GlobalTraceLog[pos].type = type;
	int i;
	for (i = 0; i < MAX_STRING_SIZE; i++)
	{
		GlobalTraceLog[pos].args.string[i] = string[i];
	}
}



char ___temp____[2] = { 0 };

int get_event_string_size()
{
	int i = 0;
	while (TraceEventString[i].id != -1)
	{
		i++;
	}
	return i;
}

int get_event_printer_size()
{
	int i = 0;
	while (ArgsPrinter[i].id != -1)
	{
		i++;
	}
	return i;
}

const char *get_event_type_string(int type)
{
	int size = get_event_string_size();
	int i;
	for (i = 0; i < size; i++)
	{
		if (TraceEventString[i].id == type)
			return TraceEventString[i].name;
	}
	___temp____[0] = type + '0';
	return (const char *)___temp____;
}

print_args get_event_printer(int type)
{
	int size = get_event_printer_size();
	int i;
	for (i = 0; i < size; i++)
	{
		if (ArgsPrinter[i].id == type)
			return ArgsPrinter[i].f;
	}
	return print_int_args;
}

static inline void __PrintTrace(FILE * tracelog)
{


	assert(global_event_clock != 0);
	int start_clock = global_event_clock;
	int i;
	for (i = 0; i < MAX_EVENTS; i++)
	{
		int p = (i + start_clock) % MAX_EVENTS;
		if (GlobalTraceLog[p].event_ts >= 0)
		{
			fprintf(tracelog, "%ld\t%d\t%s\t",
				GlobalTraceLog[p].event_ts,
				GlobalTraceLog[p].thread_id,
				get_event_type_string(GlobalTraceLog[p].
						      type));
			get_event_printer(GlobalTraceLog[p].
					  type) (GlobalTraceLog[p].args,
						 tracelog);
			fprintf(tracelog, "\n");
		}
	}
}


void PrintTrace()
{
	char tracelog_file_name[200];
	spin_lock(&lock_var);

	sprintf(tracelog_file_name, "global_trace.log");
	FILE *tracelog = fopen(tracelog_file_name, "w");
	fprintf(tracelog, "GLOBAL TRACELOG:\n");
	__PrintTrace(tracelog);
	fclose(tracelog);

	spin_unlock(&lock_var);

}

void TraceInit()
{
	int i;
	for (i = 0; i < MAX_EVENTS; i++)
	{
		GlobalTraceLog[i].event_ts = -1;
	}
}

void print_int_args(union event_arg args, FILE * out)
{
	fprintf(out, "%d\t%d\t%d\t%d", args.int_arg[0], args.int_arg[1],
		args.int_arg[2], args.int_arg[3]);
}

void print_string_args(union event_arg args, FILE * out)
{
	fprintf(out, "%s", args.string);
}

#endif
