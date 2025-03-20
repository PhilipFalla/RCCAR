const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', function connection(ws) {
  console.log('Client connected');

  ws.on('message', function incoming(message) {
    console.log('received: %s', message);
  });

  ws.send(JSON.stringify({ action: 'welcome', message: 'Connected to WebSocket server' }));
});

// Broadcast a message every 5 seconds to all clients
setInterval(() => {
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify({ action: 'ping' }));
    }
  });
}, 5000);

console.log('WebSocket server running on ws://localhost:8080');
