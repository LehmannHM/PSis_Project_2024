#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../common.h"

#include <sys/types.h>

// ---------------- constants -------------------------------
#define MAX_PLAYERS 8
#define STUN_DURATION 10
#define ZAP_COOLDOWN 3

#define ASTRONAUT_MIN 2
#define ASTRONAUT_MAX (FIELD_SIZE - 2)
#define ALIEN_MAX_COUNT ((FIELD_SIZE - 4) * (FIELD_SIZE - 4) / 3)

// ---------------- types -------------------------------
typedef struct {
    char id;
    int x, y;
    int score;
    struct timespec finished_stunned; // Last stunned time
    struct timespec finished_zap; // Last zap time
    int zap_active;
    char move_type; // 'H' or 'V' for horizontal or vertical movement
    unsigned char token[TOKEN_SIZE];
    int connect; // 0 if disconnected, 1 if connected
} Astronaut;

typedef struct {
    int x, y;
    int connect;
    char id;
    unsigned char token[TOKEN_SIZE];
} Alien;

// initialize global variables
char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
game_state state;
Astronaut astronauts[MAX_PLAYERS];
int astronaut_count = 0;
Alien aliens[ALIEN_MAX_COUNT];

double time_diff_ms(struct timespec start, struct timespec end) {
    double result = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    return result;
}

void initialize_astronauts(){     // can also replace astronaut when it gets disconnected
 
    if (astronauts[0].connect == 0){
        astronauts[0].id = 'A';
        astronauts[0].move_type = 'V';
        astronauts[0].x = 1;
        astronauts[0].y = FIELD_SIZE/2;
        astronauts[0].score = -1;
    }
 
    if (astronauts[1].connect == 0){
        astronauts[1].id = 'B';
        astronauts[1].move_type = 'V';
        astronauts[1].x = FIELD_SIZE-2;
        astronauts[1].y = FIELD_SIZE/2;
        astronauts[1].score = -1;
    }
 
    if (astronauts[2].connect == 0){ 
        astronauts[2].id = 'C';
        astronauts[2].move_type = 'H';
        astronauts[2].x = FIELD_SIZE/2;
        astronauts[2].y = 1;
        astronauts[2].score = -1;
    }
 
    if (astronauts[3].connect == 0){
        astronauts[3].id = 'D';
        astronauts[3].move_type = 'H';
        astronauts[3].x = FIELD_SIZE/2;
        astronauts[3].y = FIELD_SIZE-2;
        astronauts[3].score = -1;
    }
 
    if (astronauts[4].connect == 0){ 
        astronauts[4].id = 'E';
        astronauts[4].move_type = 'V';
        astronauts[4].x = 0;
        astronauts[4].y = FIELD_SIZE/2;
        astronauts[4].score = -1;
    }
 
    if (astronauts[5].connect == 0){
        astronauts[5].id = 'F';
        astronauts[5].move_type = 'V';
        astronauts[5].x = FIELD_SIZE-1;
        astronauts[5].y = FIELD_SIZE/2;
        astronauts[5].score = -1;
    }
 
    if (astronauts[6].connect == 0){ 
        astronauts[6].id = 'G';
        astronauts[6].move_type = 'H';
        astronauts[6].x = FIELD_SIZE/2;
        astronauts[6].y = 0;
        astronauts[6].score = -1;   
        }
 
    if (astronauts[7].connect == 0){ 
        astronauts[7].id = 'H';
        astronauts[7].move_type = 'H';
        astronauts[7].x = FIELD_SIZE/2;
        astronauts[7].y = FIELD_SIZE-1;
        astronauts[7].score = -1;
    }
}

