#include <stdlib.h>
#include <config.h>

#include "tl_handler.h"
#include "tl.h"

/// Defines the union of the three type of handlers.
union _handler
{
	_ctl_handler handler;	///< Normal handler pointer.
	_ctl_prepare_handler prepare_handler;	///< Prepare handler pointer.
	_ctl_commit_handler commit_handler;	///< Commit handler pointer.
};

/// Defines the structure of a handler list node.
struct _handlerNode
{
	union _handler handler;	///< Handler pointer.
	void *handler_args;	///< Handler arguments.
	int priority;	///< Handler node priority.
	struct _handlerNode *next;	///< Pointer to the next handler.
};


/// Defines the structure of the lists of different handlers.
struct _HandlersList
{
	struct _handlerNode *pre_start;	///< List of pre-start handlers.
	struct _handlerNode *pos_start;	///< List of pos-start handlers.
	struct _handlerNode *prepare_commit;	///< List of prepare-commit handlers.
	struct _handlerNode *pre_commit;	///< List of commit handlers.
	struct _handlerNode *pos_commit;	///< List of pos-commit handlers.
	struct _handlerNode *pre_abort;	///< List of pre-abort handlers.
	struct _handlerNode *pos_abort;	///< List of pos-abort handlers.
};



#ifdef PRE_ALLOC_HANDLERS

#define __PRE_NUM_HANDLERS__ 10

static inline void _alloc_handlers(struct _handlerNode **root)
{
	struct _handlerNode *new, *p;
	int i;

	new = calloc(1, sizeof(*new));
	new->handler.handler = NULL;
	new->handler_args = NULL;
	new->next = NULL;
	*root = new;
	p = new;

	for (i = 0; i < __PRE_NUM_HANDLERS__ - 1; i++)
	{
		new = calloc(1, sizeof(*new));
		new->handler.handler = NULL;
		new->handler_args = NULL;
		new->next = NULL;
		p->next = new;
		p = new;
	}
};

#endif

/** Initializes a _HandlersList structure.
 *
 *  @return pointer to _HandlersList structure.
 */
static inline ThreadHandlers *_ctl_new_thread_handlers()
{
	ThreadHandlers *ths = calloc(1, sizeof(*ths));

#ifdef PRE_ALLOC_HANDLERS

	_alloc_handlers(&ths->pre_start);
	_alloc_handlers(&ths->pos_start);
	_alloc_handlers(&ths->prepare_commit);
	_alloc_handlers(&ths->pre_commit);
	_alloc_handlers(&ths->pos_commit);
	_alloc_handlers(&ths->pre_abort);
	_alloc_handlers(&ths->pos_abort);

#endif

	return ths;
}

/** Cleans the respective handler list.
 *
 *  @param l list of handlers of a specified type.
 */
static inline void clean_handler_list(struct _handlerNode *l)
{
	struct _handlerNode *n, *t;
	for (n = l; n != NULL;)
	{
		t = n;
		n = n->next;
#ifndef PRE_ALLOC_HANDLERS
		free(t);
#else
		t->handler.handler = NULL;
		t->handler_args = NULL;
#endif
	}
}

/** Cleans all previous registed handlers from the system.
 *
 *  @param ths pointer to _HandlersList structure.
 */
static inline void _ctl_clean_thread_handlers(ThreadHandlers * ths)
{

	clean_handler_list(ths->pre_start);
#ifndef PRE_ALLOC_HANDLERS
	ths->pre_start = NULL;
#endif

	clean_handler_list(ths->pos_start);
#ifndef PRE_ALLOC_HANDLERS
	ths->pos_start = NULL;
#endif

	clean_handler_list(ths->prepare_commit);
#ifndef PRE_ALLOC_HANDLERS
	ths->prepare_commit = NULL;
#endif

	clean_handler_list(ths->pre_commit);
#ifndef PRE_ALLOC_HANDLERS
	ths->pre_commit = NULL;
#endif

	clean_handler_list(ths->pos_commit);
#ifndef PRE_ALLOC_HANDLERS
	ths->pos_commit = NULL;
#endif

	clean_handler_list(ths->pre_abort);
#ifndef PRE_ALLOC_HANDLERS
	ths->pre_abort = NULL;
#endif

	clean_handler_list(ths->pos_abort);
#ifndef PRE_ALLOC_HANDLERS
	ths->pos_abort = NULL;
#endif
}


#ifdef PRE_ALLOC_HANDLERS

