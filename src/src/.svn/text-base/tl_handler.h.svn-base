#ifndef _TL_HANDLER_H_
#define _TL_HANDLER_H_

//#define PRE_ALLOC_HANDLERS 1

typedef struct _Thread THREAD;

typedef void (*_ctl_handler) (THREAD *, void *);
typedef int (*_ctl_prepare_handler) (THREAD *, void *);
typedef void (*_ctl_commit_handler) (THREAD *, void *, int);

typedef struct _HandlersList ThreadHandlers;

//struct _Thread {
//  ThreadHandlers *ths;
//};

//#define GET_HANDLERS(_s) (_s)->ths

extern void _ctl_register_pre_start_handler_priority(THREAD * Self,
						     _ctl_handler handler,
						     void *args, int priority);
extern void _ctl_register_pre_start_handler(THREAD * Self, _ctl_handler handler,
					    void *args);

extern void _ctl_register_pos_start_handler_priority(THREAD * Self,
						     _ctl_handler handler,
						     void *args, int priority);
extern void _ctl_register_pos_start_handler(THREAD * Self, _ctl_handler handler,
					    void *args);

extern void _ctl_register_prepare_handler_priority(THREAD * Self,
						   _ctl_prepare_handler handler,
						   void *args, int priority);
extern void _ctl_register_prepare_handler(THREAD * Self,
					  _ctl_prepare_handler handler,
					  void *args);

extern void _ctl_register_commit_handler_priority(THREAD * Self,
						  _ctl_commit_handler handler,
						  void *args, int priority);
extern void _ctl_register_commit_handler(THREAD * Self,
					 _ctl_commit_handler handler, 
					 void *args);

extern void _ctl_register_pos_commit_handler_priority(THREAD * Self,
						      _ctl_handler handler,
						      void *args, int priority);
extern void _ctl_register_pos_commit_handler(THREAD * Self, 
					     _ctl_handler handler,
					     void *args);

extern void _ctl_register_pre_abort_handler_priority(THREAD * Self,
						     _ctl_handler handler,
						     void *args, int priority);
extern void _ctl_register_pre_abort_handler(THREAD * Self, 
					    _ctl_handler handler,
					    void *args);

extern void _ctl_register_pos_abort_handler_priority(THREAD * Self,
						     _ctl_handler handler,
						     void *args, int priority);
extern void _ctl_register_pos_abort_handler(THREAD * Self, _ctl_handler handler,
					    void *args);

extern void _ctl_call_pre_start_handlers(THREAD * Self);

#endif /* TL_HANDLER_H_ */
