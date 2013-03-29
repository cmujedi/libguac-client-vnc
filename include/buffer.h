#include <pthread.h>
#include <guacamole/client.h>
#include "queue.h"

typedef struct {
    queue data_queue;
    pthread_mutex_t update_lock;
    pthread_cond_t cond;
} buffer;

void init_buffer(buffer* buf, int size_of_data);

void buffer_insert(buffer* buf, void* data, int size_of_data);
 
void buffer_remove(buffer* buf, void* data, int size_of_data, guac_client* client);