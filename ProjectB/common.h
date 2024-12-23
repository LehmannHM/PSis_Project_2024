#define INTERACTION_ADDRESS "tcp://localhost:5555"
#define DISPLAY_ADDRESS "tcp://localhost:5556"

#define MAX_PLAYERS 8
#define FIELD_SIZE 20

#define TOKEN_SIZE 8

typedef struct game_state {
    char game_field[FIELD_SIZE][FIELD_SIZE];
    int astronaut_scores[MAX_PLAYERS]; // -1 not present, order from A to H
} game_state;

typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

// interaction message for the astronauts and aliens
typedef struct interaction_message {   
    int msg_type; // 0 - connect   1 - move  2 - zap  3 - disconnect  4 - alien connect  5 - alien move
    char id;                                 
    direction_t direction;
    unsigned char token[TOKEN_SIZE]; // token to validate client messages
} interaction_message;

typedef struct connect_message {
    char id;
    unsigned char token[TOKEN_SIZE]; // token to validate client messages
    int connect; // 0 if disconnected, 1 if connected
    int scores[MAX_PLAYERS]; // send back all of the scores
} connect_message;

#include <ncurses.h>

void display_outer_space(game_state *state, WINDOW *number_window, WINDOW *game_window, WINDOW *score_window) {
    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, i, 0, "%d", (i-1) % 10);
    }

    for (int i = 2; i <= FIELD_SIZE + 1; i++) {
        mvwprintw(number_window, 0, i, "%d", (i-1) % 10);
        for (int j = 1; j < FIELD_SIZE + 1; j++) {
            wmove(game_window, j, i);
            waddch(game_window, state->game_field[i-1][j-1] | A_BOLD);
        }
    }

    werase(score_window);
    mvwprintw(score_window, 1, 1, "SCORE");

    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->astronaut_scores[i] > -1) {
            mvwprintw(score_window, count+2, 1, "%c - %d", letters[i], state->astronaut_scores[i]);
            count++;
        }
    }

    box(game_window, 0, 0);
    wrefresh(number_window);
    wrefresh(game_window);
    wrefresh(score_window);
}

