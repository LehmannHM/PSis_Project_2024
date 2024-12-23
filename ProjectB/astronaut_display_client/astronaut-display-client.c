#include <zmq.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "../common.h"

int main() {
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    void *subscriber = zmq_socket(context, ZMQ_SUB);

    if (zmq_connect(requester, INTERACTION_ADDRESS) != 0) {
        printf("Error connecting to server: %s\n", zmq_strerror(zmq_errno()));
    }

    if (zmq_connect(subscriber, DISPLAY_ADDRESS) != 0) {
        printf("Error connecting to display server: %s\n", zmq_strerror(zmq_errno()));
    }

    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    WINDOW *number_window = newwin(FIELD_SIZE + 3, FIELD_SIZE + 3, 0, 0);
    WINDOW *game_window = newwin(FIELD_SIZE + 2, FIELD_SIZE + 2, 1, 1);
    WINDOW *score_window = newwin(15, 30, 1, FIELD_SIZE + 5);
    box(score_window, 0, 0);
    wrefresh(score_window);

    interaction_message m;
    m.msg_type = 0;
    if (zmq_send(requester, &m, sizeof(m), 0) == -1) {
        printf("Error sending message: %s\n", zmq_strerror(zmq_errno()));
    }

    connect_message connect_reply;
    zmq_recv(requester, &connect_reply, sizeof(connect_message), 0);

    if (connect_reply.id < 'A' || connect_reply.id > 'H') {
        printf("Max Players reached");
        return 0;
    }

    int ch;
    m.id = connect_reply.id;
    memcpy(m.token, connect_reply.token, TOKEN_SIZE);

    game_state current_state;

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
        zmq_recv(requester, &connect_reply, sizeof(connect_reply), 0);

        zmq_recv(subscriber, &current_state, sizeof(current_state), 0);
        display_outer_space(&current_state, number_window, game_window, score_window);
    }

    m.msg_type = 3;
    zmq_send(requester, &m, sizeof(m), 0);
    zmq_recv(requester, NULL, 0, 0);

    endwin();
    zmq_close(requester);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    return 0;
}