#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "pc.h"

static int
consume(struct consumertask *task)
{
	printf("[thread %lx] consume(%p)\n", (size_t) pthread_self(),
		(void *) task);
	return 0;
}

static void
dfini(void *arg)
{
	printf("[thread %lx] dfini(%p)\n", (size_t) pthread_self(),
		(void *) arg);
}

int
main(int argc, char **argv)
{
	unsigned int	i;
	struct consumer	consumer;
	int		retval;

	retval = consumer_init(&consumer, -1, ~0, 2);
	printf("[thread %lx] consumer_init(%p, %ld, %d, %lx) -> %d\n",
		(size_t) pthread_self(), (void *) &consumer, (ssize_t) -1, ~0,
		(ssize_t) 2, retval);

	if (retval) {
		return retval;
	}

	for (i = 0; i < 100; i++) {
		retval = produce(&consumer, &consume, &consumer, &dfini);
		printf("[thread %lx] produce(%p, %lx, %p, %lx) -> %d\n",
			(size_t) pthread_self(), (void *) &consumer,
			(size_t) &consume, (void *) &consumer, (size_t) &dfini,
			retval);
	}
	retval = consumer_fini(&consumer);
	printf("[thread %lx] consumer_fini(%p) -> %d\n", (size_t) pthread_self(),
		(void *) &consumer, retval);
	return retval;
}

