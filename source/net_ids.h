#ifndef BB_NET_IDS
#define BB_NET_IDS

#include <pthread.h>
// Nothing has a netID of 0,
// so if encountered it's an
// uninitialised/nonexistent object.
#define NULL_NET_ID 0
#define NETIDS_MAX 512

/*
 * Types of objects or states
 * that will need to be synced,
 * partially or fully serialised
 * over the network
 */
enum NetObjType {
    NET_TYPE_NULL,
    NETID_RANGE_BEGIN,
    NET_TYPE_PLAYER,
    NET_TYPE_GAME,
    NET_TYPE_COUNT
};
/*
 * A NetID is a unique numberic identifier for
 * a specific object that can be sent
 * over the network as an integer to refer
 * to the specific object.
 */
typedef long long NetID;

struct netIDList {
    pthread_mutex_t    netIDlock;
    NetID              netIDs  [NETIDS_MAX];
};

void *resolveNetIDToObj (const NetID netID,
                         enum NetObjType type);
// Assigns the pointer that is passed
// to the netID it finds and returns.
NetID createNetID       (enum NetObjType,
                         void *obj);
void  clearNetID        (const NetID netID);


#endif