void generate_aliens(){

    pid_t pid = fork();  // create a new process

    if (pid < 0) {
        // error occurred
        perror("Fork failed");
        exit(1); // terminate child process
    } else if (pid == 0) {
        void *context = zmq_ctx_new();
        void *requester = zmq_socket(context, ZMQ_REQ);
        if (zmq_connect(requester, INTERACTION_ADDRESS) != 0) {
            printf("Error connecting to server: %s\n", zmq_strerror(zmq_errno()));
        }
        
        interaction_message m;

        // connect to server
        m.msg_type = 4;
        if (zmq_send(requester, &m, sizeof(m), 0) == -1) {
            printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
        }

        // receive assigned letter
        connect_message connect_reply;
        zmq_recv(requester, &connect_reply, sizeof(connect_message), 0);

        m.id = connect_reply.id;
        memcpy(m.token, connect_reply.token, TOKEN_SIZE);

        srand((int)m.id);

        while (1) {
            m.msg_type = 5;
            
            m.direction = random()%4;

            zmq_send(requester, &m, sizeof(m), 0);
            
            zmq_recv(requester, &connect_reply, sizeof(connect_reply), 0);

            if(connect_reply.connect==0){
                break;
            }
            sleep(1);            
        }
        // cleanup
        zmq_close(requester);
        zmq_ctx_destroy(context);

        exit(0); // terminate child process

    } else {// parent process
    }
}

void initialize_game() {
    // initialize the game field to empty spaces
    for (int i = 0; i < FIELD_SIZE; i++) {
        for (int j = 0; j < FIELD_SIZE; j++) {
            state.game_field[i][j] = ' '; // fill with empty spaces
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) { // initialize astronauts
        astronauts[i].connect = 0;
    } 

    for (int i = 0; i < ALIEN_MAX_COUNT; i++) { // initialize aliens
        aliens[i].connect = 0;
    }

    initialize_astronauts();

    for (int i = 0; i < ALIEN_MAX_COUNT; i++) {
        generate_aliens();
    }
}

int find_astronaut_index(char id) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (astronauts[i].id == id) return i;
    }
    return -1;
}

int find_alien_index(char id) {
    for (int i = 0; i < ALIEN_MAX_COUNT; i++) {
        if (aliens[i].id == id) return i;
    }
    return -1;
}

void clear_laser_path(Astronaut *astronaut) {
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

void draw_laser_path(Astronaut *astronaut) {
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

void update_game_state() {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time); 

    // update laser rays
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Astronaut *astronaut = &astronauts[i];

        if (astronaut->zap_active && astronaut->connect) {

            if (time_diff_ms(current_time, astronaut->finished_zap) < 0) {
                clear_laser_path(astronaut);
                astronaut->zap_active = 0;
                continue;
            }

            // check for alien hits in the zap's path   
            int start_x = astronaut->x;                 
            int start_y = astronaut->y;

            for (int j = 0; j <= ALIEN_MAX_COUNT; j++) {

                if (astronaut->move_type == 'H' && aliens[j].connect && aliens[j].x == start_x) {

                    state.game_field[start_x][aliens[j].y] = '|'; // remove alien from field
                    (astronaut->score)++;
                    aliens[j].connect = 0;
                }

                else if (astronaut->move_type == 'V' && aliens[j].connect && aliens[j].y == start_y) {

                    state.game_field[aliens[j].x][start_y] = '-'; // remove alien from field
                    (astronaut->score)++;
                    aliens[j].connect = 0;
                    
                }
            }       

            // check for other astronauts in the line of fire
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (astronauts[j].id != astronaut->id && astronaut[j].connect) { 
                    if ((astronaut->move_type == 'H' && astronaut->x == astronauts[j].x) ||
                        (astronaut->move_type == 'V' && astronauts[i].y == astronauts[j].y)) {
                        astronauts[j].finished_stunned = current_time;
                        astronauts[j].finished_stunned.tv_sec += STUN_DURATION;
                    }
                }
            }
        }

        // update scores
        state.astronaut_scores[i] = astronaut->score;
    }
}

char validate_message(interaction_message *message) {     
    if (message->msg_type < 0 || message->msg_type > 5) return 0;

    // handle astronauts
    if (message->msg_type >= 0 && message->msg_type <= 3) {
        if (message->id < 'A' || message->id > 'H') return 0;

        int index = find_astronaut_index(message->id);
        if (index < 0 || memcmp(message->token, astronauts[index].token, TOKEN_SIZE) != 0) return 0;

        return message->id;
    }

    // handle aliens
    if (message->msg_type <= 5) {
        
        return message->id;
    }

    return 0;
}

