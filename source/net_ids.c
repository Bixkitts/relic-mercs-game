#include "net_ids.h"
#include "game_logic.h"

/*
 * Maybe replace this implementation with 
 * a resizing version if
 * we ever need more networked objects than this?
 * Remember to lock netIDs before touching it.
 *
 * NOTE: this needs a logical to physical type mapping
 * if we start working with thousands and thousands of
 * dynamic objects. That won't happen to me though :D
 */

#define NETIDS_MAX MAX_NETOBJS
// Lock this before touching netIDs
static pthread_mutex_t   netIDmutex         = PTHREAD_MUTEX_INITIALIZER;
static void             *netIDs[NETIDS_MAX] = { 0 };

/*
 * NetID ranges corresponding to object types
 */
struct NetIDRange {
    NetID max;
    NetID min;
};

static const NetID netIDRanges[NET_TYPE_COUNT] = 
{   
    NULL_NET_ID,
    1,
    MAX_PLAYERS,
    MAX_GAMES
};
/*
 * Returns the range of NetIDs
 * a particular NetObjType could
 * possibly have
 */
static inline const 
struct NetIDRange getObjNetIDRange(enum NetObjType type)
{
    struct NetIDRange range = {0};
    for (int i = 0; i < type; i++) {
        range.min += netIDRanges[i]; 
    }
    range.max = netIDRanges[type]+netIDRanges[NETID_RANGE_BEGIN];
    return range;
}

/*
 * Returns an object of the corresponding NetObjType,
 * or NULL if: 
 * - The netID is invalid/out of range
 * - A valid netID resolves to an object of the wrong type
 */
void *resolveNetIDToObj (const NetID netID,
                         enum NetObjType type)
{
    if (netID >= NETIDS_MAX) {
        return NULL;
    }
    pthread_mutex_lock(&netIDmutex);
    const void *retObj     = netIDs[netID];
    if (retObj == NULL) {
        pthread_mutex_unlock(&netIDmutex);
        return NULL;
    }
    struct NetIDRange range = getObjNetIDRange(type);
    bool  isCorrectType = ((netID < range.max) 
                          && (netID >= range.min));
    void *ret           = (void*)(((unsigned long long)retObj) * isCorrectType);
    pthread_mutex_unlock(&netIDmutex);
    return ret;
}
/*
 * Returns the usable NetID it
 * finds or -1 on failure
 */
NetID createNetID(enum NetObjType type)
{
    pthread_mutex_lock   (&netIDmutex);
    struct NetIDRange range = getObjNetIDRange(type);
    int i = range.min;
    while(netIDs[i] != NULL) {
        i++;
        if (i >= range.max) {
            return -1;
        }
    }
    pthread_mutex_unlock (&netIDmutex);
    return i;
}

/*
 * Expects a NetID smaller than NETID_MAX
 *
 * Currently just sets the pointer at
 * that NetID to NULL.
 * Do this to prevent the object from being
 * resolved from a NetID e.g. if it's being deleted.
 */
void  clearNetID (const NetID netID)
{
    pthread_mutex_lock   (&netIDmutex);
    netIDs[netID] = NULL;
    pthread_mutex_unlock (&netIDmutex);
}
