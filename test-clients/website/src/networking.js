import { getAllPlayers } from  './game-logic.js';
import { getPlayer } from  './game-logic.js';
import { tryAddPlayer } from  './game-logic.js';
import { getMyNetID } from  './game-logic.js';
import { setMyNetID } from  './game-logic.js';
import { setCurrentTurn } from  './game-logic.js';

const _scriptUrl     = new URL(window.location.href);
const _websocketUrl  = 'wss://' + _scriptUrl.hostname + ':' + _scriptUrl.port;
const _opcodeSize    = 2;
const _int64Size     = 8;
let   _connected     = false; // Have we done an initial connection?
let   _socket;

window.onload = function() {
    _socket            = new WebSocket(_websocketUrl);
    _socket.binaryType = "arraybuffer";
    _socket.onmessage  = e => handleIncoming(e.data);
    _socket.onopen = () => {
        console.log('WebSocket connection established');
        setInterval(sendHeartbeat, 3000);
        sendPlayerConnect();
    }


    _socket.onerror = e => console.log('WebSocket error:', e);
}

export function getSocket() {
    return _socket;
}


/**
 * @param {ArrayBuffer} msg 
 */
function handleIncoming(msg) {
    const dataView = new DataView(msg);

    const opcode = dataView.getInt16(0, true);

    switch (opcode) {
        case 0:
            console.log('Received Ping response!');
            break;
        case 1:
            handleMovePlayerResponse(dataView);
            break;
        case 2:
            handlePlayerConnectResponse(dataView);
            break;
        default:
            console.log('Unknown opcode: ', opcode);
            break;
    }
}

function handleMovePlayerResponse(dataView) {

    const playerNetID = dataView.getBigInt64(_opcodeSize, true);

    const xCoord = dataView.getFloat64(10, true);
    const yCoord = dataView.getFloat64(18, true);

    const movePlayerResponse = {
        playerNetID: playerNetID,
        coords: {
            xCoord: xCoord,
            yCoord: yCoord
        }
    };
    getPlayer(playerNetID).move(xCoord, yCoord);

    console.log('Received movePlayerResponse: ', movePlayerResponse);
}

function sendPlayerConnect() {
    const ab       = new ArrayBuffer(3);
    const dataView = new DataView(ab);

    dataView.setInt16   (0, 2, true);
    // Placeholder data, does nothing
    dataView.setInt8    (2, 0, true);

    _socket.send(ab);
}
// The C struct
// -----------------------------------
// struct PlayerConnectRes {
//    NetID              players      [MAX_PLAYERS_IN_GAME];
//    char               playerNames  [MAX_CREDENTIAL_LEN][MAX_PLAYERS_IN_GAME];
//    struct Coordinates playerCoords [MAX_PLAYERS_IN_GAME];
//    NetID              currentTurn; // NetID of the player who's turn it is
//    bool               gameOngoing; // Has the game we've joined started yet?
//    char               playerIndex; // Index in the "players" array of the connecting player
//}__attribute__((packed));
function handlePlayerConnectResponse(dataView) {
    const maxPlayers         = 8;
    const int64Size          = 8; // Size of int64 (BigInt)
    const netIdSize          = int64Size;
    const doubleSize         = 8; // Size of double
    const opcodeSize         = 2; // Two bytes for the opcode
    const MAX_CREDENTIAL_LEN = 32;

    let playerList   = [];

    const netIdOffset = opcodeSize;
    const nameOffset  = netIdOffset + (netIdSize * maxPlayers);
    const coordOffset = nameOffset + (MAX_CREDENTIAL_LEN * maxPlayers);
    for (let i = 0; i < maxPlayers; i++) {
        // Extract player NetID
        const netID = dataView.getBigInt64(netIdOffset + (i * netIdSize), true);
        playerList.push(netID);

        // Extract player name
        const playerNameBytes = new Uint8Array(dataView.buffer,
                                               nameOffset + (MAX_CREDENTIAL_LEN * i),
                                               MAX_CREDENTIAL_LEN);
        const playerName      = new TextDecoder().decode(playerNameBytes).trim();

        // Extract player coordinates
        const coordStartIndex = coordOffset + (i * 3 * doubleSize);
        const x = dataView.getFloat64(coordStartIndex, true);
        const y = dataView.getFloat64(coordStartIndex + doubleSize, true);

        // Add player to the map (assuming default vigour, violence, cunning, image)
        tryAddPlayer(netID, x, y, 1, 2, 3, "playerTest.png", playerName);
    }

    // Adjust offset after player coordinates
    let offset = coordOffset + (3 * doubleSize * maxPlayers);

    // Extract current turn NetID
    const currentTurn = dataView.getBigInt64(offset, true);
    setCurrentTurn(currentTurn);
    offset += int64Size;

    // Extract game ongoing status
    const gameOngoing = Boolean(dataView.getInt8(offset));
    offset += 1;

    // Extract player index
    if (!_connected) {
        _connected = true;
        const playerIndex = dataView.getInt8(offset);
        setMyNetID(playerList[playerIndex]);
        console.log('Player Index:', playerIndex, '  Offset: ', offset);
    }

    console.log('My NetID:', getMyNetID());
    console.log('Player NetIDs:', playerList);
    console.log('Current Turn NetID:', currentTurn);
    console.log('Is Game Ongoing:', gameOngoing);
}

function sendHeartbeat() {
    console.log('Sending heartbeat...');
    const ab = new ArrayBuffer(2);
    ab[0] = 0b0;
    ab[1] = 0b0;
    _socket.send(ab);
}

export function sendMovePacket(coordX, coordY) {
    const ab       = new ArrayBuffer(18);
    const dataView = new DataView(ab);
    const opcode   = 1; // opcode for moving a player

    dataView.setInt16   (0, opcode, true);
    dataView.setFloat64 (2, coordX, true);
    dataView.setFloat64 (10, coordY, true);

    _socket.send(ab);
}

export function sendArgs() {

}

//function sendMessage() {
//    const message = messageInput.value;
//    socket.send(message);
//    console.log('Message sent:', message);
//    messageInput.value = '';
//}
//
//const messageInput = document.getElementById('messageInput');
