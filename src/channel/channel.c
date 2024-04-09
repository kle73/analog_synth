#include <string.h>
#include "channel.h"


err_t channel_create(size_t channel_size, size_t elem_size, channel_t* chn){
	if (chn == NULL) return err_null;
	chn->buf = malloc(channel_size*elem_size);
	if (chn->buf == NULL) return err_null;
	chn->tx = 0; chn->rx = 0;
	chn->size = channel_size;
	chn->elem_size = elem_size;
	pthread_mutex_init(&(chn->lock), NULL);
	return err_ok;
}


/* assumes that there is exacly chn->elem_size to be copied
 * from data to the channel
 */
err_t channel_send(void* data, channel_t* chn){
	if (data == NULL || chn == NULL) return err_null;
 	// Bad -> always leaves one slot unused
	pthread_mutex_lock(&(chn->lock));
	if (chn->tx  == (chn->rx - 1) || (chn->tx == chn->size -1 && chn->rx == 0)){
		pthread_mutex_unlock(&(chn->lock));
		return err_full;
	}
	memcpy(chn->buf + (chn->tx*chn->elem_size), data, chn->elem_size);
	if (chn->tx < chn->size -1) chn->tx++;
	else chn->tx = 0; 
	pthread_mutex_unlock(&(chn->lock));
	return err_ok;
}


err_t channel_rcv(void* buf, channel_t* chn){
	if (buf == NULL || chn == NULL) return err_null;
	pthread_mutex_lock(&(chn->lock));
	if (chn->tx == chn->rx) {
		pthread_mutex_unlock(&(chn->lock));
		return err_empty;
	}
	memcpy(buf, chn->buf + (chn->rx*chn->elem_size), chn->elem_size);
	if (chn->rx < chn->size -1) chn->rx++;
	else chn->rx = 0;
	pthread_mutex_unlock(&(chn->lock));
	return err_ok;
}


void channel_free(channel_t *chn){
	if (chn == NULL) return;
	if (chn->buf == NULL) return;
	free(chn->buf);
}
