
#define INTERACTION_ADDRESS "tcp://localhost:5555"
#define DISPLAY_ADDRESS "tcp://localhost:5556"

#define MAX_PLAYERS 8
#define FIELD_SIZE 20

typedef struct game_state {
    char game_field[FIELD_SIZE][FIELD_SIZE];
    int astronaut_scores[MAX_PLAYERS]; // -1 not present, order from A to H
} game_state;

typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct astronaut_client {   
    int msg_type; /* 0 - connect   1 - move  2 - zap  3 - disconnect  4 - alien connect  5 - alien move*/
    char id;                                 
    direction_t direction;
    // int connection; // here we can verify if the the player disconnected when it's equal to zero
    int code;  // need code (int)
} astronaut_client;

typedef struct astronaut_connect {
    char id;
    int code;
    int connect;
    int scores[MAX_PLAYERS];
} astronaut_connect;

