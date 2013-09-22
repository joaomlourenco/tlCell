#ifndef LOGGER_H
#define LOGGER_H
#include <config.h>

#ifndef DEBUG_MODE
//#define DEBUG_MODE 0
#error DEBUG_MODE not defined
#endif

// 0 -> all disabled
// 1 -> ASSERT_DEBUG* enabled
// 2 -> ASSERT_DEBUG* and TRACE_DEBUG enabled


// ------------------------------------------------------
#ifdef DEBUG_MODE
#define ASSERT(expr)          log_assert(expr, "")
#define ASSERT_MSG(expr, ...) log_assert(expr, __VA_ARGS__)
#define TRACE(...)            log_msg(__VA_ARGS__)
#define TRACE_IF(cond, ...)   do{ if(cond) log_msg(__VA_ARGS__); }while(0)
#endif

#if DEBUG_MODE >=1
#define ASSERT_DEBUG(expr)          log_assert(expr, "")
#define ASSERT_MSG_DEBUG(expr, ...) log_assert(expr, __VA_ARGS__)
#else
#define ASSERT_DEBUG(expr)
#define ASSERT_MSG_DEBUG(...)
#endif

#if DEBUG_MODE >=2
#define TRACE_IF_DEBUG(cond, ...)   do{ if(cond) log_msg(__VA_ARGS__); }while(0)
#define TRACE_DEBUG(...)            log_msg(__VA_ARGS__)
#else
#define TRACE_IF_DEBUG(cond, ...)
#define TRACE_DEBUG(...)
#endif

#ifndef __STRING
#define __STRING(X) #X
#endif

#define log_assert(expr, ...) \
do { \
	((expr) ? 0 :            \
	(log_assert_fail (__STRING(expr), __FILE__, __LINE__, __VA_ARGS__))); \
} while(0)

// ------------------------------------------
extern void log_msg(const char *format, ...);

// This prints an "Assertion failed" message and aborts
extern void log_assert_fail(const char *assertion, const char *file,
			    unsigned int line, const char *format, ...);


#endif
