#include "queue.h"

/* generic queues */

int
dequeue(struct queue *queue, void **dest)
{
	int		locked;
	int		retval;
	struct qnode	*temp;

	locked = 0;
	retval = 0;

	if (dest == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	if (queue == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	/* synchronize */

	if (queue->synchronized
			&& pthread_mutex_lock(&queue->mutex)) {
		retval = -errno;
		goto bubble;
	}
	locked = ~0;

	/* dequeue */

	if (queue->head == NULL) {	/* distrust size */
		retval = ENOENT;
		goto bubble;
	}
	temp = queue->head;

	if (temp == NULL) {
		retval = -EINVAL;
		goto bubble;
	}
	*dest = temp->value;

	if (queue->head == queue->tail) {	/* distrust size */
		queue->head = NULL;
		queue->tail = NULL;
	} else {
		queue->head = temp->next;
	}
	free(temp);
	queue->size--;
bubble:
	/* synchronize */

	if (locked
			&& pthread_mutex_unlock(&queue->mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

int
enqueue(struct queue *queue, void *value)
{
	int		locked;
	struct qnode	*node;
	int		retval;

	locked = 0;
	node = NULL;
	retval = 0;

	if (queue == NULL) {
		retval = -EFAULT;
		goto bubble;
	}

	/* synchronize */

	if (queue->synchronized
			&& pthread_mutex_lock(&queue->mutex)) {
		retval = -errno;
		goto bubble;
	}
	locked = ~0;

	if (queue->capacity >= 0
			&& queue->capacity <= queue->size) {
		retval = ENOBUFS;
		goto bubble;
	}

	/* allocate */

	node = (struct qnode *) calloc(1, sizeof(struct qnode));

	if (node == NULL) {
		retval = -ENOMEM;
		goto bubble;
	}
	*node = (struct qnode) {
		.next = NULL,
		.value = value
	};

	/* enqueue */

	queue->size++;

	if (queue->tail == NULL) {
		queue->head = node;
	} else {
		queue->tail->next = node;
	}
	queue->tail = node;
bubble:
	if (retval
			&& node != NULL) {
		free(node);
	}

	/* synchronize */

	if (locked
			&& pthread_mutex_unlock(&queue->mutex)
			&& !retval) {
		retval = -errno;
	}
	return retval;
}

int
queue_fini(struct queue *queue, void (*vfini)(void *))
{
	struct qnode	*temp;

	if (queue == NULL) {
		return -EFAULT;
	}

	/* synchronize */

	if (queue->synchronized
			&& pthread_mutex_lock(&queue->mutex)) {
		return -errno;
	}

	/* flush */

	while (queue->head != NULL) {
		temp = queue->head;
		queue->head = temp->next;

		if (vfini != NULL) {
			(*vfini)(temp->value);
		}
		free(temp);
	}

	queue->capacity = 0;	/* prevents further enqueue attempts */

	/* synchronize */

	if (queue->synchronized) {
		if (pthread_mutex_unlock(&queue->mutex)) {
			return -errno;
		}

		/* destroy mutex */

		if (pthread_mutex_destroy(&queue->mutex)) {
			return -errno;
		}

		if (pthread_mutexattr_destroy(&queue->mutexattr)) {
			return -errno;
		}
	}
	return 0;
}

int
queue_init(struct queue *dest, ssize_t capacity, int synchronized)
{
	int	initedmutexattr;
	int	retval;

	initedmutexattr = 0;
	retval = 0;

	if (dest == NULL) {
		retval = -EFAULT;
		goto bubble;
	}
	*dest = (struct queue) {
		.capacity = capacity,
		.head = NULL,
		.size = 0,
		.synchronized = synchronized,
		.tail = NULL
	};

	if (synchronized) {
		/* initialize mutex */

		if (pthread_mutexattr_init(&dest->mutexattr)) {
			retval = -errno;
			goto bubble;
		}
		initedmutexattr = ~0;

		if (pthread_mutexattr_settype(&dest->mutexattr,
				PTHREAD_MUTEX_RECURSIVE)) {
			retval = -errno;
			goto bubble;
		}

		if (pthread_mutex_init(&dest->mutex, &dest->mutexattr)) {
			retval = -errno;
			goto bubble;
		}
	}
bubble:
	if (retval
			&& initedmutexattr) {
		pthread_mutexattr_destroy(&dest->mutexattr);
	}
	return retval;
}

