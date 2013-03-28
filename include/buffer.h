#include <pthread.h>
#include "queue.h"

typedef struct {
    queue data_queue;
    pthread_mutex_t update_lock;
    pthread_cond_t cond;
} buffer;

void init_buffer(buffer* buf);

void buffer_insert(buffer* buf, void* data);
 
void buffer_remove(buffer* buf, void* data);