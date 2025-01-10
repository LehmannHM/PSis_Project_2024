#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "score_update.pb-c.h"
#include "../common.h"

#include <sys/types.h>
#include <pthread.h>

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

typedef struct {
    void *publisher_display;
    WINDOW *number_window;
    WINDOW *game_window;
    WINDOW *score_window;
} update_thread_args;

// initialize global variables
game_state state;
Astronaut astronauts[MAX_PLAYERS];
int astronaut_count = 0;
Alien aliens[ALIEN_MAX_COUNT];
//--------------------------
int alien_count;
int alien_add_count;

struct timespec alien_respawn_time;

// thread initializing
pthread_mutex_t mtx_alien = PTHREAD_MUTEX_INITIALIZER;

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

                    pthread_mutex_lock(&mtx_alien);

                    state.game_field[start_x][aliens[j].y] = '|'; // remove alien from field
                    (astronaut->score)++;
                    aliens[j].connect = 0;
                    alien_count--;   // update counter

                    alien_respawn_time = current_time;
                    alien_respawn_time.tv_sec += 10;

                    pthread_mutex_unlock(&mtx_alien);
                }

                else if (astronaut->move_type == 'V' && aliens[j].connect && aliens[j].y == start_y) {

                    pthread_mutex_lock(&mtx_alien);

                    state.game_field[aliens[j].x][start_y] = '-'; // remove alien from field
                    (astronaut->score)++;
                    aliens[j].connect = 0;
                    alien_count--;  // update counter

                    alien_respawn_time = current_time;
                    alien_respawn_time.tv_sec += 10;

                    pthread_mutex_unlock(&mtx_alien);
                }
            }       

            // check for other astronauts in the line of fire
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (astronauts[j].id != astronaut->id && astronauts[j].connect == 1) { 
                    if ((astronaut->move_type == 'H' && astronaut->x == astronauts[j].x) ||
                        (astronaut->move_type == 'V' && astronaut->y == astronauts[j].y)) {
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

void * thread_alien_function(void * arg){  // single thread to manage aliens
    
    // initialize aliens
    pthread_mutex_lock(&mtx_alien);
    for (int i = 0; i < ALIEN_MAX_COUNT; i++) {
        
        
        // initialize alien
        aliens[i].connect = 1;
        aliens[i].id = i;
        aliens[i].x = rand() % (FIELD_SIZE - 4) + 2;
        aliens[i].y = rand() % (FIELD_SIZE - 4) + 2;

        // add to the game field
        state.game_field[aliens[i].x][aliens[i].y] = '*';

    }
    alien_count = ALIEN_MAX_COUNT;
    pthread_mutex_unlock(&mtx_alien);

    direction_t direction;
    int to_add=0;


    int new_x;
    int new_y;

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    alien_respawn_time = current_time;
    alien_respawn_time.tv_sec += 10;   // add 10 seconds
    
    while(1){

        pthread_mutex_lock(&mtx_alien);
        for (int i = 0; i < ALIEN_MAX_COUNT; i++) {

            if (aliens[i].connect)
            {
                // get old pos
                new_x = aliens[i].x;
                new_y = aliens[i].y;

                // old position clear    
                state.game_field[aliens[i].x][aliens[i].y] = ' ';

                // check if there was another alien there
                for (int j = 0; j < ALIEN_MAX_COUNT; j++) {

                    if (aliens[i].x == aliens[j].x && aliens[i].y == aliens[j].y && i != j && aliens[j].connect) {

                        state.game_field[aliens[i].x][aliens[i].y] = '*'; // put star back
                    }
                                    
                }

                direction = random()%4;

                switch(direction) {
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
                aliens[i].y = new_y;
                aliens[i].x = new_x;
            }
            // set new pos
            state.game_field[aliens[i].x][aliens[i].y] = '*';

            }
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time);

        if (time_diff_ms(current_time, alien_respawn_time) < 0)
        {

            to_add = alien_count*0.1;
            // add 10% of aliens
            for (int j = 0; j < ALIEN_MAX_COUNT; j++)
            {
                // get out the loop if there is no aliens to add
                if (to_add == 0){
                    break;
                }

                if (aliens[j].connect == 1) continue;

                int attempts = 0;
                do {
                    aliens[j].x = rand() % (FIELD_SIZE - 4) + 2;
                    aliens[j].y = rand() % (FIELD_SIZE - 4) + 2;
                    attempts++;
                } while (state.game_field[aliens[j].x][aliens[j].y] == '*' && attempts < 100);

                if (attempts >= 100) {
                    // If we can't find a free spot after 100 attempts, skip adding this alien
                    aliens[j].connect = 0;
                    continue;
                } else {
                    // adding new alien
                    aliens[j].connect = 1;
                    aliens[j].id = j;

                    // add to the game field
                    state.game_field[aliens[j].x][aliens[j].y] = '*';

                    // update alien_counter
                    alien_count ++;

                    // one less alien to add
                    to_add--;
                }                
            }

            alien_respawn_time = current_time;
            alien_respawn_time.tv_sec += 10;   // add 10 seconds
            
        }
       
        pthread_mutex_unlock(&mtx_alien);
        usleep(1000000);
    }
}

void *game_state_update_thread(void *args) {
    update_thread_args *update_args = (update_thread_args *)args;

    while (1) {
        update_game_state();
        display_outer_space(&state, update_args->number_window, update_args->game_window, update_args->score_window, NULL);
        zmq_send(update_args->publisher_display, &state, sizeof(state), 0);

        usleep(100000);
    }
    return NULL;
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

    pthread_t t_alien;
    pthread_create(&t_alien, NULL, thread_alien_function, NULL);
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

    char id = validate_message(&message);

    // handle invalid message
    if (id == 0) {
        zmq_send(socket, NULL, 0, 0);
        return;
    }   

    connect_message con_reply;
    con_reply.id = id;
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
    }
    // Send the scores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        con_reply.scores[i] = state.astronaut_scores[i];
    }

    update_game_state();
    zmq_send(socket, &con_reply, sizeof(con_reply), 0);
}

