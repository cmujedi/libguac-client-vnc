#include <stdlib.h>
#include <string.h>
#include "buffer.h"

void init_buffer(buffer* buf) {
    init_queue(&(buf->data_queue));
}

void buffer_insert(buffer* buf, void* data) {
    pthread_mutex_lock(&(buf->update_lock));
    enqueue(&(buf->data_queue), data);
    pthread_mutex_unlock(&(buf->update_lock));
    pthread_cond_signal(&(buf->cond));     
}

void buffer_remove(buffer* buf, void* data) {
    while (!(data)) {
        pthread_mutex_lock(&(buf->update_lock));
        if((buf->data_queue).count > 0) {
            void* next = malloc(sizeof(data));
            dequeue(&(buf->data_queue), next);
            memcpy(data, next, sizeof(next));
            pthread_mutex_unlock(&(buf->update_lock));
        } else {
            pthread_cond_wait(&(buf->cond), &(buf->update_lock));
        }
    }
}