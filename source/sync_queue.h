#ifndef BB_SYNC_QUEUE
#define BB_SYNC_QUEUE

#include <stdatomic.h>

#define MAX_SYNC_QUEUE_SIZE 100

// Just initialise this
// to { 0 } on creation
// and start queueing
struct SyncQueue {
    int buffer[MAX_SYNC_QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
};

void enqueue       (struct SyncQueue *queue, int item);
int  dequeue       (struct SyncQueue *queue);

#endif
