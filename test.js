// node text.js

const WebSocket = require('ws');

const socket = new WebSocket("wss://936a-138-117-143-165.ngrok-free.app");

socket.onopen = () => {
    console.log("Connected to WebSocket server");
};

socket.onmessage = (event) => {
    console.log("Received:", event.data);
};

socket.onerror = (error) => {
    console.error("WebSocket Error:", error);
};

socket.onclose = () => {
    console.log("WebSocket connection closed");
};