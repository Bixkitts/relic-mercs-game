/*
 * ===========================
 * validators.h
 * ===========================
 * This file is just a big bunch of static inline
 * helper functions for validating data coming in from
 * the websockets, e.g. making sure a set of coordinates
 * a player wants to move to is within the bounds of
 * the map.
 * These functions will probably all only
 * ever have one entry point each, so they're inlined.
 *
 */

#ifndef BB_GAME_VALIDATORS
#define BB_GAME_VALIDATORS
#include "game_logic.h"
#include "helpers.h"
#include "websocket_handlers.h"

/*
 * Returns true if the coordinates that were
 * passed in are valid, false otherwise.
 * Attempts to correct the coordinates either way
 * and write corrected coords to outData.
 */
static inline bool validate_player_move_coords(const struct player_move_req *in_data,
                                               struct player_move_req *out_data)
{
    const double map_bound_x = 1.6;
    const double map_bound_y = 1.0;

    out_data->x_coord = clamp(in_data->x_coord, map_bound_x * -1, map_bound_x);
    out_data->y_coord = clamp(in_data->y_coord, map_bound_y * -1, map_bound_y);

    // Currently the client can't pass "invalid" move coords, they're just
    // clamped.
    return true;
}

/*
 * We've just gotten a character sheet
 * parsed out of a html form,
 * but we need to make sure it's
 * not fudged somehow by the client.
 */
static inline int validate_new_charsheet(struct character_sheet *sheet)
{
    // All this math is unsigned for a reason
    player_attr_t player_power =
        sheet->vigour + sheet->cunning + sheet->violence;
    if (player_power != 13) {
        return -1;
    }
    if (sheet->background >= PLAYER_BACKGROUND_COUNT || sheet->background < 0) {
        return -1;
    }
    if (sheet->gender >= GENDER_COUNT || sheet->gender < 0) {
        return -1;
    }
    return 0;
}

#endif
