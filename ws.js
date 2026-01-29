// node ws.js
// ngrok http 8080

const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

// Client tracking
const clients = {
  python: null,
  esp32: null
};

wss.on('connection', (ws) => {
  console.log('New client connected');
  
  // Handle client identification
  const identifyClient = (message) => {
    const msgString = message.toString();
    
    if (msgString.includes('"type":"esp32_identity"')) {
      clients.esp32 = ws;
      console.log('ESP32 identified');
      return 'esp32';
    }
    else if (msgString.includes('"joysticks"')) {
      clients.python = ws;
      console.log('Python controller identified');
      return 'python';
    }
    return null;
  };

  // Initial identification
  ws.once('message', (message) => {
    const clientType = identifyClient(message);
    if (!clientType) {
      console.log('Unknown client type, disconnecting');
      ws.close();
    }
  });

  // Subsequent messages
  ws.on('message', (message) => {
    const msgString = message.toString();
    
    // Handle ESP32 acknowledgments
    if (msgString.includes('"type":"ack"')) {
      console.log('Received acknowledgment from ESP32');
      return;
    }
    
    // Forward Python controller data to ESP32
    if (ws === clients.python && clients.esp32) {
      console.log('Forwarding controller data to ESP32');
      console.log("Attempting to send this exact message to ESP32:");
      console.log(message.toString());
      clients.esp32.send(message);
    }
  });

  ws.on('close', () => {
    if (ws === clients.python) {
      console.log('Python controller disconnected');
      clients.python = null;
    }
    else if (ws === clients.esp32) {
      console.log('ESP32 disconnected');
      clients.esp32 = null;
    }
  });
});

console.log('WebSocket server running on ws://localhost:8080');