static inline void clean_handler_list_pre_alloc(struct _handlerNode *l)
{
	struct _handlerNode *n, *t;
	for (n = l; n != NULL;)
	{
		t = n;
		n = n->next;

		free(t);
	}
}

static inline void _ctl_clean_thread_handlers_pre_alloc(ThreadHandlers *
							ths)
{

	clean_handler_list_pre_alloc(ths->pre_start);
	ths->pre_start = NULL;

	clean_handler_list_pre_alloc(ths->pos_start);
	ths->pos_start = NULL;

	clean_handler_list_pre_alloc(ths->prepare_commit);
	ths->prepare_commit = NULL;

	clean_handler_list_pre_alloc(ths->pre_commit);
	ths->pre_commit = NULL;

	clean_handler_list_pre_alloc(ths->pos_commit);
	ths->pos_commit = NULL;

	clean_handler_list_pre_alloc(ths->pre_abort);
	ths->pre_abort = NULL;

	clean_handler_list_pre_alloc(ths->pos_abort);
	ths->pos_abort = NULL;
}
#endif

/** Deallocates a _HandlersList structure.
 *
 *  @param ths pointer to _HandlersList structure.
 */
static inline void _ctl_delete_thread_handlers(ThreadHandlers * ths)
{
#ifdef PRE_ALLOC_HANDLERS
	_ctl_clean_thread_handlers_pre_alloc(ths);
#else
	_ctl_clean_thread_handlers(ths);
#endif
	free(ths);
}


static inline void _ctl_register_handler(struct _handlerNode **list,
					 union _handler h, void *args,
					 int priority)
{
	struct _handlerNode *n, *a = NULL;
#ifndef PRE_ALLOC_HANDLERS
	struct _handlerNode *new;
#endif


#ifndef PRE_ALLOC_HANDLERS
	for (n = *list; n != NULL && n->priority >= priority;)
	{
#else
	for (n = *list; n->handler.handler != NULL;)
	{
#endif
		a = n;
		n = n->next;
	}

#ifndef PRE_ALLOC_HANDLERS
	new = calloc(1, sizeof(*new));
	new->handler = h;
	new->handler_args = args;
	new->priority = priority;

	if (a == NULL)
	{
		new->next = *list;
		*list = new;
	} else
	{
		if (n == NULL)
		{
			new->next = NULL;
			a->next = new;
		} else
		{
			new->next = a->next;
			a->next = new;
		}
	}
#else

	n->handler = h;
	n->handler_args = args;
	n->priority = priority;

#endif

}


#define __DEFAULT_PRIORITY__ 10


void _ctl_register_pre_start_handler_priority(THREAD * Self,
					      _ctl_handler handler,
					      void *args, int priority)
{
	union _handler n;
	n.handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pre_start, n, args,
			      priority);
}

void _ctl_register_pre_start_handler(THREAD * Self, _ctl_handler handler,
				     void *args)
{
	_ctl_register_pre_start_handler_priority(Self, handler, args,
						 __DEFAULT_PRIORITY__);
}

void _ctl_register_pos_start_handler_priority(THREAD * Self,
					      _ctl_handler handler,
					      void *args, int priority)
{
	union _handler n;
	n.handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pos_start, n, args,
			      priority);
}

void _ctl_register_pos_start_handler(THREAD * Self, _ctl_handler handler,
				     void *args)
{
	_ctl_register_pos_start_handler_priority(Self, handler, args,
						 __DEFAULT_PRIORITY__);
}

void _ctl_register_prepare_handler_priority(THREAD * Self,
					    _ctl_prepare_handler handler,
					    void *args, int priority)
{
	union _handler n;
	n.prepare_handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->prepare_commit, n, args,
			      priority);
}

void _ctl_register_prepare_handler(THREAD * Self,
				   _ctl_prepare_handler handler,
				   void *args)
{
	_ctl_register_prepare_handler_priority(Self, handler, args,
					       __DEFAULT_PRIORITY__);
}

void _ctl_register_commit_handler_priority(THREAD * Self,
					   _ctl_commit_handler handler,
					   void *args, int priority)
{
	union _handler n;
	n.commit_handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pre_commit, n, args,
			      priority);
}

void _ctl_register_commit_handler(THREAD * Self,
				  _ctl_commit_handler handler, void *args)
{
	_ctl_register_commit_handler_priority(Self, handler, args,
					      __DEFAULT_PRIORITY__);
}


void _ctl_register_pos_commit_handler_priority(THREAD * Self,
					       _ctl_handler handler,
					       void *args, int priority)
{
	union _handler n;
	n.handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pos_commit, n, args,
			      priority);
}

