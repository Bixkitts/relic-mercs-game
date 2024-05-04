#ifndef BB_NET_IDS
#define BB_NET_IDS

// Nothing has a netID of 0,
// so if encountered it's an
// uninitialised/nonexistent object.
#define NULL_NET_ID 0

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

void *resolveNetIDToObj (const NetID netID,
                         enum NetObjType type);
NetID createNetID       (enum NetObjType);
void  clearNetID        (const NetID netID);


#endif
