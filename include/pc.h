#ifndef PC_H
#define PC_H
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#include "queue.h"

/* thread-slaving producer/consumer model */

/* producer/consumer state */
struct pc {
	sig_atomic_t	alive;
	int		ephemeral;	/*
					whether slaves exist only when there
					are incomplete tasks
					*/
	ssize_t		maxslaves;
	struct queue	queue;
	size_t		slaves;
};

/* task abstraction */
struct pctask {
	/* consume the task, and return whether to re-consume the task */
	int	(*consume)(struct pctask *task);
	void	*data;
	void	(*dfini)(void *data);
};

int
pc_fini(struct pc *pc);

int
pc_init(struct pc *dest, ssize_t capacity, int ephemeral, ssize_t maxslaves);

/* signal slave threads to exit gracefully */
int
pc_kill(struct pc *pc);

/* produce a task for eventual consumption */
int
produce(struct pc *pc, int (*consume)(struct pctask *), void *data,
	void (*dfini)(void *));

#endif

