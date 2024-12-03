#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "sockets.h"

#define CONNECT 'C'
#define MOVE 'M'
#define ZAP 'Z'
#define DISCONNECT 'D'

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

    // Connect to server
    char connect_msg[2] = {CONNECT, '\0'};
    if (zmq_send(requester, connect_msg, sizeof(connect_msg), 0) == -1) {
        printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
    }

    // Receive assigned letter
    char astronaut_id;
    zmq_recv(requester, &astronaut_id, 1, 0);

    int score = 0;
    char msg[3];
    int ch;

    while ((ch = getch()) != 'q' && ch != 'Q') {
        msg[0] = '\0';
        msg[1] = astronaut_id;
        msg[2] = '\0';

        switch (ch) {
            case KEY_UP:
                msg[0] = MOVE;
                msg[2] = 'U';
                break;
            case KEY_DOWN:
                msg[0] = MOVE;
                msg[2] = 'D';
                break;
            case KEY_LEFT:
                msg[0] = MOVE;
                msg[2] = 'L';
                break;
            case KEY_RIGHT:
                msg[0] = MOVE;
                msg[2] = 'R';
                break;
            case ' ':
                msg[0] = ZAP;
                break;
            default:
                continue;
        }

        zmq_send(requester, msg, 3, 0);
        zmq_recv(requester, &score, sizeof(int), 0);

        mvprintw(0, 0, "Astronaut %c - Score: %d", astronaut_id, score);
        refresh();
    }

    // Disconnect
    msg[0] = DISCONNECT;
    msg[1] = astronaut_id;
    zmq_send(requester, msg, 2, 0);
    zmq_recv(requester, NULL, 0, 0);

    // Cleanup
    endwin();
    zmq_close(requester);
    zmq_ctx_destroy(context);
    return 0;
}