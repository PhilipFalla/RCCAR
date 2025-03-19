#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <hidapi.h>
#include <cJSON.h>
#include <unistd.h>
#include <curl/curl.h> // Add libcurl for HTTP requests

// ESP32's IP
#define ESP32_IP "172.20.10.3"

// Firebase API Key
#define FIREBASE_API_KEY "API"
//#define FIREBASE_API_KEY "API"

// Firebase URL
#define FIREBASE_URL "URL"
//#define FIREBASE_URL "URL"


// Firebase email
#define EMAIL "EMAIL"

// Firebase Pass
#define PASS "PASS"

// Response buffer for authentication
#define RESPONSE_BUFFER_SIZE 4096

// Compile with:
//gcc -o controller_state main.c \
//    -I/opt/homebrew/Cellar/hidapi/0.14.0/include/hidapi \
//    -L/opt/homebrew/Cellar/hidapi/0.14.0/lib -lhidapi \
//    -I/opt/homebrew/Cellar/cjson/1.7.18/include/cjson \
//    -L/opt/homebrew/Cellar/cjson/1.7.18/lib -lcjson \
//    -I/opt/homebrew/include/SDL2 \
//    -L/opt/homebrew/lib -lSDL2

// Write callback for curl response
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    strncat((char *)userp, (char *)contents, total_size);
    return total_size;
}

// Authenticate with Firebase and get token
char* authenticate_with_firebase(const char* email, const char* password) {
    CURL *curl;
    CURLcode res;
    char *token = NULL;
    char response[RESPONSE_BUFFER_SIZE] = {0};  // Initialize response buffer

    char url[256];
    snprintf(url, sizeof(url),
             "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%s",
             FIREBASE_API_KEY);

    char post_data[512];
    snprintf(post_data, sizeof(post_data),
             "{\"email\":\"%s\",\"password\":\"%s\",\"returnSecureToken\":true}",
             email, password);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            printf("Firebase Auth Response: %s\n", response);  // Print full response
            cJSON *json = cJSON_Parse(response);
            cJSON *token_json = cJSON_GetObjectItem(json, "idToken");
            if (token_json) {
                token = strdup(token_json->valuestring);
                printf("Authentication successful! Token: %s\n", token);
            } else {
                fprintf(stderr, "Failed to retrieve ID token. Firebase response: %s\n", response);
            }
            cJSON_Delete(json);
        } else {
            fprintf(stderr, "Authentication failed: %s\n", curl_easy_strerror(res));
        }


        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return token;
}

// Send JSON data to Firebase
void send_json_to_firebase(const char* json_str, const char* id_token) {
    if (!id_token) {
        fprintf(stderr, "Error: Firebase authentication token is NULL.\n");
        return;
    }

    CURL *curl;
    CURLcode res;

    char url[512];
    snprintf(url, sizeof(url), 
             FIREBASE_URL "controller.json?auth=%s", id_token);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to send JSON: %s\n", curl_easy_strerror(res));
        } else {
            char response[RESPONSE_BUFFER_SIZE] = {0};  // Store Firebase response
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);  // Capture response
            printf("Firebase Response: %s\n", response);  // Debugging

            if (strstr(response, "error")) {  // Check for error in Firebase response
                fprintf(stderr, "Firebase Error: %s\n", response);
            } else {
                printf("Data successfully sent to Firebase!\n");
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

int main(void) {
    char* Token = authenticate_with_firebase(EMAIL, PASS);
    if (Token) {
        printf("Using Token: %s\n", token);
    } else {
        fprintf(stderr, "No authentication token available!\n");
    }

    if (!Token) {
        fprintf(stderr, "Error: Unable to authenticate with Firebase.\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        free(Token);  // Free token before exiting
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
        free(Token);
        return 1;
    }

    while (1) {
        SDL_GameControllerUpdate();

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
            send_json_to_firebase(json_str, Token);
            free(json_str);
        }

        cJSON_Delete(root);

        // Sleep for 500ms (Reduce request frequency)
        usleep(500000);
    }

    SDL_GameControllerClose(controller);
    SDL_Quit();
    free(Token);  // Free allocated memory
    return 0;
}