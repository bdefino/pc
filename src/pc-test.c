#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "pc.h"

static int
consume(struct pctask *task)
{
	printf("[thread %lx] consume(%p)\n", (size_t) pthread_self(),
		task);
	return 0;
}

static void
dfini(void *arg)
{
	printf("[thread %lx] dfini(%p)\n", (size_t) pthread_self(), arg);
}

int
main(int argc, char **argv)
{
	unsigned int	i;
	struct pc	pc;
	int		retval;

	retval = pc_init(&pc, -1, ~0, 2);
	printf("[thread %lx] pc_init(%p, %ld, %d, %lx) -> %d\n",
		(size_t) pthread_self(), &pc, (ssize_t) -1, ~0, (ssize_t) 2,
		retval);

	if (retval) {
		return retval;
	}

	for (i = 0; i < 100; i++) {
		retval = produce(&pc, &consume, &pc, &dfini);
		printf("[thread %lx] produce(%p, %p, %p, %p) -> %d\n",
			(size_t) pthread_self(), &pc, &consume, &pc, &dfini,
			retval);
	}
	retval = pc_fini(&pc);
	printf("[thread %lx] pc_fini(%p) -> %d\n", (size_t) pthread_self(),
		&pc, retval);
	return retval;
}

