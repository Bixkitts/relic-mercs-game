const wsUrl = 'wss://localhost:443'; // WebSocket URL
const socket = new WebSocket(wsUrl);

socket.addEventListener('open', function (event) 
{
    console.log('WebSocket connection established');
    // Send a heartbeat message every 3 seconds
    setInterval(sendHeartbeat, 3000);
});

socket.addEventListener('error', function (error) 
{
    console.error('WebSocket error:', error);
});

socket.addEventListener('message', function (event) 
{
    console.log('Received:', event.data);
});

function sendHeartbeat() 
{
    console.log('Sending heartbeat...');
    const byteData = new Uint8Array([0x01]);
    const blob     = new Blob([byteData]);
    socket.send(blob);
}

function sendMessage() 
{
    const messageInput = document.getElementById('messageInput');
    const message = messageInput.value;
    socket.send(message); // Send message to server
    console.log('Message sent:', message);
    messageInput.value = ''; // Clear input field
}
