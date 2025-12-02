import { getGLContext } from '../canvas-getter.js';
import { loadTexture } from './resource-loading.js';
import * as Ui from './ui/ui-utils.js';

// Where we should position the player
// models so they align with the map plane
// on Z axis
const PLAYER_POS_Z = 0.0125;

// We get this from the server and use it to know
// who's turn it currently is
let _currentTurn = 0;
let _myNetID     = 0;

// Map to store players by netID
const _players = new Map();


export function movePlayer(player, x, y)
{
    player.position[0] = x;
    player.position[1] = y;
}

function makePlayer(netId, x, y, vigour, violence, cunning, image, name)
{
    return {
        netId,
        position: vec3.fromValues(x, y, PLAYER_POS_Z),
        vigour,
        violence,
        cunning,
        image,
        name
    };
}

export function addPlayerToGame(netID, x, y, vigour, violence, cunning, image, name) {
    const invalidNetID = -1;
    if (!_players.has(netID) && netID != invalidNetID) {
        Ui.greetPlayer(name);
        Ui.doUiTest();
        const player = new makePlayer(netID, x, y, vigour, violence, cunning, image, name);
        _players.set(netID, player);
    }
}

export function getPlayer(netID) {
    console.log("Current PLayers:" + _players);
    return _players.get(netID);
}

export function removePlayer(netID) {
    _players.delete(netID);
}

export function getAllPlayers() {
    return Array.from(_players.values());
}

export function setCurrentTurn(netID) {
    _currentTurn = netID;
}

export function setMyPlayerId(netID) {
    _myNetID = netID;
}

export function getMyPlayerId() {
    console.log("My NetID: " + _myNetID);
    return _myNetID;
}
