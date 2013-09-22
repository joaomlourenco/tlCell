#ifndef _USER_TRACE_LOG_H_
#define _USER_TRACE_LOG_H_

#include <trace_log.h>

struct event_string TraceEventString[] = {
	{_tl_txstart, (const char *)"_tl_txstart"},
	{_tl_txabort, (const char *)"_tl_txabort"},
	{_tl_txcommit, (const char *)"_tl_txcommit"},
	{_tl_sigsegf, (const char *)"_tl_sigsegf"},
	{_tl_rscoherent, (const char *)"_tl_rscoherent"},
	{_tl_txload, (const char *)"_tl_txload"},
	{_tl_txvfy, (const char *)"_tl_txvfy"},
	{_tl_txstore, (const char *)"_tl_txstore"},
	{_tl_txsterilize, (const char *)"_tl_txsterilize"},
	{_tl_LK_ldlock, (const char *)"_tl_LK_ldlock"},
	{_tl_LK_stlock, (const char *)"_tl_LK_stlock"},
	{_tl_LK_caslock, (const char *)"_tl_LK_caslock"},
	{_tl_LK_ldaddr, (const char *)"_tl_LK_ldaddr"},
	{_tl_LK_staddr, (const char *)"_tl_LK_staddr"},
#ifdef USER_TRACE_EVENTNAME_HEADER
#  include USER_TRACE_EVENTNAME_HEADER
#endif
	, {-1, NULL}
};

struct event_printer ArgsPrinter[] = {
#ifdef USER_TRACE_EVENTPRINTER_HEADER
#  include USER_TRACE_EVENTPRINTER_HEADER
#endif
	{-1, NULL}
};

#endif
