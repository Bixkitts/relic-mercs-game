#ifndef BB_NET_IDS
#define BB_NET_IDS

#include <pthread.h>
// Nothing has a netID of 0,
// so if encountered it's an
// uninitialised/nonexistent object.
#define NULL_NET_ID 0
#define NETIDS_MAX  512

/*
 * Types of objects or states
 * that will need to be synced,
 * partially or fully serialised
 * over the network
 */
enum net_obj_type {
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
typedef long long net_id_t;

struct net_id_slot {
    pthread_mutex_t mutex;
    void *object;
};

struct game;

enum net_obj_type resolve_net_id_to_obj(const net_id_t net_id,
                                        struct game *game,
                                        void **ret);
net_id_t create_net_id(enum net_obj_type type, struct game *game, void *obj);
// Assigns the pointer that is passed
// to the netID it finds and returns.

void clear_net_id(struct game *game, const net_id_t net_id);

pthread_mutex_t *get_mutex_from_net_id(struct game *game,
                                       const net_id_t net_id);

#endif
