# `pc` - a threaded producer/consumer model
Getting the maximum oomph out of your server's often difficult: not only does
design complexity accelerate, but implementations suffer.  Enter `consumer`.

## Goal
This library aims to ease the pain of parallel task consumption, by providing
an interface into a producer/consumer model.

## Features
- Intuitive model,
- **optional** parallel (threaded) execution,
- need-based thread creation,
- task **re**consumption,
- and **dynamic** task updating.

## Model
### API
Here's an example usage (without error checking):
```
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "pc.h"

...

int
print_pthread_self(struct consumertask *current_task)
{
	printf("[thread %zu] print_pthread_self(%p)\n", pthread_self(),
		current_task);
	return 0;	/* don't reconsume */
}

...

int i;
struct consumer consumer;

consumer_init(&consumer, -1, ~0, 4);		/* up to 4 ephemeral slaves */

for (i = 0; i < 100; i++) {
	produce(&consumer, &print_pthread_self, "some data",
		(void (*)(void *) &print_pthread_self);
}
consumer_fini(&consumer);
```

And that's pretty much it.

