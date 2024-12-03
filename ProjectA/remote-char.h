
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct remote_char_t
{   
    int msg_type; /* 0 join   1 - move */
    char ch;                                 
    direction_t direction ;                  // CAN be USED for the project
    int conection;                          // here we can verify if the the player disconnected when it's equal to zero
                                            // need code (int)
    /* data */
}remote_char_t;

#define ZMQ_LOC "ipc:///tmp/s1"


// can serve for the the display

typedef struct display_info
{   
    int msg_type2; /* 0 join   1 - move */
    char ch; 
    int pos_x ;               
    int pos_y;
    int pos_x_new;
    int pos_y_new;
    /* data */
}display_info;