void move_astronaut(Astronaut *astronaut, int old_x, int old_y) {
    if (old_x == astronaut->x && old_y == astronaut->y) return;
    state.game_field[astronaut->x][astronaut->y] = astronaut->id;
    if (old_x == -1 || old_y == -1) return;
    state.game_field[old_x][old_y] = ' ';
}

void generate_token(unsigned char *token) {
    getrandom(token, TOKEN_SIZE, 0);
}

void handle_astronaut_connect(void *client_socket) {   
    connect_message con_reply;
    if (astronaut_count >= MAX_PLAYERS) {
        con_reply.id = 'Z';
        zmq_send(client_socket, &con_reply, sizeof(con_reply), 0);
        return;
    }

    // get index of first free astronaut
    int index = 0; 
    for ( index=0; index <= MAX_PLAYERS; index++) {
        if (astronauts[index].connect == 0){
            break;
        }
    }

    Astronaut *astronaut = &astronauts[index];

    // set up new astronaut
    char id = 'A' + index;
    astronaut->id = id;
    astronaut->score = 0;
    astronaut->connect = 1;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    astronaut->finished_stunned = current_time;

    // place astronaut
    move_astronaut(astronaut, -1, -1);

    // reply 
    con_reply.id = id;
    generate_token(astronaut->token);
    memcpy(con_reply.token, astronaut->token, TOKEN_SIZE);
    zmq_send(client_socket, &con_reply, sizeof(con_reply), 0);
    astronaut_count++;
}


void handle_alien_movement(interaction_message message){

    int index = find_alien_index(message.id);
    if (index == -1) return;

    Alien *alien = &aliens[index];

    int new_x = alien->x;
    int new_y = alien->y;

    // old position clear    
    state.game_field[alien->x][alien->y] = ' ';

    // check if there was another alien there
    for (int i = 0; i < ALIEN_MAX_COUNT; i++) {

        if (alien->x == aliens[i].x && alien->y == aliens[i].y && i!= index && aliens[i].connect) {

            state.game_field[alien->x][alien->y] = '*'; // put star back
        }
                        
    }

    switch(message.direction) {
        case DOWN: // Move down
            new_y++;
            break;
        case UP: // Move up
            new_y--;
            break;
        case RIGHT: // Move right
            new_x++;
            break;
        case LEFT: // Move left
            new_x--;
            break;
        }

    // set new position
    if (new_x < ASTRONAUT_MAX && new_y < ASTRONAUT_MAX && new_x >= ASTRONAUT_MIN && new_y >= ASTRONAUT_MIN){
        alien->y = new_y;
        alien->x = new_x;
    }
    
    state.game_field[alien->x][alien->y] = '*';
}

void handleAlienConnect(void *client_socket){

    connect_message con_reply;
    int i;

    for (i = 0; i < ALIEN_MAX_COUNT; i++) {
        if (aliens[i].connect == 0){
            generate_token(aliens[i].token);
            aliens[i].connect = 1;
            aliens[i].id = 'J' + i;
            break;
        }                        
    }
    
    // Generate random number between for position
    aliens[i].x = rand() % (FIELD_SIZE - 4) + 2;  
    aliens[i].y = rand() % (FIELD_SIZE - 4) + 2;

    state.game_field[aliens[i].x][aliens[i].y] = '*';
    
    con_reply.id = aliens[i].id;
    memcpy(con_reply.token, aliens[i].token, TOKEN_SIZE);
    zmq_send(client_socket, &con_reply, sizeof(con_reply), 0);
    
    return;
}

void handle_astronaut_movement(char id, direction_t direction) {
    int index = find_astronaut_index(id);
    Astronaut *astronaut = &astronauts[index];

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double test = time_diff_ms(current_time, astronaut->finished_stunned);
    if (index == -1 || test > 0.0) return;
    
    clear_laser_path(astronaut);

    int new_x = astronaut->x;
    int new_y = astronaut->y;
    int old_x = astronaut->x;
    int old_y = astronaut->y;

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
    if (new_y >= ASTRONAUT_MIN && new_y < ASTRONAUT_MAX) {
        astronaut->y = new_y;
    }

    // Check bounds for horizontal movement
    if (new_x >= ASTRONAUT_MIN && new_x < ASTRONAUT_MAX) {
        astronaut->x = new_x;
    }

    move_astronaut(astronaut, old_x, old_y);

    if (time_diff_ms(current_time, astronaut->finished_zap) >= 0) {
        draw_laser_path(astronaut);
    }
}

