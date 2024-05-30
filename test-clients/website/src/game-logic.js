import { getGLContext } from '../canvas-getter.js';
import { loadTexture } from './resource-loading.js';

// Where we should position the player
// models so they align with the map plane
// on Z axis
const playerZHeight = 0.0125;

// We get this from the server and use it to know
// who's turn it currently is
let currentTurn = 0;
let myNetID     = 0;

export class Player {
    constructor(netID, x, y, vigour, violence, cunning, image) {
        this.netID = netID;
        this.position = vec3.fromValues(x, y, playerZHeight);
        this.vigour   = vigour;
        this.violence = violence;
        this.cunning  = cunning;
        this.image    = loadTexture(getGLContext(), image);
    }
    move(targetX, targetY) {
        this.position[0] = targetX;
        this.position[1] = targetY;
    }
}

// Map to store players by netID
const players = new Map();

// Function to add a player
export function addPlayer(netID, x, y, vigour, violence, cunning, image) {
    if (!players.has(netID)) {
        const player = new Player(netID, x, y, vigour, violence, cunning, image);
        players.set(netID, player);
    }
}

// Function to get a player by netID
export function getPlayer(netID) {
    return players.get(netID);
}

// Function to remove a player by netID
export function removePlayer(netID) {
    players.delete(netID);
}

// Function to get all players (as an array)
export function getAllPlayers() {
    return Array.from(players.values());
}

export function setCurrentTurn(netID) {
    currentTurn = netID;
}

export function setMyNetID(netID) {
    myNetID = netID;
}

export function getMyNetID() {
    return myNetID;
}
