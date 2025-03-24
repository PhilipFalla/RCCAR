import pygame
import json
import asyncio
import websockets
import ssl  # Added for SSL handling

# WebSocket server configuration
SERVER_IP = "2c47-138-117-143-165.ngrok-free.app"
SERVER_PORT = 443
WS_URI = f"wss://{SERVER_IP}:{SERVER_PORT}/"

class WebSocketClient:
    def __init__(self):
        self.websocket = None
        self.connection_established = False
    
    async def connect(self):
        try:
            # Create SSL context that doesn't verify certificates
            ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            ssl_context.check_hostname = False
            ssl_context.verify_mode = ssl.CERT_NONE
            
            self.websocket = await websockets.connect(
                WS_URI,
                ssl=ssl_context
            )
            self.connection_established = True
            print("WebSocket Connection Established")
            return True
        except Exception as e:
            print(f"WebSocket Connection Error: {e}")
            self.connection_established = False
            return False
    
    async def send_json(self, data):
        if self.connection_established and self.websocket:
            try:
                await self.websocket.send(json.dumps(data))
            except Exception as e:
                print(f"Error sending WebSocket message: {e}")
                self.connection_established = False
                await self.close()
    
    async def close(self):
        if self.websocket:
            await self.websocket.close()
            self.connection_established = False
            print("WebSocket Closed")

def main():
    # Initialize pygame
    pygame.init()
    pygame.joystick.init()
    
    # Initialize controller
    controller = None
    joystick_count = pygame.joystick.get_count()
    print(f"Found {joystick_count} joystick(s)")
    
    for i in range(joystick_count):
        try:
            controller = pygame.joystick.Joystick(i)
            controller.init()
            print(f"Controller connected: {controller.get_name()}")
            break
        except pygame.error as e:
            print(f"Error initializing joystick {i}: {e}")
            continue
    
    if not controller:
        print("No valid controller found!")
        pygame.quit()
        return
    
    # Create WebSocket client
    ws_client = WebSocketClient()
    
    async def controller_loop():
        # Connect to WebSocket
        if not await ws_client.connect():
            return
        
        try:
            while True:
                pygame.event.pump()  # Process event queue
                
                # Create data dictionary
                data = {
                    "joysticks": {
                        "left": {
                            "x": controller.get_axis(0),  # Left stick X
                            "y": controller.get_axis(1)   # Left stick Y
                        }
                    },
                    "triggers": {
                        "left": (controller.get_axis(4) + 1) / 2 * 32767,  # Convert -1 to 1 range to 0 to 32767
                        "right": (controller.get_axis(5) + 1) / 2 * 32767  # Convert -1 to 1 range to 0 to 32767
                    }
                }
                
                # Send data via WebSocket
                await ws_client.send_json(data)
                
                # Small delay to prevent high CPU usage
                await asyncio.sleep(0.001)
                
        except KeyboardInterrupt:
            pass
        except Exception as e:
            print(f"Error: {e}")
        finally:
            await ws_client.close()
            if controller:
                controller.quit()
            pygame.quit()
    
    # Run the asyncio event loop
    asyncio.run(controller_loop())

if __name__ == "__main__":
    main()