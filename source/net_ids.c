#include "net_ids.h"
#include "game_logic.h"

/*
 * Maybe replace this implementation with
 * a resizing version if
 * we ever need more networked objects than this?
 * Remember to lock net_ids before touching it.
 *
 * NOTE: this needs a logical to physical type mapping
 * if we start working with thousands and thousands of
 * dynamic objects. That won't happen to me though :D
 */

/*
 * net_id_t ranges corresponding to object types
 */
struct net_id_range {
    net_id_t max;
    net_id_t min;
};

static const net_id_t net_id_ranges[NET_TYPE_COUNT] = {NULL_NET_ID,
                                                       1,
                                                       MAX_PLAYERS_IN_GAME};
/*
 * Returns the range of net_id_ts
 * a particular NetObjType could
 * possibly have
 */
static inline const struct net_id_range
get_id_range_from_type(enum net_obj_type type)
{
    struct net_id_range range = {0};
    for (int i = 0; i < type; i++) {
        range.min += net_id_ranges[i];
    }
    range.max = net_id_ranges[type] + net_id_ranges[NETID_RANGE_BEGIN];
    return range;
}

/*
 * returns NetObjType from
 * a netID based on ranges
 */
static inline enum net_obj_type get_type_from_net_id(net_id_t id)
{
    for (int i = 0; i < NET_TYPE_COUNT - 1; i++) {
        if (id > net_id_ranges[i] && id < net_id_ranges[i + 1]) {
            return net_id_ranges[i + 1];
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
enum net_obj_type resolve_net_id_to_obj(const net_id_t net_id,
                                        struct game *game,
                                        void **ret)
{
    if (net_id >= NETIDS_MAX) {
        return NET_TYPE_NULL;
    }
    *ret = game->net_ids[net_id].object;
    return get_type_from_net_id(net_id);
}
/*
 * Returns the usable net_id_t it
 * finds or -1 on failure
 */
net_id_t create_net_id(enum net_obj_type type, struct game *game, void *obj)
{
    struct net_id_range range = get_id_range_from_type(type);
    int i                     = range.min;
    while (game->net_ids[i].object != NULL) {
        i++;
        if (i >= range.max) {
            return -1;
        }
    }
    game->net_ids[i].object = obj;
    return i;
}

/*
 * Expects a net_id_t smaller than NETID_MAX
 *
 * Currently just sets the pointer at
 * that net_id_t to NULL.
 * Do this to prevent the object from being
 * resolved from a net_id_t e.g. if it's being deleted.
 */
void clear_net_id(struct game *game, const net_id_t net_id)
{
    game->net_ids[net_id].object = NULL;
}

pthread_mutex_t *get_mutex_from_net_id(struct game *game, const net_id_t net_id)
{
    return &game->net_ids[net_id].mutex;
}
