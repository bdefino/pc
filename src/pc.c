#include "pc.h"

/* thread-slaving producer/consumer model */

static int
consumertask_vfini(struct consumertask *task);

/* consumption slave */
static int
consumer(struct consumer *consumer)
{
	struct consumertask	*task;
	int		retval;

	retval = 0;

	if (consumer == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	/* greedily consume */

	while (consumer->alive) {
		/* dequeue */

		retval = dequeue(&consumer->queue, (void **) &task);

		if (retval == ENOENT) {
			retval = 0;	/* doesn't signify failure */

			if (consumer->ephemeral) {
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
			consumertask_vfini(task);	/* won't fail */
			continue;
		}

		/* consume */

		if (!(*task->consume)(task)) {
			consumertask_vfini(task);	/* won't fail */
			continue;
		}

		/* requeue */

		retval = enqueue(&consumer->queue, task);

		if (retval) {
			consumertask_vfini(task);	/* won't fail */
		}
	}
bubble:
	/* decrement slave count */

	if (consumer == NULL) {
		return retval;
	}

	if (!consumer->maxslaves) {
		return retval;
	}

	if (!consumer->queue.synchronized) {
		consumer->slaves--;
		return retval;
	}

	/* synchronize */

	if (pthread_mutex_lock(&consumer->queue.mutex)) {
		return retval ? retval : -errno;
	}
	consumer->slaves--;

	/* synchronize */

	if (pthread_mutex_unlock(&consumer->queue.mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

int
consumer_fini(struct consumer *consumer)
{
	if (consumer == NULL) {
		return -EFAULT;
	}
	consumer_kill(consumer);	/* won't fail */
	return queue_fini(&consumer->queue,
		(void (*)(void *)) &consumertask_vfini);
}

int
consumer_init(struct consumer *dest, ssize_t capacity, int ephemeral,
	ssize_t maxslaves)
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
consumer_kill(struct consumer *consumer)
{
	if (consumer == NULL) {
		return -EFAULT;
	}
	consumer->alive = 0;
	return 0;
}

static int
consumertask_vfini(struct consumertask *task)
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
produce(struct consumer *consumer, int (*consume)(struct consumertask *),
	void *data, void (*dfini)(void *))
{
	int		locked;
	int		retval;
	struct consumertask	*task;
	pthread_t	thread;

	locked = 0;
	retval = 0;
	task = NULL;

	if (consumer == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	if (!consumer->queue.synchronized) {
		retval = -EINVAL;
		goto bubble;
	}

	/* allocate */

	task = (struct consumertask *) calloc(1, sizeof(struct consumertask));

	if (task == NULL) {
		retval = -ENOMEM;
		goto bubble;
	}
	task->consume = consume;
	task->data = data;
	task->dfini = dfini;

	/* synchronize */

	if (consumer->queue.synchronized) {
		if (pthread_mutex_lock(&consumer->queue.mutex)) {
			retval = -errno;
			goto bubble;
		}
		locked = ~0;
	}

	/* enqueue */

	retval = enqueue(&consumer->queue, task);

	if (retval) {
		goto bubble;
	}

	/* spin up a slave (as needed) */

	if (!consumer->maxslaves) {
		/* single-threaded */

		retval = consumer(consumer);
		goto bubble;
	} else if (consumer->maxslaves > 0
			&& consumer->maxslaves <= consumer->slaves) {
		/* at capacity */

		goto bubble;
	}

	if (pthread_create(&thread, NULL, &pthread_consumer_compat, consumer)) {
		retval = -errno;
		goto bubble;
	}

	if (pthread_detach(thread)) {
		retval = -errno;
		pthread_join(thread, NULL);	/* ignore failure */
		goto bubble;
	}
	consumer->slaves++;
bubble:
	if (retval
			&& task != NULL) {
		free(task);
	}

	/* synchronize */

	if (locked
			&& pthread_mutex_unlock(&consumer->queue.mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

static void	*
pthread_consumer_compat(void *arg)
{
	consumer((struct consumer *) arg);
	return NULL;
}

