const websocketUrl = 'wss://remotehost:7676';
const socket       = new WebSocket(websocketUrl);
socket.binaryType = "arraybuffer";

export function getSocket() {
    return socket;
}

socket.onopen = () => {
    console.log('WebSocket connection established');
    setInterval(sendHeartbeat, 3000);
}

socket.onerror = e => console.log('WebSocket error:', e);

/**
 * @param {ArrayBuffer} msg 
 */
function handleIncoming(msg) {
    console.log('Received: ', msg.byteLength);
}

socket.onmessage = e => handleIncoming(e.data);

function sendHeartbeat() {
    console.log('Sending heartbeat...');
    const ab = new ArrayBuffer(2);
    ab[0] = 0b0;
    ab[1] = 0b0;
    socket.send(ab);
}

export function sendArgs() {

}

function sendMessage() {
    const message = messageInput.value;
    socket.send(message);
    console.log('Message sent:', message);
    messageInput.value = '';
}

const messageInput = document.getElementById('messageInput');
//document.getElementById("messageInput").onkeydown = e => { if(e.key === "Enter") sendMessage(); };
//document.getElementById("sendMessage").onclick = () => { sendMessage(); };