void _ctl_register_pos_commit_handler(THREAD * Self, _ctl_handler handler,
				      void *args)
{
	_ctl_register_pos_commit_handler_priority(Self, handler, args,
						  __DEFAULT_PRIORITY__);
}


void _ctl_register_pre_abort_handler_priority(THREAD * Self,
					      _ctl_handler handler,
					      void *args, int priority)
{
	union _handler n;
	n.handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pre_abort, n, args,
			      priority);
}

void _ctl_register_pre_abort_handler(THREAD * Self, _ctl_handler handler,
				     void *args)
{
	_ctl_register_pre_abort_handler_priority(Self, handler, args,
						 __DEFAULT_PRIORITY__);
}


void _ctl_register_pos_abort_handler_priority(THREAD * Self,
					      _ctl_handler handler,
					      void *args, int priority)
{
	union _handler n;
	n.handler = handler;
	_ctl_register_handler(&GET_HANDLERS(Self)->pos_abort, n, args,
			      priority);
}

void _ctl_register_pos_abort_handler(THREAD * Self, _ctl_handler handler,
				     void *args)
{
	_ctl_register_pos_abort_handler_priority(Self, handler, args,
						 __DEFAULT_PRIORITY__);
}


void _ctl_call_pre_start_handlers(THREAD * Self)
{
	struct _handlerNode *n;
#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pre_start; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pre_start; n->handler.handler != NULL;
	     n = n->next)
	{
#endif
		n->handler.handler(Self, n->handler_args);
	}
}


/** Calls all the registed pos-start handlers.
 *
 *  @param ths pointer to _HandlersList structure.
 *  @param env hashtable environment.
 */
static inline void _ctl_call_pos_start_handlers(THREAD * Self)
{
	struct _handlerNode *n;

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pos_start; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pos_start; n->handler.handler != NULL;
	     n = n->next)
	{
#endif
		n->handler.handler(Self, n->handler_args);
	}
}

/** Calls all the registed prepare handlers and right after
 *  calls the commit handlers in order to perform a Two Phase Commit.
 *
 *  @param ths pointer to _HandlersList structure.
 *  @param env hashtable environment.
 *  @return result of the 2PC (1 => commit, 0 => abort).
 */
static inline int _ctl_call_prepare_commit_handlers(THREAD * Self)
{
	struct _handlerNode *n;

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->prepare_commit; n != NULL;
	     n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->prepare_commit;
	     n->handler.prepare_handler != NULL; n = n->next)
	{
#endif
		if (!(n->handler.prepare_handler(Self, n->handler_args)))
			return 0;
	}

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pre_commit; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pre_commit;
	     n->handler.commit_handler != NULL; n = n->next)
	{
#endif
		n->handler.commit_handler(Self, n->handler_args, 1);
	}

	return 1;
}

/** Calls all the registed pos-commit handlers.
 *
 *  @param ths pointer to _HandlersList structure.
 *  @param env hashtable environment.
 */
static inline void _ctl_call_pos_commit_handlers(THREAD * Self)
{
	struct _handlerNode *n;

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pos_commit; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pos_commit;
	     n->handler.handler != NULL; n = n->next)
	{
#endif
		n->handler.handler(Self, n->handler_args);
	}
}

/** Calls all the registed pre-abort handlers.
 *
 *  @param ths pointer to _HandlersList structure.
 *  @param env hashtable environment.
 */
static inline void _ctl_call_pre_abort_handlers(THREAD * Self)
{
	struct _handlerNode *n;

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pre_abort; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pre_abort; n->handler.handler != NULL;
	     n = n->next)
	{
#endif
		n->handler.handler(Self, n->handler_args);
	}
}

/** Calls all the registed pos-abort handlers.
 *
 *  @param ths pointer to _HandlersList structure.
 *  @param env hashtable environment.
 */
static inline void _ctl_call_pos_abort_handlers(THREAD * Self)
{
	struct _handlerNode *n;

#ifndef PRE_ALLOC_HANDLERS
	for (n = GET_HANDLERS(Self)->pos_abort; n != NULL; n = n->next)
	{
#else
	for (n = GET_HANDLERS(Self)->pos_abort; n->handler.handler != NULL;
	     n = n->next)
	{
#endif
		n->handler.handler(Self, n->handler_args);
	}
}




/** \example tl_handler_example.c
 *  This is an example of how to use the
 *  CTL handler system API.
 */
