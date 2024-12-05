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
#define STUN_DURATION 10
#define ZAP_COOLDOWN 3

#define ASTRONAUT_MIN 2
#define ASTRONAUT_MAX (FIELD_SIZE - 2)

typedef struct {
    char id;
    int x, y;
    int *score;
    struct timespec finished_stunned; // Last stunned time
    struct timespec finished_zap; // Last zap time
    int zap_active;
    char move_type;
    int code;
    int connect;
} Astronaut;

typedef struct {
    int x, y;
} Alien;

game_state state;
Astronaut astronauts[MAX_PLAYERS];
int astronaut_count = 0;
char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

double time_diff_ms(struct timespec start, struct timespec end) {
    double result = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    return result;
}

void initialize_astronauts(){     // can also replace austronaut when it gets disconnected
 
    if (astronauts[0].connect == 0){
       
        astronauts[0].id = 'A';
        astronauts[0].move_type = 'V';
        astronauts[0].x = 1;
        astronauts[0].y = FIELD_SIZE/2;
        astronauts[0].score = 0;
   
        }
 
    if (astronauts[1].connect == 0){
 
        astronauts[1].id = 'B';
        astronauts[1].move_type = 'V';
        astronauts[1].x = FIELD_SIZE-2;
        astronauts[1].y = FIELD_SIZE/2;
        astronauts[1].score = 0;
   
        }
 
    if (astronauts[2].connect == 0){ // new
 
        astronauts[2].id = 'C';
        astronauts[2].move_type = 'H';
        astronauts[2].x = FIELD_SIZE/2;
        astronauts[2].y = 1;
        astronauts[2].score = 0;
   
        }
 
    if (astronauts[3].connect == 0){
 
        astronauts[3].id = 'D';
        astronauts[3].move_type = 'H';
        astronauts[3].x = FIELD_SIZE/2;
        astronauts[3].y = FIELD_SIZE-1;
        astronauts[3].score = 0;
   
        }
 
    if (astronauts[4].connect == 0){ // new
 
        astronauts[4].id = 'E';
        astronauts[4].move_type = 'V';
        astronauts[4].x = 0;
        astronauts[4].y = FIELD_SIZE/2;
        astronauts[4].score = 0;
   
        }
 
    if (astronauts[5].connect == 0){
 
        astronauts[5].id = 'F';
        astronauts[5].move_type = 'V';
        astronauts[5].x = FIELD_SIZE-2;
        astronauts[5].y = FIELD_SIZE/2;
        astronauts[5].score = 0;
   
        }
 
    if (astronauts[6].connect == 0){ // new
 
        astronauts[6].id = 'G';
        astronauts[6].move_type = 'H';
        astronauts[6].x = FIELD_SIZE/2;
        astronauts[6].y = 0;
        astronauts[6].score = 0;
   
        }
 
    if (astronauts[7].connect == 0){
 
    astronauts[7].id = 'H';
    astronauts[7].move_type = 'V';
    astronauts[7].x = FIELD_SIZE/2;
    astronauts[7].y = FIELD_SIZE-1;
    astronauts[7].score = 0;

    }
}

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

    initialize_astronauts();
}

int findAstronautIndex(char id) {
    for (int i = 0; i < astronaut_count; i++) {
        if (astronauts[i].id == id) return i;
    }
    return -1;
}

void clearLaserPath(Astronaut *astronaut) {
    if (astronaut->move_type == 'H') { 
        for (int j = 0; j < FIELD_SIZE; j++) {
            if (state.game_field[astronaut->x][j] == '|') {
                state.game_field[astronaut->x][j] = ' '; // Clear horizontal laser
            }
        }
    } else if (astronaut->move_type == 'V') { 
        for (int i = 0; i < FIELD_SIZE; i++) {
            if (state.game_field[i][astronaut->y] == '-') {
                state.game_field[i][astronaut->y] = ' '; // Clear vertical laser
            }
        }
    }
}

void drawLaserPath(Astronaut *astronaut) {
    if (!astronaut->zap_active) return;

    if (astronaut->move_type == 'H') { 
        for (int j = 0; j < FIELD_SIZE; j++) {
            if (state.game_field[astronaut->x][j] != ' ') continue;
            state.game_field[astronaut->x][j] = '|'; // Draw horizontal laser
        }
    } else if (astronaut->move_type == 'V') { 
        for (int i = 0; i < FIELD_SIZE; i++) {
            if (state.game_field[i][astronaut->y] != ' ') continue;
            state.game_field[i][astronaut->y] = '-'; // Draw vertical laser
        }
    }
}

