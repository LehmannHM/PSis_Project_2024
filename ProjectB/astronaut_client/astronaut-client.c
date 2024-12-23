#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "../common.h"

int main() {
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);

    if (zmq_connect(requester, INTERACTION_ADDRESS) != 0) {
        printf("Error connecting to server: %s\n", zmq_strerror(zmq_errno()));
    }

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    char letters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    interaction_message m;
    // connect to server
    m.msg_type = 0;
    if (zmq_send(requester, &m, sizeof(m), 0) == -1) {
        printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
    }

    // receive assigned letter
    connect_message connect_reply;
    zmq_recv(requester, &connect_reply, sizeof(connect_message), 0);

    // received id out of range
    if (connect_reply.id < 'A' || connect_reply.id > 'H') {
        printf("Max Players reached");
        return 0;
    }

    int ch;
    m.id = connect_reply.id;
    // m.code = connect_reply.code;
    memcpy(m.token, connect_reply.token, MAX_PLAYERS);

    // exit when q is pressed
    while ((ch = getch()) != 'q' && ch != 'Q') {
        m.msg_type = 1;
        
        switch (ch) {
            case KEY_UP:
                m.direction = UP;
                break;
            case KEY_DOWN:
                m.direction = DOWN;
                break;
            case KEY_LEFT:
                m.direction = LEFT;
                break;
            case KEY_RIGHT:
                m.direction = RIGHT;
                break;
            case ' ':
                m.msg_type = 2;
                break;
            default:
                continue;
        }

        // send action to server
        zmq_send(requester, &m, sizeof(m), 0);

        // receive scores
        zmq_recv(requester, &connect_reply, sizeof(connect_reply), 0); 

        int count = 0;
        clear();
        for (int i = 0; i < MAX_PLAYERS; i++){
            // only print scores of connected astronauts
            if(connect_reply.scores[i] > -1){

                
                mvprintw(count, 0, "%c - %d",letters[i], connect_reply.scores[i]); 
                if (letters[i] == m.id){
                    mvprintw(count, 10, "[YOUR PLAYER]");     // identify player
                }
                
                count++;
            }
        } 

        refresh();
    }

    // disconnect
    m.msg_type = 3;
    zmq_send(requester, &m, sizeof(m), 0);
    zmq_recv(requester, NULL, 0, 0);

    // cleanup
    endwin();
    zmq_close(requester);
    zmq_ctx_destroy(context);
    return 0;
}