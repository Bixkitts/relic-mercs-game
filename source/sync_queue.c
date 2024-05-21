#include <stdatomic.h>
#include <pthread.h>
#include <string.h>

#include "sync_queue.h"
#include "host_custom_attributes.h"

void enqueue(struct SyncQueue *queue,
             char *data,
             ssize_t packetSize,
             Host remotehost)
{
    while (1) {
        int tail = atomic_load(&queue->tail);
        int next_tail = (tail + 1) % MAX_SYNC_QUEUE_SIZE;
        if (next_tail != atomic_load(&queue->head)) {
            // The queue is not full, we can enqueue
            if (atomic_compare_exchange_weak(&queue->tail, &tail, next_tail)) {
                struct HostCustomAttributes *attr = NULL;
                struct QueueParams *par = &queue->params[tail];
                memcpy(par->data,
                       data,
                       sizeof(par->data));
                par->dataSize   = packetSize;
                par->remotehost = remotehost;
                attr = getHostCustomAttr(remotehost);
                par->handler = attr->handler;
                return;
            }
        } else {
            // The queue is full, wait
            // You may use pthread_cond_wait here for more efficient waiting
            sched_yield();
        }
    }
}

struct QueueParams *dequeue(struct SyncQueue *queue)
{
    while (1) {
        int head = atomic_load(&queue->head);
        int tail = atomic_load(&queue->tail);
        if (head != tail) {
            // The queue is not empty, we can dequeue
            if (atomic_compare_exchange_weak(&queue->head,
                                             &head,
                                             (head + 1) % MAX_SYNC_QUEUE_SIZE)) {
                // Successfully reserved the head, dequeue the item
                return &queue->params[head];
            }
        } else {
            // The queue is empty, wait
            // You may use pthread_cond_wait here for more efficient waiting
            sched_yield(); // Simple yield
        }
    }
}
