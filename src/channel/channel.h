#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>
#include <pthread.h>


#define err_ok 			 0
#define err_null 		-1
#define err_arg 		-2
#define err_full 		-3
#define err_empty   -4

typedef int err_t;

typedef struct {
	size_t tx;
	size_t rx;
	size_t size;
	size_t elem_size;
	void* buf;
	pthread_mutex_t lock;
}channel_t;

err_t channel_create(size_t channel_size, size_t elem_size, channel_t* chn);

err_t channel_send(void* data, channel_t* chn);

err_t channel_rcv(void* buf, channel_t* chn);

void channel_free(channel_t* chn);

#endif
