const scriptUrl     = new URL(window.location.href);
const websocketUrl  = 'wss://' + scriptUrl.hostname + ':' + scriptUrl.port;
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
    console.log('Received: ', msg.byteLength);
}


function sendHeartbeat() {
    console.log('Sending heartbeat...');
    const ab = new ArrayBuffer(2);
    ab[0] = 0b0;
    ab[1] = 0b0;
    socket.send(ab);
}

export function sendMovePacket(coordX, coordY) {
    const ab       = new ArrayBuffer(26);
    const dataView = new DataView(ab);
    const netID    = 69n;

    dataView.setInt16   (0, 1, true);
    dataView.setBigInt64(2, netID, true);
    dataView.setFloat64 (10, coordX, true);
    dataView.setFloat64 (18, coordY, true);

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
