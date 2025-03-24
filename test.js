// node text.js

const WebSocket = require('ws');

const socket = new WebSocket("wss://641a-138-117-143-165.ngrok-free.app");

socket.onopen = () => {
    console.log("Connected to WebSocket server");
    sendContinuously();
};

function sendContinuously() {
    if (socket.readyState === WebSocket.OPEN) {
        const timestamp = Date.now();
        const message = JSON.stringify({ type: "test", timestamp });
        socket.send(message);
        console.log("Sent:", message);
    }
    setImmediate(sendContinuously); // Send again immediately
}

socket.onmessage = (event) => {
    const receivedTimestamp = Date.now();
    const data = JSON.parse(event.data);
    console.log("Received:", data);
    console.log(`Latency: ${receivedTimestamp - data.timestamp} ms`);
};

socket.onerror = (error) => {
    console.error("WebSocket Error:", error);
};

socket.onclose = () => {
    console.log("WebSocket connection closed");
};
