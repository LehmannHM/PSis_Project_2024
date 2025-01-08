#define INTERACTION_ADDRESS "tcp://localhost:5555"
#define DISPLAY_ADDRESS "tcp://localhost:5556"
#define SCORE_UPDATE_ADDRESS "tcp://localhost:5557"

#define MAX_PLAYERS 8
#define FIELD_SIZE 20

#define TOKEN_SIZE 8

extern char letters[8];

typedef struct game_state {
    char game_field[FIELD_SIZE][FIELD_SIZE];
    int astronaut_scores[MAX_PLAYERS]; // -1 not present, order from A to H
} game_state;

typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

// interaction message for the astronauts and aliens
typedef struct interaction_message {   
    int msg_type; // 0 - connect   1 - move  2 - zap  3 - disconnect
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

typedef void (*score_callback_t)(connect_message *connect_reply);

void handle_keyboard_input(void *requester, interaction_message *m, connect_message *connect_reply, score_callback_t callback);
void display_outer_space(game_state *state, WINDOW *number_window, WINDOW *game_window, WINDOW *score_window, char *current_player);