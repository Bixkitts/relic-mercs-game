const websocketUrl = 'wss://5.147.206.132:443';
const socket = new WebSocket(websocketUrl);

// The code:
// 0x01 TCP keepalive ping
// 0x6D Multicast test

socket.addEventListener('open', () => {
    console.log('WebSocket connection established');
    setInterval(sendHeartbeat, 3000);
});

socket.addEventListener('error', function (error) {
    console.log('WebSocket error:', error);
});

socket.addEventListener('message', function (event) {
    console.log('Received:', event.data);
});

function sendHeartbeat() {
    console.log('Sending heartbeat...');
    const byteData = new Uint8Array([0x01]);
    const blob     = new Blob([byteData]);
    socket.send(blob);
}

function sendMessage() {
    const message = messageInput.value;
    socket.send(message);
    console.log('Message sent:', message);
    messageInput.value = '';
}

const messageInput = document.getElementById('messageInput');
document.getElementById("messageInput").onkeydown = e => { if(e.key === "Enter") sendMessage(); };
document.getElementById("sendMessage").onclick = () => { sendMessage(); };
