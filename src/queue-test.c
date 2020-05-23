#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "queue.h"

static void
callback(void *arg)
{
	printf("callback(%p)\n", arg);
}

static void
enqueuer(struct queue *queue)
{
	int	i;
	int	retval;

	for (i = 0; i < 100; i++) {
		retval = enqueue(queue, queue);
		printf("enqueue(%p = &(struct queue){.capacity = %ld, .head ="
			" %p, .size = %ld, .tail = %p}, %p) -> %d\n",
			(void *) &queue, queue->capacity, (void *) queue->head,
			queue->size, (void *) queue->tail, (void *) &queue,
			retval);
	}
}

int
main(int argc, char **argv)
{
	int		i;
	void		*out;
	struct queue	queue;
	int		retval;
	pthread_t	thread;

	retval = queue_init(&queue, -1, ~0);
	printf("queue_init(%p, %ld, %d) -> %d\n", (void *) &queue,
		(ssize_t) -1, ~0, retval);
	
	if (retval) {
		return retval;
	}
	
	if (pthread_create(&thread, NULL, (void *(*)(void *)) &enqueuer,
			&queue)) {
		return -errno;
	}
	sleep(2);

	for (i = 0; i < 101; i++) {
		retval = dequeue(&queue, &out);
		printf("dequeue(%p = &(struct queue){.capacity = %ld, .head ="
			" %p, .size = %ld, .tail = %p}, %p) -> %d (*%p ="
			" %p)\n", (void *) &queue, queue.capacity,
			(void *) queue.head, queue.size, (void *) queue.tail,
			(void *) &out, retval, (void *) &out, (void *) out);
	}
	retval = queue_fini(&queue, &callback);
	printf("queue_fini(%p, %lx) -> %d\n", (void *) &queue,
		(size_t)  &callback, retval);
	return retval;
}

