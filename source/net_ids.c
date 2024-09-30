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

/*
 * NetID ranges corresponding to object types
 */
struct NetIDRange {
    NetID max;
    NetID min;
};

static const NetID netIDRanges[NET_TYPE_COUNT] = {NULL_NET_ID,
                                                  1,
                                                  MAX_PLAYERS_IN_GAME};
/*
 * Returns the range of NetIDs
 * a particular NetObjType could
 * possibly have
 */
static inline const struct NetIDRange getIDRangeFromType(enum NetObjType type)
{
    struct NetIDRange range = {0};
    for (int i = 0; i < type; i++) {
        range.min += netIDRanges[i];
    }
    range.max = netIDRanges[type] + netIDRanges[NETID_RANGE_BEGIN];
    return range;
}

/*
 * returns NetObjType from
 * a netID based on ranges
 */
static inline enum NetObjType getTypeFromNetID(NetID id)
{
    for (int i = 0; i < NET_TYPE_COUNT - 1; i++) {
        if (id > netIDRanges[i] && id < netIDRanges[i + 1]) {
            return netIDRanges[i + 1];
        }
    }
    return NULL_NET_ID;
}

/*
 * Returns an object of the corresponding NetObjType,
 * or NULL if:
 * - The netID is invalid/out of range
 * - A valid netID resolves to an object of the wrong type
 */
enum NetObjType resolveNetIDToObj(const NetID netID,
                                  struct Game *game,
                                  void **ret)
{
    if (netID >= NETIDS_MAX) {
        return NET_TYPE_NULL;
    }
    *ret = game->netIDs[netID].object;
    return getTypeFromNetID(netID);
}
/*
 * Returns the usable NetID it
 * finds or -1 on failure
 */
NetID createNetID(enum NetObjType type, struct Game *game, void *obj)
{
    struct NetIDRange range = getIDRangeFromType(type);
    int i                   = range.min;
    while (game->netIDs[i].object != NULL) {
        i++;
        if (i >= range.max) {
            return -1;
        }
    }
    game->netIDs[i].object = obj;
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
void clearNetID(struct Game *game, const NetID netID)
{
    game->netIDs[netID].object = NULL;
}

pthread_mutex_t *getMutexFromNetID(struct Game *game, const NetID netID)
{
    return &game->netIDs[netID].mutex;
}