void updateGameState() {
    // clock_t current_time = clock();
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Update laser rays
    for (int i = 0; i < astronaut_count; i++) {
        Astronaut *astronaut = &astronauts[i];

        if (astronaut->zap_active) {
            if (time_diff_ms(current_time, astronaut->finished_zap) < 0) {
                clearLaserPath(astronaut);
                astronaut->zap_active = 0;
                continue;
            }

            // Check if the laser duration has expired
            // if (time_diff_ms(current_time, astronauts[i].finished_zap) < 0) {
            //     astronaut->zap_active = 0; // Deactivate zap
                
            //     // Clear the laser from game field
            //     if (astronaut->move_type == 'H') { 
            //         for (int j = 0; j < FIELD_SIZE; j++) {
            //             if (state.game_field[astronaut->x][j] != '|') continue;
            //             state.game_field[astronaut->x][j] = ' '; // Clear horizontal laser
            //         }
            //     } else if (astronaut->move_type == 'V') { 
            //         for (int j = 0; j < FIELD_SIZE; j++) {
            //             if (state.game_field[j][astronaut->y] != '-') continue;
            //             state.game_field[j][astronaut->y] = ' '; // Clear vertical laser
            //         }
            //     }
            //     continue;
            // }

            // Check for alien hits in the zap's path
            int start_x = astronaut->x;
            int start_y = astronaut->y;

            if (astronaut->move_type == 'H') { 
                for (int j = ASTRONAUT_MIN; j <= ASTRONAUT_MAX; j++) {
                    if (state.game_field[start_x][j] == '*') { 
                        state.game_field[start_x][j] = '|'; // Remove alien from field
                        (*astronaut->score)++; 
                    }
                }
            } else if (astronaut->move_type == 'V') { 
                for (int j = ASTRONAUT_MIN; j <= ASTRONAUT_MAX; j++) {
                    if (state.game_field[j][start_y] == '*') { 
                        state.game_field[j][start_y] = '-'; 
                        (*astronaut->score)++; 
                    }
                }
            }

            // Check for other astronauts in the line of fire
            for (int j = 0; j < astronaut_count; j++) {
                if (astronauts[j].id != astronaut->id) { 
                    if ((astronaut->move_type == 'H' && astronaut->x == astronauts[j].x) ||
                        (astronaut->move_type == 'V' && astronauts[i].y == astronauts[j].y)) {
                        astronauts[j].finished_stunned = current_time;
                        astronauts[j].finished_stunned.tv_sec += STUN_DURATION;
                    }
                }
            }
        }
    }

}

void sendGameState(void *socket) {
    //zmq_send(socket, "1", strlen("1"),ZMQ_SNDMORE );
    zmq_send(socket, &state, sizeof(state), 0);
}

