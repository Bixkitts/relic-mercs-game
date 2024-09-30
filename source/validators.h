#ifndef BB_GAME_VALIDATORS
#define BB_GAME_VALIDATORS
#include "game_logic.h"
#include "helpers.h"
/*

This file is just a big bunch of static inline
helper functions for validating data coming in from
the websockets, e.g. making sure a set of coordinates
a player wants to move to is within the bounds of
the map.
These functions will probably all only
ever have one entry point each, so they're inlined.

 */

/*
 * Returns true if the coordinates that were
 * passed in are valid, false otherwise.
 * Attempts to correct the coordinates either way
 * and write corrected coords to outData.
 */
static inline bool validatePlayerMoveCoords(const struct MovePlayerReq *inData,
                                            struct MovePlayerReq *outData)
{
    const double mapBound_x = 1.6;
    const double mapBound_y = 1.0;

    outData->xCoord = clamp(inData->xCoord, mapBound_x * -1, mapBound_x);
    outData->yCoord = clamp(inData->yCoord, mapBound_y * -1, mapBound_y);

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
static inline int validateNewCharsheet(struct CharacterSheet *sheet)
{
    // All this math is unsigned for a reason
    PlayerAttr playerPower = sheet->vigour + sheet->cunning + sheet->violence;
    if (playerPower != 13) {
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
