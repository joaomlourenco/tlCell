#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include "logger.h"

static char *LOG_FILE_NAME = NULL;

// ------------------------------------------------------

// log message to stdout and LOG_FILE_NAME
void log_msg(const char *format, ...)
{
	va_list ap;
	FILE *logfile;

	va_start(ap, format);
	(void)vfprintf(stderr, format, ap);
	va_end(ap);

	if (LOG_FILE_NAME != NULL)
	{
		va_start(ap, format);
		logfile = fopen(LOG_FILE_NAME, "a");
		assert(logfile != NULL);

		(void)vfprintf(logfile, format, ap);

		(void)fclose(logfile);
		va_end(ap);
	}

}

// handle assertion faileds
void log_assert_fail(const char *assertion, const char *file,
		     unsigned int line, const char *format, ...)
{
	va_list ap;
	FILE *logfile;

	(void)fprintf(stderr,
		      "%ld: LOG Assertion FAILED: %s - file %s - line %u\n",
		      (long)getpid(), assertion, file, line);

	va_start(ap, format);
	(void)vfprintf(stderr, format, ap);
	va_end(ap);

	if (LOG_FILE_NAME != NULL)
	{
		va_start(ap, format);
		logfile = fopen(LOG_FILE_NAME, "a");
		assert(logfile != NULL);

		(void)fprintf(logfile,
			      "%ld: LOG Assertion FAILED: %s - file %s - line %u\n",
			      (long)getpid(), assertion, file, line);
		(void)vfprintf(logfile, format, ap);

		(void)fclose(logfile);
		va_end(ap);
	}
	abort();
}
