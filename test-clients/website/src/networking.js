import { getPlayers } from  './game-logic.js';

const scriptUrl     = new URL(window.location.href);
const websocketUrl  = 'wss://' + scriptUrl.hostname + ':' + scriptUrl.port;
const opcodeSize    = 2;
let   socket;

window.onload = function() {
    socket            = new WebSocket(websocketUrl);
    socket.binaryType = "arraybuffer";
    socket.onmessage  = e => handleIncoming(e.data);
    socket.onopen = () => {
        console.log('WebSocket connection established');
        setInterval(sendHeartbeat, 3000);
    }

    socket.onerror = e => console.log('WebSocket error:', e);
}

export function getSocket() {
    return socket;
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
        default:
            console.log('Unknown opcode: ', opcode);
            break;
    }
}

function handleMovePlayerResponse(dataView) {

    const playerNetID = dataView.getBigInt64(opcodeSize, true);

    const xCoord = dataView.getFloat64(10, true);
    const yCoord = dataView.getFloat64(18, true);

    const movePlayerResponse = {
        playerNetID: playerNetID,
        coords: {
            xCoord: xCoord,
            yCoord: yCoord
        }
    };
    // TODO: placeholder, we need to resolve the correct
    // player from the netID
    const players = getPlayers();
    players[0].move(xCoord, yCoord);

    console.log('Received movePlayerResponse: ', movePlayerResponse);
}

function fetchPlayerNetIDs() {
    
}

function sendHeartbeat() {
    console.log('Sending heartbeat...');
    const ab = new ArrayBuffer(2);
    ab[0] = 0b0;
    ab[1] = 0b0;
    socket.send(ab);
}

export function sendMovePacket(coordX, coordY) {
    const ab       = new ArrayBuffer(18);
    const dataView = new DataView(ab);

    dataView.setInt16   (0, 1, true);
    dataView.setFloat64 (2, coordX, true);
    dataView.setFloat64 (10, coordY, true);

    socket.send(ab);
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
