// node ws.js
// ngrok http 8080

const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8080 });
const connectedClients = new Set();

console.log('WebSocket server running on ws://localhost:8080');

wss.on('connection', function connection(ws) {
  // Add client to tracking set
  connectedClients.add(ws);
  const clientIp = ws._socket.remoteAddress;
  console.log(`New client connected (Total: ${connectedClients.size}) - IP: ${clientIp}`);

  // Send welcome message
  const welcomeMsg = JSON.stringify({ 
    action: 'welcome', 
    message: 'Connected to WebSocket server',
    timestamp: new Date().toISOString()
  });
  ws.send(welcomeMsg);

  // Message handler with enhanced logging
  ws.on('message', function incoming(message) {
    try {
      const timestamp = new Date().toISOString();
      let parsedMessage = message;
      
      // Try to parse JSON if possible
      try {
        parsedMessage = JSON.parse(message);
      } catch (e) {
        // Not JSON, keep as raw message
      }

      console.log('\n--- Received Message ---');
      console.log(`Timestamp: ${timestamp}`);
      console.log(`Client IP: ${clientIp}`);
      console.log('Raw message:', message);
      console.log('Parsed message:', parsedMessage);
      console.log('--- End of Message ---\n');

      // Here you can add specific message handling logic
      // For example, respond to certain message types:
      if (typeof parsedMessage === 'object' && parsedMessage.type === 'ping') {
        ws.send(JSON.stringify({
          type: 'pong',
          timestamp: new Date().toISOString()
        }));
      }

    } catch (error) {
      console.error('Error processing message:', error);
    }
  });

  // Handle client disconnection
  ws.on('close', function() {
    connectedClients.delete(ws);
    console.log(`Client disconnected (Remaining: ${connectedClients.size}) - IP: ${clientIp}`);
  });

  // Handle errors
  ws.on('error', function(error) {
    console.error('WebSocket error from client', clientIp, ':', error);
  });
});

// Periodic server status logging
setInterval(() => {
  console.log(`Server status - Connected clients: ${connectedClients.size}, Time: ${new Date().toISOString()}`);
}, 1000); // Log every minute