// Function to send score updates
void send_score_update(void *publisher_score) {
    ScoreUpdate score_update = SCORE_UPDATE__INIT;
    int scores[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        scores[i] = state.astronaut_scores[i];
    }
    score_update.n_scores = MAX_PLAYERS;
    score_update.scores = scores;

    size_t len = score_update__get_packed_size(&score_update);
    void *buf = malloc(len);
    score_update__pack(&score_update, buf);

    zmq_send(publisher_score, buf, len, 0);
    free(buf);
}

int main() {

    void *context = zmq_ctx_new();

    // interaction REP socket
    void *responder_interaction = zmq_socket(context, ZMQ_REP);
    if (zmq_bind(responder_interaction, INTERACTION_ADDRESS) == -1) {
        printf("Error binding: %s\n", zmq_strerror(zmq_errno()));
        return -1;
    }

    // display PUB socket
    void *publisher_display = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_display, DISPLAY_ADDRESS);

    // score update PUB socket
    void *publisher_score = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_score, SCORE_UPDATE_ADDRESS);

    // start variables
    initialize_game();

    // ncurses 
    initscr();		    	
    cbreak();				
    keypad(stdscr, TRUE);   
    noecho();			    

    WINDOW *numbers_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE+5);

    pthread_t game_state_thread;
    update_thread_args update_args = {publisher_display, numbers_window, game_window, score_window};
    pthread_create(&game_state_thread, NULL, game_state_update_thread, &update_args);

    while (1) {
        // process current message, update game state and respond        
        process_message(responder_interaction);

        // print the game field to the screen
        display_outer_space(&state, numbers_window, game_window, score_window, NULL);

        // send updated game state
        zmq_send(publisher_display, &state, sizeof(state), 0);

        // send score update
        send_score_update(publisher_score);
    }

    // cleanup
    endwin();
    zmq_close(responder_interaction);
    zmq_close(publisher_display);
    zmq_close(publisher_score);
    zmq_ctx_destroy(context);
    return 0;
}