void handle_astronaut_zap(char id) {
    int index = find_astronaut_index(id);

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

void handle_astronaut_disconnect(char id) {
    int index = find_astronaut_index(id);
    if (index == -1) return;

    astronauts[index].connect = 0;

    state.game_field[astronauts[index].x][astronauts[index].y] = ' ';

    // Reset astronauts that were disconnected
    initialize_astronauts();

    astronaut_count--;
}

void process_message(void *socket) {
    interaction_message message;

    int size = zmq_recv(socket, &message, sizeof(message), 0);

    if (size == -1) {
        printf("Error receiving message: %s\n", zmq_strerror(zmq_errno()));
        return;
    }
    
    if (message.msg_type == 0) {
        handle_astronaut_connect(socket);
        update_game_state();
        return;
    }

    if (message.msg_type == 4) {
        handleAlienConnect(socket);
        update_game_state();
        return;
    }

    char id = validate_message(&message);

    // handle invalid message
    if (id == 0) {
        zmq_send(socket, NULL, 0, 0);
        return;
    }   

    connect_message con_reply;
    con_reply.connect = 1;

    switch(message.msg_type) {
        case 1:
            handle_astronaut_movement(id, message.direction);
            break;
        case 2:
            handle_astronaut_zap(id);
            break;
        case 3:
            handle_astronaut_disconnect(id);
            con_reply.connect = 0;
            break;
        case 5:
            int index = find_alien_index(id);
            if(index > -1 && aliens[index].connect == 0){    // remove the aliens
                con_reply.connect = 0;
                break;
            }
            handle_alien_movement(message);
    }
    // Send the scores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        con_reply.scores[i] = state.astronaut_scores[i];
    }

    update_game_state();
    zmq_send(socket, &con_reply, sizeof(con_reply), 0);
}

void print_game_field(game_state *state, WINDOW *number_window, WINDOW *game_window, WINDOW *score_window) {
    
    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, i, 0, "%d", (i-1) % 10);
    }

    // Print row numbers on left and game field
    for (int i = 1; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, 0, i, "%d", (i-1) % 10);
        for (int j = 1; j < FIELD_SIZE + 1; j++) {
            wmove(game_window, j, i);
            waddch(game_window, state->game_field[i-1][j-1] | A_BOLD);
        }
    }

    // update score screen
    werase(score_window);
    box(score_window, 0 , 0);
    mvwprintw(score_window, 1, 1, "SCORE");

    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++){

        if(state->astronaut_scores[i] > -1){
        mvwprintw(score_window, count +2, 1, "%c - %d",letters[i], state->astronaut_scores[i]);
        count++;
        }
    } 
  
    box(game_window, 0, 0);
    wrefresh(number_window);
    wrefresh(game_window);
    wrefresh(score_window);
}

int main() {
    void *context = zmq_ctx_new();

    // interaction REP socket
    void *responder_interaction = zmq_socket(context, ZMQ_REP);
    if (zmq_bind(responder_interaction, INTERACTION_ADDRESS) == -1) {
        printf("Error binding: %s\n", zmq_strerror(zmq_errno()));
        return -1;
    }
    // assert(responder_interaction == 0);

    // display PUP socket
    void *publisher_display = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_display, DISPLAY_ADDRESS);

    // start variables
    initialize_game();

    // lncurses 
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    WINDOW *numbers_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE+5);

    while (1) {
        // process current message, update game state and respond        
        process_message(responder_interaction);

        // print the game field to the screen
        print_game_field(&state, numbers_window, game_window, score_window);

        // send updated game state
        zmq_send(publisher_display, &state, sizeof(state), 0);
    }

    // cleanup
    endwin();
    zmq_close(responder_interaction);
    zmq_close(publisher_display);
    zmq_ctx_destroy(context);
    
    return 0;
}