import { getGLContext } from '../canvas-getter.js';
import { loadTexture } from './resource-loading.js';
import { buildTextElement,
         buildButton,
         deleteTextElement,
         TextElement} from './ui-utils.js';

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
        this.image    = loadTexture(getGLContext(), image, getGLContext().LINEAR);
        this.name     = name;
    }
    move(targetX, targetY) {
        this.position[0] = targetX;
        this.position[1] = targetY;
    }
}

// Map to store players by netID
const players = new Map();


function truncateAtNull(string)
{
    const nullIndex = string.indexOf('\0');
    return nullIndex !== -1 ? string.substring(0, nullIndex) : string;
}

export function tryAddPlayer(netID, x, y, vigour, violence, cunning, image, name) {
    const truncatedName = truncateAtNull(name);
    const welcomeMsg = `Welcome ${truncatedName}!`;
    const invalidNetID = -1;
    if (!players.has(netID) && netID != invalidNetID) {
        const textCoords = [0.3, 0.2 - (0.1 * players.size)];
        let te = buildTextElement(welcomeMsg, textCoords, 0.25);
        let but2 = buildButton([0.4, 0.4], 0.2, 0.04, "Option1: Eat the poor", buttonCallbackTest);
        let but1 = buildButton([0.4, 0.35], 0.2, 0.04, "Option2: Eat the rich", buttonCallbackTest);
        const player = new Player(netID, x, y, vigour, violence, cunning, image, name);
        players.set(netID, player);
    }
}

function buttonCallbackTest(button)
{
    console.log("Button was clicked!");
    button.hide();
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

export function setMyPlayerId(netID) {
    _myNetID = netID;
}

export function getMyPlayerId() {
    return _myNetID;
}
