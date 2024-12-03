#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#define MAX_PLAYERS 8
#define FIELD_SIZE 20
#define STUN_DURATION 10
#define ZAP_COOLDOWN 3

#define ASTRONAUT_MIN 2
#define ASTRONAUT_MAX FIELD_SIZE - 2

typedef struct {
    char id;
    int x, y;
    int score;
    clock_t last_stunned; // Last stunned time
    clock_t finished_zap; // Last zap time
    char move_type;
    int code;
    // void *client_socket;
} Astronaut;

typedef struct {
    int x, y;
} Alien;

game_state state;
Astronaut astronauts[MAX_PLAYERS];
int astronaut_count = 0;

time_t last_alien_move;

void initializeGame() {
    // Initialize the game field to empty spaces
    for (int i = 0; i < FIELD_SIZE; i++) {
        for (int j = 0; j < FIELD_SIZE; j++) {
            state.game_field[i][j] = ' '; // Fill with empty spaces
        }
    }

    // Place aliens in the center of the game field
    srand(time(NULL));
    int alien_count = (FIELD_SIZE - 4) * (FIELD_SIZE - 4) / 3;

    for (int i = 0; i < alien_count; i++) {
        int x, y;
        do {
            x = rand() % (FIELD_SIZE - 4) + 2;
            y = rand() % (FIELD_SIZE - 4) + 2;
        } while (state.game_field[x][y] != ' '); // Ensure position is empty

        state.game_field[x][y] = '*'; // Place an alien
    }
}

void updateGameState() {
    clock_t current_time;
    time(&current_time);

    // Add seconds 
    if (current_time > last_alien_move + 1) {
        // moveAliens();
        last_alien_move = current_time;
    }

    // Update laser rays
}

void sendGameState(void *socket) {
    // Create a buffer to hold the serialized data
    char buffer[FIELD_SIZE * FIELD_SIZE + MAX_PLAYERS * sizeof(int) + 1];
    int offset = 0;

    // Serialize game_field
    for (int i = 0; i < FIELD_SIZE; i++) {
        memcpy(buffer + offset, state.game_field[i], FIELD_SIZE);
        offset += FIELD_SIZE;
    }

    // Serialize astronaut_scores
    memcpy(buffer + offset, state.astronaut_scores, MAX_PLAYERS * sizeof(int));
    
    // Send the serialized data
    zmq_send(socket, buffer, sizeof(buffer), 0);
}

char validateMessage(astronaut_client *message) {
    // char type = message[0];
    // char id = message[1];
    // if (type != 'C' && type != 'M' && type != 'Z' && type != 'D') return 0;
    if (message->id < 'A' || message->id > 'H') return 0;
    return message->id;
}

int findAstronautIndex(char id) {
    for (int i = 0; i < astronaut_count; i++) {
        if (astronauts[i].id == id) return i;
    }
    return -1;
}

void moveAustronautInField(int old_x, int old_y, int new_x, int new_y) {
    if (old_x == new_x && old_y == new_y) return;
    state.game_field[new_x][new_y] = state.game_field[old_x][old_y];
    state.game_field[old_x][old_y] = ' ';
}

void moveAustronautInZone(Astronaut* astronaut, bool delete) {
    if (astronaut->move_type == 'V') {
        for (int y = ASTRONAUT_MIN; y <= ASTRONAUT_MAX; y++) {
            state.game_field[astronaut->x][y] = ' '; 
        }
    } else  if (astronaut->move_type == 'H') {
        for (int x = ASTRONAUT_MIN; x <= ASTRONAUT_MAX; x++) {
            state.game_field[x][astronaut->y] = ' '; 
        }
    }

    if (!delete) {
        state.game_field[astronaut->x][astronaut->y] = astronaut->id;
    }
}

char handleAstronautConnect(void *client_socket) {
    if (astronaut_count >= MAX_PLAYERS) {
        zmq_send(client_socket, "Z", 1, 0); // F for Full
        return 'Z';
    }
    char id = 'A' + astronaut_count;
    astronauts[astronaut_count].id = id;
    
    //Initialize Position from ID
    int middle = 10;
    if (astronaut_count % 2 == 0) {
        astronauts[astronaut_count].move_type = 'V';
        astronauts[astronaut_count].x = 1;
        astronauts[astronaut_count].y = middle;
    } else {
        astronauts[astronaut_count].move_type = 'H';
        astronauts[astronaut_count].x = middle;
        astronauts[astronaut_count].y = 1;
    }
    moveAustronautInZone(&astronauts[astronaut_count], false);

    astronaut_count++;
    return id;
}

