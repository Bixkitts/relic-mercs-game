#include <stdatomic.h>
#include <pthread.h>

#include "sync_queue.h"

void enqueue(struct SyncQueue *queue, int item) {
    while (1) {
        int tail = atomic_load(&queue->tail);
        int next_tail = (tail + 1) % MAX_SYNC_QUEUE_SIZE;
        if (next_tail != atomic_load(&queue->head)) {
            // The queue is not full, we can enqueue
            if (atomic_compare_exchange_weak(&queue->tail, &tail, next_tail)) {
                // Successfully reserved the tail, enqueue the item
                queue->buffer[tail] = item;
                return;
            }
        } else {
            // The queue is full, wait
            // You may use pthread_cond_wait here for more efficient waiting
            sched_yield(); // Simple yield
        }
    }
}

int dequeue(struct SyncQueue *queue) {
    while (1) {
        int head = atomic_load(&queue->head);
        int tail = atomic_load(&queue->tail);
        if (head != tail) {
            // The queue is not empty, we can dequeue
            if (atomic_compare_exchange_weak(&queue->head,
                                             &head,
                                             (head + 1) % MAX_SYNC_QUEUE_SIZE)) {
                // Successfully reserved the head, dequeue the item
                int item = queue->buffer[head];
                return item;
            }
        } else {
            // The queue is empty, wait
            // You may use pthread_cond_wait here for more efficient waiting
            sched_yield(); // Simple yield
        }
    }
}
