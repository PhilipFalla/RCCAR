//Compile with:
/*
    gcc -o controller_state main.c \
    -I/opt/homebrew/opt/openssl@3/include \
    -L/opt/homebrew/opt/openssl@3/lib \
    -I/opt/homebrew/Cellar/hidapi/0.14.0/include/hidapi \
    -I/opt/homebrew/Cellar/cjson/1.7.18/include/cjson \
    -I/opt/homebrew/include/SDL2 \
    -I/opt/homebrew/Cellar/libwebsockets/4.3.5/include \
    -L/opt/homebrew/Cellar/hidapi/0.14.0/lib -lhidapi \
    -L/opt/homebrew/Cellar/cjson/1.7.18/lib -lcjson \
    -L/opt/homebrew/lib -lSDL2 \
    -L/opt/homebrew/lib -lwebsockets \
    -lssl -lcrypto

    Then:

    ./controller_state
*/

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <hidapi.h>
#include <cJSON.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <string.h>

// WebSocket server IP and port
#define SERVER_IP "18ee-181-174-72-222.ngrok-free.app"
#define SERVER_PORT 443

// Response buffer for WebSocket
#define RESPONSE_BUFFER_SIZE 4096

// Global WebSocket context and client
struct lws_context *context;
struct lws *websocket;
int connection_established = 0; // Flag to track connection status

// Callback for WebSocket events
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                               void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("WebSocket Connection Established\n");
            connection_established = 1; // Set connection flag
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("Received message: %s\n", (char *)in);
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // WebSocket is ready to send a message
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("WebSocket Connection Error\n");
            connection_established = 0; // Reset connection flag
            break;
        case LWS_CALLBACK_CLOSED:
            printf("WebSocket Closed\n");
            connection_established = 0; // Reset connection flag
            break;
        default:
            break;
    }
    return 0;
}

// WebSocket protocol structure
static struct lws_protocols protocols[] = {
    {
        "example-protocol",
        callback_websocket,
        0,
        RESPONSE_BUFFER_SIZE,
    },
    { NULL, NULL, 0, 0 }
};

// Send JSON data via WebSocket
void send_json_via_websocket(const char *json_str) {
    if (websocket && connection_established) {
        int len = strlen(json_str);
        unsigned char *msg = (unsigned char *)malloc(LWS_PRE + len + 1);
        if (!msg) {
            fprintf(stderr, "Failed to allocate memory for WebSocket message\n");
            return;
        }
        memcpy(msg + LWS_PRE, json_str, len + 1); // Copy message after LWS_PRE
        lws_write(websocket, msg + LWS_PRE, len, LWS_WRITE_TEXT);
        free(msg);
    }
}

// Initialize WebSocket connection
int init_websocket_connection() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;  // Ensure SSL is initialized

    context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "WebSocket context creation failed\n");
        return -1;
    }

    struct lws_client_connect_info client_info = {0};
    client_info.context = context;
    client_info.address = SERVER_IP;  // ngrok URL
    client_info.port = SERVER_PORT;   // Use 443 for WSS
    client_info.path = "/";           // WebSocket endpoint
    client_info.host = SERVER_IP;
    client_info.origin = SERVER_IP;
    client_info.protocol = protocols[0].name;
    client_info.ssl_connection = LCCSCF_USE_SSL;  // Enable SSL

    websocket = lws_client_connect_via_info(&client_info);

    if (!websocket) {
        fprintf(stderr, "Failed to connect to WebSocket server\n");
        return -1;
    }

    // Wait for the connection to be established
    int timeout = 5000; // 5 seconds
    while (!connection_established && timeout > 0) {
        lws_service(context, 100); // Poll WebSocket events
        usleep(1000); // Sleep for 1ms
        timeout -= 100;
    }

    if (!connection_established) {
        fprintf(stderr, "WebSocket connection timed out\n");
        return -1;
    }

    return 0;
}

int main(void) {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GameController *controller = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                printf("Controller connected!\n");
                break;
            }
        }
    }

    if (!controller) {
        fprintf(stderr, "No controller found!\n");
        SDL_Quit();
        return 1;
    }

    // Initialize WebSocket
    if (init_websocket_connection() != 0) {
        SDL_GameControllerClose(controller);
        SDL_Quit();
        return 1;
    }

    while (1) {
        SDL_GameControllerUpdate(); // Poll controller state

        cJSON *root = cJSON_CreateObject();

        // Joysticks (Left Joystick)
        cJSON *json_joysticks = cJSON_CreateObject();
        cJSON *json_left = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_left, "x", SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX));
        cJSON_AddNumberToObject(json_left, "y", SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY));
        cJSON_AddItemToObject(json_joysticks, "left", json_left);
        cJSON_AddItemToObject(root, "joysticks", json_joysticks);

        // Triggers (L2 and R2)
        cJSON *json_triggers = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_triggers, "left", SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        cJSON_AddNumberToObject(json_triggers, "right", SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
        cJSON_AddItemToObject(root, "triggers", json_triggers);

        // Convert JSON object to string
        char *json_str = cJSON_Print(root);
        if (json_str) {
            send_json_via_websocket(json_str);
            free(json_str);
        }

        cJSON_Delete(root);

        // Poll WebSocket events
        lws_service(context, 0);

        // Reduce sleep time to minimize delay
        usleep(1000); // Sleep for 1ms
    }

    lws_context_destroy(context);
    SDL_GameControllerClose(controller);
    SDL_Quit();
    return 0;
}