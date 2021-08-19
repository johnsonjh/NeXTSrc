/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  It
 * is padded so that new data can take the place of storage occupied by part
 * of it.
 */
int msg_send_timeout = 100;	/* milliseconds */
int msg_receive_timeout = 10;	/* milliseconds */
int mutex_spin_limit = 0;
int cthread_stack_mask = 0;
extern void cthread_exit();
int (*_cthread_exit_routine)() = (int (*)()) cthread_exit;
extern void cthread_init();
int (*_cthread_init_routine)() = (int (*)()) cthread_init;
unsigned int cproc_default_stack_size = 1000000;
int condition_spin_limit = 0;
int condition_yield_limit = 7;
unsigned int initial_stack_boundary = 0;
unsigned int cthread_stack_base = 0;	/* Base for stack allocation */
int	malloc_lock = 0;			/* 
					 * Needs to be shared between malloc.o
					 * and malloc_utils.o
					 */

/* global data padding, must NOT be static */
char _threads_data_padding[208] = { 0 };
