#ifndef PC_H
#define PC_H
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#include "queue.h"

/* thread-slaving producer/consumer model */

/* consumer state */
struct consumer {
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
struct consumertask {
	/* consume the task, and return whether to re-consume the task */
	int	(*consume)(struct consumertask *task);
	void	*data;
	void	(*dfini)(void *data);
};

int
consumer_fini(struct consumer *consumer);

int
consumer_init(struct consumer *dest, ssize_t capacity, int ephemeral,
	ssize_t maxslaves);

/* signal slave threads to exit gracefully */
int
consumer_kill(struct consumer *consumer);

/* produce a task for eventual consumption */
int
produce(struct consumer *consumer, int (*consume)(struct consumertask *),
	void *data, void (*dfini)(void *));

#endif

