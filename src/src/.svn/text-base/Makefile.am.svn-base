lib_LTLIBRARIES=libctl@RELEASE_SUFFIX@.la
libctl@RELEASE_SUFFIX@_la_SOURCES=tl.c

include_HEADERS=tl.h \
	        trace_log.h \
		tl_handler.h \
		user_trace_log.h \
		x86_sync_defns.h

noinst_HEADERS=gvclock.h \
	       logger.h

# Set the include path found by configure
INCLUDES=$(all_includes)

# Set the library search path.
libctl@RELEASE_SUFFIX@_la_LDFLAGS=$(all_libraries)

clean-local:
	rm -f *~

install-data-hook:
	../scripts/gen_header.sh ../config.h $(includedir)
