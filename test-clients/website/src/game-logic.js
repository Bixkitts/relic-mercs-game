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

function truncateAtNull(string)
{
    const nullIndex = string.indexOf('\0');
    return nullIndex !== -1 ? string.substring(0, nullIndex) : string;
}

export function addPlayerToGame(netID, x, y, vigour, violence, cunning, image, name) {
    const truncatedName = truncateAtNull(name);
    const welcomeMsg = `Welcome ${truncatedName}.`;
    const invalidNetID = -1;
    if (!_players.has(netID) && netID != invalidNetID) {
        const textCoords = [0.3, 0.2 - (0.1 * _players.size)];
        let te   = Ui.makeTextElement(welcomeMsg, textCoords, 0.125);

        const labelTransform = Ui.makeUiTransform(0.25, 0.9, 0.5, 0.54);
        const labelColor  = [0.8, 0.8, 1.0, 0.7];
        let lab = Ui.makeLabel(labelTransform,
                               '<color="#FF0000">Option1:</color> The poor should eat the poor\n'
                               + '         as visciously as possible.\nThis is going to be a longer piece of text...\nIm not sorry.',
                               labelColor,
                               Ui.TextAlignment.Top);

        const buttonTransform = Ui.makeUiTransform(0.3, 0.35, 0.4, 0.04);
        const buttonColor = [0.9, 0.9, 1.0, 1.0];
        let but1 = Ui.makeButton(buttonTransform,
                                 '<color="#FF0000">Option2:</color> Say nothing',
                                 buttonColor,
                                 buttonCallbackTest,
                                 Ui.TextAlignment.Center);

        const player = new makePlayer(netID, x, y, vigour, violence, cunning, image, name);
        _players.set(netID, player);
    }
}

function buttonCallbackTest(button)
{
    console.log("Button was clicked!");
    Ui.hideUiElement(button);
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
