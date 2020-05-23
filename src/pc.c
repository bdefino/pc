#include "pc.h"

/* thread-slaving producer/consumer model */

static int
pctask_vfini(struct pctask *task);

/* consumption slave */
static int
consumer(struct pc *pc)
{
	struct pctask	*task;
	int		retval;

	retval = 0;

	if (pc == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	/* greedily consume */

	while (pc->alive) {
		/* dequeue */

		retval = dequeue(&pc->queue, (void **) &task);

		if (retval == ENOENT) {
			retval = 0;	/* doesn't signify failure */

			if (pc->ephemeral) {
				break;
			}
			continue;
		} else if (retval) {
			goto bubble;
		}

		if (task == NULL) {
			continue;
		}

		if (task->consume == NULL) {
			pctask_vfini(task);	/* won't fail */
			continue;
		}

		/* consume */

		if (!(*task->consume)(task)) {
			pctask_vfini(task);	/* won't fail */
			continue;
		}

		/* requeue */

		retval = enqueue(&pc->queue, task);

		if (retval) {
			pctask_vfini(task);	/* won't fail */
		}
	}
bubble:
	/* decrement slave count */

	if (pc == NULL) {
		return retval;
	}

	if (!pc->maxslaves) {
		return retval;
	}

	if (!pc->queue.synchronized) {
		pc->slaves--;
		return retval;
	}

	/* synchronize */

	if (pthread_mutex_lock(&pc->queue.mutex)) {
		return retval ? retval : -errno;
	}
	pc->slaves--;

	/* synchronize */

	if (pthread_mutex_unlock(&pc->queue.mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

int
pc_fini(struct pc *pc)
{
	if (pc == NULL) {
		return -EFAULT;
	}
	pc_kill(pc);	/* won't fail */
	return queue_fini(&pc->queue, (void (*)(void *)) &pctask_vfini);
}

int
pc_init(struct pc *dest, ssize_t capacity, int ephemeral, ssize_t maxslaves)
{
	if (dest == NULL) {
		return -EFAULT;
	}
	dest->alive = ~0;
	dest->ephemeral = ephemeral;
	dest->maxslaves = maxslaves;
	dest->slaves = 0;
	return queue_init(&dest->queue, capacity, ~0);
}

int
pc_kill(struct pc *pc)
{
	if (pc == NULL) {
		return -EFAULT;
	}
	pc->alive = 0;
	return 0;
}

static int
pctask_vfini(struct pctask *task)
{
	if (task == NULL) {
		return -EFAULT;
	}

	if (task->dfini != NULL) {
		(*task->dfini)(task->data);
	}
	free(task);
	return 0;
}

/* compatibility layer between a POSIX thread and a consumer slave */
static void	*
pthread_consumer_compat(void *arg);

int
produce(struct pc *pc, int (*consume)(struct pctask *), void *data,
		void (*dfini)(void *))
{
	int		locked;
	int		retval;
	struct pctask	*task;
	pthread_t	thread;

	locked = 0;
	retval = 0;
	task = NULL;

	if (pc == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	if (!pc->queue.synchronized) {
		retval = -EINVAL;
		goto bubble;
	}

	/* allocate */

	task = (struct pctask *) calloc(1, sizeof(struct pctask));

	if (task == NULL) {
		retval = -ENOMEM;
		goto bubble;
	}
	task->consume = consume;
	task->data = data;
	task->dfini = dfini;

	/* synchronize */

	if (pc->queue.synchronized) {
		if (pthread_mutex_lock(&pc->queue.mutex)) {
			retval = -errno;
			goto bubble;
		}
		locked = ~0;
	}

	/* enqueue */

	retval = enqueue(&pc->queue, task);

	if (retval) {
		goto bubble;
	}

	/* spin up a slave (as needed) */

	if (!pc->maxslaves) {
		/* single-threaded */

		retval = consumer(pc);
		goto bubble;
	} else if (pc->maxslaves > 0
			&& pc->maxslaves <= pc->slaves) {
		/* at capacity */

		goto bubble;
	}

	if (pthread_create(&thread, NULL, &pthread_consumer_compat, pc)) {
		retval = -errno;
		goto bubble;
	}

	if (pthread_detach(thread)) {
		retval = -errno;
		pthread_join(thread, NULL);	/* ignore failure */
		goto bubble;
	}
	pc->slaves++;
bubble:
	if (retval
			&& task != NULL) {
		free(task);
	}

	/* synchronize */

	if (locked
			&& pthread_mutex_unlock(&pc->queue.mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

static void	*
pthread_consumer_compat(void *arg)
{
	consumer((struct pc *) arg);
	return NULL;
}

