#ifndef QUEUE_H
#define QUEUE_H
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>

/* generic queues */

/* queue node */
struct qnode {
	struct qnode	*next;
	void		*value;
};

/* queue */
struct queue {
	ssize_t			capacity;
	struct qnode		*head;
	pthread_mutex_t		mutex;		/* reentrant */
	pthread_mutexattr_t	mutexattr;
	size_t			size;
	int			synchronized;
	struct qnode		*tail;
};

/* dequeue a value from a queue (return `+ENOENT` if empty) */
int
dequeue(struct queue *queue, void **dest);

/* enqueue a value to a queue (return `+ENOBUFS` if unable) */
int
enqueue(struct queue *queue, void *value);

int
queue_fini(struct queue *queue, void (*vfini)(void *));

int
queue_init(struct queue *dest, ssize_t capacity, int synchronized);

#endif