void handleAstronautMovement(char id, direction_t direction) {
    int index = findAstronautIndex(id);
    clock_t current_time = clock();
    if (index == -1 || difftime(current_time, astronauts[index].last_stunned) < STUN_DURATION) return;

    int new_x = astronauts[index].x;
    int new_y = astronauts[index].y;

    if (astronauts[index].move_type == 'V') { // Vertical movement
        switch(direction) {
            case DOWN: // Move down
                new_y++;
                break;
            case UP: // Move up
                new_y--;
                break;
        }
    } else if (astronauts[index].move_type == 'H') { // Horizontal movement
        switch(direction) {
            case RIGHT: // Move right
                new_x++;
                break;
            case LEFT: // Move left
                new_x--;
                break;
        }
    }

    // Check bounds for vertical movement
    if (new_y >= ASTRONAUT_MIN && new_y <= ASTRONAUT_MAX) {
        astronauts[index].y = new_y; // Update position if within bounds
    }

    // Check bounds for horizontal movement
    if (new_x >= ASTRONAUT_MIN && new_x <= ASTRONAUT_MAX) {
        astronauts[index].x = new_x; // Update position if within bounds
    }

    moveAustronautInZone(&astronauts[index], false);
}

void handleAstronautZap(char id) {
    int index = findAstronautIndex(id);
    if (index == -1 || astronauts[index].last_stunned > 0) return;

    clock_t current_time = clock() * CLOCKS_PER_SEC;
    if (difftime(current_time, astronauts[index].finished_zap) < ZAP_COOLDOWN) return;

    astronauts[index].finished_zap = current_time;

    // Implement zapping logic
    // Check for alien hits and update scores
    // Stun other astronauts in the line of fire
}

void handleAstronautDisconnect(char id) {
    int index = findAstronautIndex(id);
    if (index == -1) return;

    // Remove astronaut from the game
    // TODO: this will move the following astronauts but this may not be wanted
    for (int i = index; i < astronaut_count - 1; i++) {
        astronauts[i] = astronauts[i + 1];
    }
    astronaut_count--;
    moveAustronautInZone(&astronauts[astronaut_count], true);
}

void processMessage(void *socket) {
    astronaut_client message;
    int size = zmq_recv(socket, &message, sizeof(message), 0);
    if (size == -1) {
        printf("Error receiving message: %s\n", zmq_strerror(zmq_errno()));
        return;
    }

    char id = validateMessage(&message);

    if (message.msg_type == 0) {
        char id = handleAstronautConnect(socket);
        astronaut_connect con_reply;
        con_reply.id = id;
        con_reply.code = random();
        int index = findAstronautIndex(id);
        astronauts[index].code = con_reply.code;
        zmq_send(socket, &con_reply, sizeof(astronaut_connect), 0);
        return;
    }

    if (id == 0) return;

    switch(message.msg_type) {
        case 1:
            handleAstronautMovement(id, message.direction);
            break;
        case 2:
            handleAstronautZap(id);
            break;
        case 3:
            handleAstronautDisconnect(id);
            break;
    }

    zmq_send(socket, &state.astronaut_scores, sizeof(state.astronaut_scores), 0);
}

void printGameField(game_state *state) {
    for (int i = 0; i < FIELD_SIZE; i++) {
        for (int j = 0; j < FIELD_SIZE; j++) {
            printf("%c ", state->game_field[i][j]);
        }
        printf("\n");
    }
}

int main() {
    void *context = zmq_ctx_new();

    // Interaction REP socket
    void *responder_interaction = zmq_socket(context, ZMQ_REP);
    if (zmq_bind(responder_interaction, INTERACTION_ADDRESS) == -1) {
        printf("Error binding: %s\n", zmq_strerror(zmq_errno()));
    }
    // assert(rc_interaction == 0);

    // Display PUP socket
    void *publisher_display = zmq_socket(context, ZMQ_PUB);
    int rc_display = zmq_bind(publisher_display, DISPLAY_ADDRESS);
    // assert(rc_display == 0);

    initializeGame();

    while (1) {
        processMessage(responder_interaction);
        updateGameState();
        printGameField(&state);
        sendGameState(publisher_display);
    }

    zmq_close(responder_interaction);
    zmq_close(publisher_display);
    zmq_ctx_destroy(context);
    
    return 0;
}