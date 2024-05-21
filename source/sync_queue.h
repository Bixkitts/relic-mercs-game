#ifndef BB_SYNC_QUEUE
#define BB_SYNC_QUEUE

#include <stdatomic.h>
#include <stdio.h>

#include "bbnetlib.h"
#include "packet_handlers.h"

#define MAX_SYNC_QUEUE_SIZE 100

struct QueueParams {
    char         data[MAX_PACKET_SIZE];
    ssize_t      dataSize;
    Host         remotehost;
    enum Handler handler;
};

// Just initialise this
// to { 0 } on creation
// and start queueing
struct SyncQueue {
    struct QueueParams  params[MAX_SYNC_QUEUE_SIZE];
    atomic_int          head;
    atomic_int          tail;
};


void 
enqueue            (struct SyncQueue *queue,
                    char *data,
                    ssize_t packetSize,
                    Host remotehost);
struct QueueParams
*dequeue           (struct SyncQueue *queue);

#endif