char validateMessage(astronaut_client *message) {
    if (message->msg_type < 0 || message->msg_type > 3) return 0;
    if (message->id < 'A' || message->id > 'H') return 0;
    int index = findAstronautIndex(message->id);
    if (index < 0 || message->code != astronauts[index].code) return 0;
    return message->id;
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



void handleAstronautConnect(void *client_socket) {
    astronaut_connect con_reply;
    if (astronaut_count >= MAX_PLAYERS) {
        con_reply.id = 'Z';
        con_reply.code = -1;
        zmq_send(client_socket, &con_reply, sizeof(con_reply), 0);
        return;
    }
    char id = 'A' + astronaut_count;
    astronauts[astronaut_count].id = id;
    astronauts[astronaut_count].score = &state.astronaut_scores[astronaut_count];
    *astronauts[astronaut_count].score = 0;

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    astronauts[astronaut_count].finished_stunned = current_time;

    //Initialize Position from ID
    // int middle = 10;
    // if (astronaut_count % 2 == 0) {
    //     astronauts[astronaut_count].move_type = 'V';
    //     astronauts[astronaut_count].x = 0;
    //     astronauts[astronaut_count].y = middle;
    // } else {
    //     // astronauts[astronaut_count].move_type = 'H';
    //     // astronauts[astronaut_count].x = middle;
    //     // astronauts[astronaut_count].y = 0;
    //     astronauts[astronaut_count].move_type = 'V';
    //     astronauts[astronaut_count].x = FIELD_SIZE - 2;
    //     astronauts[astronaut_count].y = middle;
    // }
    moveAustronautInZone(&astronauts[astronaut_count], false);
    
    con_reply.id = id;
    con_reply.code = random();
    astronauts[astronaut_count].code = con_reply.code;
    zmq_send(client_socket, &con_reply, sizeof(con_reply), 0);
    astronaut_count++;

    return;
}

void handleAstronautMovement(char id, direction_t direction) {
    int index = findAstronautIndex(id);
    Astronaut *astronaut = &astronauts[index];

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double test = time_diff_ms(current_time, astronaut->finished_stunned);
    if (index == -1 || test > 0.0) return;
    
    clearLaserPath(astronaut);

    int new_x = astronaut->x;
    int new_y = astronaut->y;

    if (astronaut->move_type == 'V') { // Vertical movement
        switch(direction) {
            case DOWN: // Move down
                new_y++;
                break;
            case UP: // Move up
                new_y--;
                break;
        }
    } else if (astronaut->move_type == 'H') { // Horizontal movement
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
        astronaut->y = new_y;
    }

    // Check bounds for horizontal movement
    if (new_x >= ASTRONAUT_MIN && new_x <= ASTRONAUT_MAX) {
        astronaut->x = new_x;
    }

    moveAustronautInZone(astronaut, false);

    if (time_diff_ms(current_time, astronaut->finished_zap) >= 0) {
        drawLaserPath(astronaut);
    }
}

void handleAstronautZap(char id) {
    int index = findAstronautIndex(id);

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    if (index == -1 || time_diff_ms(current_time, astronauts[index].finished_stunned) > 0) return;
    
    if (time_diff_ms(astronauts[index].finished_zap, current_time) < ZAP_COOLDOWN * 1000) return;

    astronauts[index].finished_zap = current_time;
    astronauts[index].finished_zap = current_time;
    astronauts[index].finished_zap.tv_nsec += 500000000; // Add 500ms
    if (astronauts[index].finished_zap.tv_nsec >= 1000000000) {
        astronauts[index].finished_zap.tv_sec++;
        astronauts[index].finished_zap.tv_nsec -= 1000000000;
    }

    // Update game_field with the laser representation
    if (astronauts[index].move_type == 'H') { // Horizontal zap
        int pos = astronauts[index].x;
        for (int i = 0; i < FIELD_SIZE; i++) {
            if (state.game_field[pos][i] != ' ') continue;
            state.game_field[pos][i] = '|';
        }
    } else if (astronauts[index].move_type == 'V') { // Vertical zap
        int pos = astronauts[index].y;
        for (int i = 0; i < FIELD_SIZE; i++) {
            if (state.game_field[i][pos] != ' ') continue;
            state.game_field[i][pos] = '-';
        }
    }

    astronauts[index].zap_active = 1;
}

void handleAstronautDisconnect(char id) {
    int index = findAstronautIndex(id);
    if (index == -1) return;

    // Remove astronaut from the game
    // TODO: this will move the following astronauts but this may not be wanted
    // for (int i = index; i < astronaut_count - 1; i++) {
    //     astronauts[i] = astronauts[i + 1];
    // }
    initialize_astronauts();

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

    if (message.msg_type == 0) {
        handleAstronautConnect(socket);
        return;
    }

    char id = validateMessage(&message);

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

void printGameField(game_state *state,WINDOW * my_win,WINDOW * my_win_2) {

    for (int i = 1; i < FIELD_SIZE+1; i++) {
        for (int j = 1; j < FIELD_SIZE+1; j++) {
            //printf("%c ", state->game_field[i][j]);
            wmove(my_win, j, i);
            waddch(my_win,state->game_field[i-1][j-1]| A_BOLD);
        }}
        //printf("\n");

        // update score screen 
        mvwprintw(my_win_2, 1, 1, "SCORE");
    
        for (int i = 0; i < MAX_PLAYERS; i++){
            mvwprintw(my_win_2, i+2, 1, "%c - %d",letters[i], state->astronaut_scores[i]);
        } 

        box(my_win, 0 , 0);
        wrefresh(my_win);
        wrefresh(my_win_2);
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

    // start variables
    initializeGame();

    // lncurses 
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    
    /* creates a window and draws a border */
    WINDOW * my_win = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 0, 0);
    WINDOW * my_win_2 = newwin(15, 30, 0, FIELD_SIZE+4);
    box(my_win_2, 0 , 0);	
	wrefresh(my_win);

    while (1) {
        processMessage(responder_interaction);
        updateGameState();
        printGameField(&state,my_win,my_win_2);
        sendGameState(publisher_display);
    }

    endwin();			/* End curses mode		  */

    zmq_close(responder_interaction);
    zmq_close(publisher_display);
    zmq_ctx_destroy(context);
    
    return 0;
}