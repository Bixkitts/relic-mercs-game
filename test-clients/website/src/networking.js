import * as GameLogic from './game-logic.js';

const _scriptUrl     = new URL(window.location.href);
const _websocketUrl  = 'wss://' + _scriptUrl.hostname + ':' + _scriptUrl.port;
const _opcodeSize    = 2;
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
    const playerId = dataView.getInt16(_opcodeSize, true);
    const xCoord   = dataView.getFloat64(4, true);
    const yCoord   = dataView.getFloat64(12, true);

    const movePlayerResponse = {
        playerNetID: playerId,
        coords: {
            xCoord: xCoord,
            yCoord: yCoord
        }
    };
    const player = GameLogic.getPlayer(playerId);
    GameLogic.movePlayer(player, xCoord, yCoord);

    console.log('Received movePlayerResponse: ', movePlayerResponse);
}

function sendPlayerConnect() {
    const ab       = new ArrayBuffer(3);
    const dataView = new DataView(ab);

    // opcode
    dataView.setInt16   (0, 2, true);
    // Placeholder data, does nothing
    dataView.setInt8    (2, 0, true);

    _socket.send(ab);
}

// The C struct
// -----------------------------------
// #define MAX_PLAYERS_IN_GAME 8
// #define MAX_CREDENTIAL_LEN  32
//
// typedef uint16_t player_id_t;
//
// struct player_connect_res {
//    player_id_t        players       [MAX_PLAYERS_IN_GAME];
//    char               player_names  [MAX_CREDENTIAL_LEN][MAX_PLAYERS_IN_GAME];
//    struct coordinates player_coords [MAX_PLAYERS_IN_GAME];
//    player_id_t        current_turn; // NetID of the player who's turn it is
//    player_id_t        player_index; // Index in the "players" array of the connecting player
//    bool               game_ongoing; // Has the game we've joined started yet?
//}__attribute__((packed));
function handlePlayerConnectResponse(dataView) {
    const maxPlayers         = 8;
    const playerIdSize       = 2;
    const doubleSize         = 8;
    const opcodeSize         = 2;
    const maxCredentialLen   = 32;
    const playerIdOffset     = opcodeSize;
    const nameOffset         = playerIdOffset + (playerIdSize * maxPlayers);
    const coordOffset        = nameOffset + (maxCredentialLen * maxPlayers);

    let playerList = [];

    for (let i = 0; i < maxPlayers; i++) {
        // Extract player_id
        const playerId = dataView.getInt16(playerIdOffset + (i * playerIdSize), true);
        playerList.push(playerId);

        // Extract player name
        const playerNameBytes = new Uint8Array(dataView.buffer,
                                               nameOffset + (maxCredentialLen * i),
                                               maxCredentialLen);
        const playerName      = new TextDecoder().decode(playerNameBytes).trim();

        // Extract player coordinates
        const coordStartIndex = coordOffset + (i * 3 * doubleSize);
        const x = dataView.getFloat64(coordStartIndex, true);
        const y = dataView.getFloat64(coordStartIndex + doubleSize, true);

        // Add player to the map, if their ID is not already in the map
        GameLogic.addPlayerToGame(playerId, x, y, 1, 2, 3, "playerTest.png", playerName);
    }

    // Extract current turn player_id
    const currentTurnOffset = coordOffset + (3 * doubleSize * maxPlayers);
    const currentTurn = dataView.getInt16(currentTurnOffset, true);
    GameLogic.setCurrentTurn(currentTurn);

    // Extract player index
    const myPlayerIdOffset = currentTurnOffset + playerIdSize;
    if (!_connected) {
        _connected = true;
        const extractedPlayerId = dataView.getInt16(myPlayerIdOffset, true);
        GameLogic.setMyPlayerId(extractedPlayerId);
        console.log('Player ID extracted from packet:', extractedPlayerId);
    }

    const gameOngoingOffset = myPlayerIdOffset + playerIdSize;
    // Extract game ongoing status
    const gameOngoing = Boolean(dataView.getInt8(gameOngoingOffset));

    console.log('My Player Id:', GameLogic.getMyPlayerId());
    console.log('Player Ids:', playerList);
    console.log('Current Turn Player Id:', currentTurn);
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
