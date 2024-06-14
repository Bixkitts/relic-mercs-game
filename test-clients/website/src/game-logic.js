import { getGLContext } from '../canvas-getter.js';
import { loadTexture } from './resource-loading.js';

// Where we should position the player
// models so they align with the map plane
// on Z axis
const _playerZHeight = 0.0125;

// We get this from the server and use it to know
// who's turn it currently is
let _currentTurn = 0;
let _myNetID     = 0;

export class Player {
    constructor(netID, x, y, vigour, violence, cunning, image, name) {
        this.netID    = netID;
        this.position = vec3.fromValues(x, y, _playerZHeight);
        this.vigour   = vigour;
        this.violence = violence;
        this.cunning  = cunning;
        this.image    = loadTexture(getGLContext(), image);
        this.name     = name;
    }
    move(targetX, targetY) {
        this.position[0] = targetX;
        this.position[1] = targetY;
    }
}

// Map to store players by netID
const players = new Map();

export function tryAddPlayer(netID, x, y, vigour, violence, cunning, image, name) {
    const invalidNetID = 0;
    if (!players.has(netID) && netID != invalidNetID) {
        const player = new Player(netID, x, y, vigour, violence, cunning, image, name);
        players.set(netID, player);
    }
}

export function getPlayer(netID) {
    return players.get(netID);
}

export function removePlayer(netID) {
    players.delete(netID);
}

export function getAllPlayers() {
    return Array.from(players.values());
}

export function setCurrentTurn(netID) {
    _currentTurn = netID;
}

export function setMyNetID(netID) {
    _myNetID = netID;
}

export function getMyNetID() {
    return _myNetID;
}
