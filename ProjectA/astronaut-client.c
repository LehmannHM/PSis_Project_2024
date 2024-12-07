#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

int main() {
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    // zmq_connect(requester, INTERACTION_ADDRESS);
    if (zmq_connect(requester, INTERACTION_ADDRESS) != 0) {
        printf("Error connecting to server: %s\n", zmq_strerror(zmq_errno()));
    }

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    astronaut_client m;
    int code;
    // Connect to server
    m.msg_type = 0;
    if (zmq_send(requester, &m, sizeof(m), 0) == -1) {
        printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
    }

    // Receive assigned letter
    astronaut_connect connect_reply;
    zmq_recv(requester, &connect_reply, sizeof(astronaut_connect), 0);
    if (connect_reply.id < 'A' || connect_reply.id > 'H') {
        printf("Max Players reached");
        return 0;
    }

    code = connect_reply.code;

    int score = 0;
    int ch;
    m.id = connect_reply.id;
    m.code = connect_reply.code;

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

        zmq_send(requester, &m, sizeof(m), 0);

        //int astronaut_scores[MAX_PLAYERS];
        //zmq_recv(requester, &astronaut_scores, sizeof(int), 0); 
        zmq_recv(requester, &connect_reply, sizeof(connect_reply), 0); // update this
        
        //mvprintw(0, 0, "Astronaut %c - Score: %d", m.id, astronaut_scores[0]);
        //mvprintw(0, 0, "Astronaut %c - Score: %d", m.id, connect_reply.scores[0]);
        mvprintw(0, 0, "Astronaut %c - Score: %d", m.id, connect_reply.scores[m.id-'A']);
        refresh();
    }

    // Disconnect
    m.msg_type = 3;
    zmq_send(requester, &m, sizeof(m), 0);
    zmq_recv(requester, NULL, 0, 0);

    // Cleanup
    endwin();
    zmq_close(requester);
    zmq_ctx_destroy(context);
    return 0;
}