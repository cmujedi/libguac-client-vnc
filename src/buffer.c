#include <stdlib.h>
#include <string.h>
#include "buffer.h"

void init_buffer(buffer* buf, int size_of_data) {
    init_queue(&(buf->data_queue), size_of_data);
}

void buffer_insert(buffer* buf, void* data, int size_of_data) {
    pthread_mutex_lock(&(buf->update_lock));
    enqueue(&(buf->data_queue), data, size_of_data);
    pthread_mutex_unlock(&(buf->update_lock));
    pthread_cond_signal(&(buf->cond));     
}

void buffer_remove(buffer* buf, void* data, int size_of_data, guac_client* client) {

    pthread_mutex_lock(&(buf->update_lock));
    if((buf->data_queue).count <= 0) 
        pthread_cond_wait(&(buf->cond), &(buf->update_lock));

    dequeue(&(buf->data_queue), data, size_of_data);
    pthread_mutex_unlock(&(buf->update_lock));
}