// node ws.js
// ngrok http 8080

const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', function connection(ws) {
  console.log('Client connected');

  ws.on('message', function incoming(message) {
    console.log('received: %s', message);
    // Here you can handle the message from the client, like parsing JSON and doing something with it
  });

  // Send a welcome message once upon connection
  ws.send(JSON.stringify({ action: 'welcome', message: 'Connected to WebSocket server' }));
});

console.log('WebSocket server running on ws://localhost:8080');